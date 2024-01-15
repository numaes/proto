/*
 * core.cpp
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#include "../headers/proto.h"

#include <malloc.h>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <functional>
#include <condition_variable>

using namespace std;
using namespace std::literals::chrono_literals;

namespace proto {

#define GC_SLEEP_MILLISECONDS           1000
#define BLOCKS_PER_ALLOCATION           1024
#define BLOCKS_PER_MALLOC_REQUEST       8 * BLOCKS_PER_ALLOCATION
#define MAX_ALLOCATED_CELLS_PER_CONTEXT 1024

#define KB                              1024
#define MB                              1024 * KB
#define GB                              1024 * MB
#define MAX_HEAP_SIZE                   256 * MB

std::mutex		ProtoSpace::globalMutex;

void gcCollectCells(ProtoContext *context, void *self, Cell *value) {
     ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) value;

    ProtoSparseList *cellSet = new(context) ProtoSparseList(context);

    // Go further in the scanning only if it is a cell and the cell belongs to current context!
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE) {
        // It is an object pointer with references
        if (! cellSet->has(context, p.asHash.hash)) {
            cellSet = cellSet->setAt(context, p.asHash.hash, PROTO_NONE);
            p.cell.cell->processReferences(context, (void *) cellSet, gcCollectCells);
        }
    }
}

void gcCollectObjects(ProtoContext *context, void *self, ProtoObject *value) {
     ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) value;

    ProtoSparseList *cellSet = new(context) ProtoSparseList(context);

    // Go further in the scanning only if it is a cell and the cell belongs to current context!
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE) {
        // It is an object pointer with references
        if (! cellSet->has(context, p.asHash.hash)) {
            cellSet = cellSet->setAt(context, p.asHash.hash, PROTO_NONE);
            p.cell.cell->processReferences(context, (void *) cellSet, gcCollectCells);
        }
    }
}

void gcScan(ProtoContext *context, ProtoSpace *space) {
    DirtySegment *toAnalize;

    // Acquire space lock and take all dirty segments to analyze

    BOOLEAN oldValue = FALSE;
    while (space->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    toAnalize = space->dirtySegments;
    space->dirtySegments = NULL;

    space->gcLock.store(FALSE);

    ProtoContext gcContext(context); 

    // Stop the world
    // Wait till all managed threads join the stopped state
    // After stopping the world, no managed thread is changing its state
    
    space->state = SPACE_STATE_STOPPING_WORLD;
    while (space->state == SPACE_STATE_STOPPING_WORLD) {
        std::unique_lock<std::mutex> lk(ProtoSpace::globalMutex);
        space->stopTheWorldCV.wait(lk);

        int allStoping = TRUE;
        for (unsigned n = 0; n < space->threads->count; n++) {
            ProtoThread *t = (ProtoThread *) space->threads->getAt(&gcContext, n);

            // Be sure no thread is still in managed state
            if (t->state == THREAD_STATE_MANAGED) {
                allStoping = FALSE;
                break;
            }
        }
        if (allStoping)
            space->state = SPACE_STATE_WORLD_TO_STOP;
    }

    space->restartTheWorldCV.notify_all();

    while (space->state == SPACE_STATE_WORLD_TO_STOP) {
        std::unique_lock<std::mutex> lk(ProtoSpace::globalMutex);
        space->stopTheWorldCV.wait(lk);

        int allStopped = TRUE;
        for (unsigned n = 0; n < space->threads->count; n++) {
            ProtoThread *t = (ProtoThread *) space->threads->getAt(&gcContext, n);

            if (t->state != THREAD_STATE_STOPPED) {
                allStopped = FALSE;
                break;
            }
        }
        if (allStopped)
            space->state = SPACE_STATE_WORLD_STOPPED;

        space->restartTheWorldCV.notify_all();
    }

    // cellSet: a set of all referenced Cells

    ProtoSparseList *cellSet = new(context) ProtoSparseList(context);

    // Add all mutables to cellSet
    ((ProtoSparseList *) space->mutableRoot.load())->processValues(
        &gcContext,
        &cellSet,
        gcCollectObjects
    );

    // Collect all roots from thread stacks

    int threadCount = space->threads->count;
    while (threadCount--) {
        ProtoThread *thread = (ProtoThread *) space->threads->getAt(&gcContext, threadCount);

        // Collect allocated objects
        ProtoContext *currentContext = thread->currentContext;
        while (currentContext) {
            Cell * currentCell = currentContext->lastAllocatedCell;
            while (currentCell) {
                if (! cellSet->has(context, currentCell->getHash(context))) {
                    cellSet = cellSet->setAt(context, currentCell->getHash(context));
                }

                currentCell = currentCell->nextCell;
            }

            if (currentContext->localsBase) {
                ProtoObjectPointer *p = currentContext->localsBase;
                for (int n = currentContext->localsCount;
                     n > 0;
                     n--) {
                    if (p->op.pointer_tag != POINTER_TAG_EMBEDEDVALUE) {
                            cellSet = cellSet->setAt(context, p->asHash.hash);
                    }
                }
            }

            currentContext = currentContext->previous;
        }
    };

    // Free the world. Let them run
    space->state = SPACE_STATE_RUNNING;
    space->restartTheWorldCV.notify_all();

    // Deep Scan all indirect roots. Deep traversal of cellSet
    cellSet->processValues(context, cellSet, gcCollectObjects);

    // Scan all blocks to analyze and if they are not referenced, free them

    Cell *freeBlocks = NULL;
    Cell *firstBlock = NULL;
    int freeCount = 0;
    while (toAnalize) {
        Cell *block = toAnalize->cellChain;

        while (block) {
            Cell *nextCell = block->nextCell;

            if (!cellSet->has(&gcContext, block->getHash(context))) {
                block->~Cell();

                void **p = (void **) block;
                for (unsigned i = 0; i < sizeof(BigCell) / sizeof(void *); i++)
                    *p++ = NULL;

                if (! firstBlock)
                    firstBlock = block;

                block->nextCell = freeBlocks;
                freeBlocks = block;
                freeCount++;
            }
            block = nextCell;
        }

        DirtySegment *segmentToFree = toAnalize;
        toAnalize = toAnalize->nextSegment;
        delete segmentToFree;
    }

    // Update space freeCells
    oldValue = FALSE;
    while (space->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    if (firstBlock)
        firstBlock->nextCell = space->freeCells;
    
    space->freeCells = freeBlocks;
    space->freeCellsCount += freeCount;

    space->gcLock.store(FALSE);
};

void gcThreadLoop(ProtoSpace *space) {
    ProtoContext gcContext;

    space->gcStarted = TRUE;
    space->gcCV.notify_one();

    while (space->state != SPACE_STATE_RUNNING) {
        std::unique_lock<std::mutex> lk(ProtoSpace::globalMutex);

        space->gcCV.wait_for(lk, std::chrono::milliseconds(space->gcSleepMilliseconds));

        if (space->dirtySegments) {
            gcScan(&gcContext, space);
        }
    }
};

ProtoSpace::ProtoSpace(
    ProtoMethod mainFunction,
    int argc,
    char **argv    
) {
    this->state = SPACE_STATE_RUNNING;

    ProtoContext *creationContext = new ProtoContext(
        NULL,
        NULL,
        0,
        NULL,
        this
    );
    this->threads = creationContext->newList();
    this->tupleRoot = new(creationContext) TupleDictionary(creationContext);
    
    ProtoList *mainParameters = creationContext->newList();
    mainParameters = mainParameters->appendLast(
        creationContext,
        creationContext->fromInteger(argc)
    );
    ProtoList *argvList = creationContext->newList();
    if (argc && argv) {
        for (int i; i < argc; i++)
            argvList = argvList->appendLast(
                creationContext,
                creationContext->fromUTF8String(argv[i])->asObject(creationContext)
            );
    }
    mainParameters = mainParameters->appendLast(
        creationContext, argvList->asObject(creationContext)
    );

    mainThread->currentContext = creationContext;
    this->mainThreadId = std::this_thread::get_id();

    this->mutableLock.store(FALSE);
    this->threadsLock.store(FALSE);
    this->gcLock.store(FALSE);
    this->mutableRoot.store(new(creationContext) ProtoSparseList(creationContext));

    this->maxAllocatedCellsPerContext = MAX_ALLOCATED_CELLS_PER_CONTEXT;
    this->blocksPerAllocation = BLOCKS_PER_ALLOCATION;
    this->heapSize = 0;
    this->freeCellsCount = 0;
    this->gcSleepMilliseconds = GC_SLEEP_MILLISECONDS;
    this->maxHeapSize = MAX_HEAP_SIZE;
    this->blockOnNoMemory = FALSE;
    this->gcStarted = FALSE;

    // Create GC thread and ensure it is working
    this->gcThread = new std::thread(
        (void (*)(ProtoSpace *)) (&gcThreadLoop), 
        this
    );

    while (!this->gcStarted) {
        std::unique_lock<std::mutex> lk(globalMutex);
        this->gcCV.wait_for(lk, 100ms);
    }

    ProtoThread *mainThread = new(creationContext) ProtoThread(
        creationContext,
        creationContext->fromUTF8String("Main thread"),
        this,
        mainFunction,
        mainParameters
    );

    // Wait till main thread and gcThread end

    mainThread->join(creationContext);
    this->gcThread->join();
};

void scanThreads(ProtoContext *context, void *self, ProtoObject *value) {
    ProtoList **threadList = (ProtoList **) self;

    *threadList = (*threadList)->appendLast(context, value);
}

ProtoSpace::~ProtoSpace() {
    ProtoContext finalContext(NULL);

    int threadCount = this->threads->getSize(&finalContext);

    this->state = SPACE_STATE_ENDING;

    // Wait till all threads are ended
    for (int i = 0; i < threadCount; i++) {
        ProtoThread *t = (ProtoThread *) this->threads->getAt(
            &finalContext,
            i
        );
        t->join(&finalContext);
    }

    this->triggerGC();

    this->gcThread->join();
};

void ProtoSpace::triggerGC() {
    this->gcCV.notify_all();
}

void ProtoSpace::allocThread(ProtoContext *context, ProtoThread *thread) {
    BOOLEAN oldValue = FALSE;
    while (this->threadsLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    if (this->threads)
        this->threads = this->threads->appendLast(context, thread->asObject(context));
    
    this->threadsLock.store(FALSE);
};

void ProtoSpace::deallocThread(ProtoContext *context, ProtoThread *thread) {
    BOOLEAN oldValue = FALSE;
    while (this->threadsLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    int threadCount = this->threads->count;
    while (threadCount--) {
        ProtoThread * t = (ProtoThread *) this->threads->getAt(context, threadCount);
        if (t == thread) {
            this->threads = this->threads->removeAt(context, threadCount);
            break;
        }
    }

    this->threadsLock.store(FALSE);
};

Cell *ProtoSpace::getFreeCells(ProtoThread * currentThread){
    Cell *freeBlocks = NULL;
    Cell *newBlock = NULL;

    BOOLEAN oldValue = FALSE;
    while (this->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    for (int i=0; i < BLOCKS_PER_ALLOCATION; i++) {
        if (! this->freeCells) {
            // Alloc from OS

            int toAllocBytes = sizeof(BigCell) * BLOCKS_PER_MALLOC_REQUEST;
            if (this->maxHeapSize != 0 && !this->blockOnNoMemory && 
                this->heapSize + toAllocBytes >= this->maxHeapSize) {
                printf("\nPANIC ERROR: HEAP size will be bigger than configured maximun (%d is over %d bytes)! Exiting ...\n",
                       this->heapSize + toAllocBytes, this->maxHeapSize
                );
                std::exit(1);
            }

            if (this->maxHeapSize != 0 && this->blockOnNoMemory && 
                this->heapSize + toAllocBytes >= this->maxHeapSize) {
                while (!this->freeCells) {
                    this->gcLock.store(FALSE);

                    currentThread->synchToGC();

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    while (this->gcLock.compare_exchange_strong(
                        oldValue,
                        TRUE
                    )) std::this_thread::yield();
                }
            }
            else {
                BigCell *newBlocks = (BigCell *) malloc(toAllocBytes);
                if (!newBlocks) {
                    printf("\nPANIC ERROR: Not enough MEMORY! Exiting ...\n");
                    std::exit(1);
                }

                BigCell *currentBlock = newBlocks;
                Cell *lastBlock = this->freeCells;
                int allocatedBlocks = toAllocBytes / sizeof(BigCell);
                for (int n = 0; n < allocatedBlocks; n++) {
                    // Clear new allocated block
                    void **p =(void **) newBlocks;
                    for (unsigned long count = 0;
                        count < sizeof(BigCell) / sizeof(void *);
                        count++)
                        *p++ = NULL;

                    // Chain new blocks as a list
                    currentBlock->nextCell = lastBlock;
                    lastBlock = currentBlock++;
                }

                this->freeCells = lastBlock;

                this->heapSize += toAllocBytes;
                this->freeCellsCount += allocatedBlocks;
            }
        }

        if (this->freeCells) {
            newBlock = this->freeCells;
            this->freeCells = newBlock->nextCell;

            this->freeCellsCount -= 1;
            newBlock->nextCell = NULL;
        }
    }

    this->gcLock.store(FALSE);

    return newBlock;
};

void ProtoSpace::analyzeUsedCells(Cell *cellsChain) {
    DirtySegment *newChain;

    BOOLEAN oldValue = FALSE;
    while (this->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    newChain = new DirtySegment();
    newChain->cellChain = (BigCell *) cellsChain;
    newChain->nextSegment = this->dirtySegments;
    this->dirtySegments = newChain;

    this->gcLock.store(FALSE);
};

};
