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
#include <functional>
#include <condition_variable>

using namespace std;

namespace proto {

#define GC_SLEEP_MILLISECONDS           1000
#define BLOCKS_PER_ALLOCATION           1024
#define BLOCKS_PER_MALLOC_REQUEST       8 * BLOCKS_PER_ALLOCATION
#define MAX_ALLOCATED_CELLS_PER_CONTEXT 1024

void gcCollectCells(ProtoContext *context, void *self, Cell *value) {
     ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) value;

    ProtoSet *&cellSet = (ProtoSet *&) self;

    // Go further in the scanning only if it is a cell and the cell belongs to current context!
    if (p.op.pointer_tag == POINTER_TAG_CELL) {
        // It is an object pointer with references
        if (! cellSet->has(context, p.oid.oid)) {
            cellSet = cellSet->add(context, p.oid.oid);
            p.cell.cell->processReferences(context, (void *) cellSet, gcCollectCells);
        }
    }
}

void gcCollectObjects(ProtoContext *context, void *self, ProtoObject *value) {
     ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) value;

    ProtoSet *&cellSet = (ProtoSet *&) self;

    // Go further in the scanning only if it is a cell and the cell belongs to current context!
    if (p.op.pointer_tag == POINTER_TAG_CELL) {
        // It is an object pointer with references
        if (! cellSet->has(context, p.oid.oid)) {
            cellSet = cellSet->add(context, p.oid.oid);
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
        std::unique_lock lk(globalMutex);
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
        std::unique_lock lk(globalMutex);
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

    ProtoSet *cellSet = new(&gcContext) ProtoSet(&gcContext);

    // Add all mutables to cellSet
    ((IdentityDict *) space->mutableRoot.load())->processValues(
        &gcContext,
        &cellSet,
        gcCollectObjects
    );

    // Collect all roots from thread stacks

    int threadCount = space->threads->count;
    while (threadCount--) {
        ProtoThread *thread = (ProtoThread *) space->threads->getAt(&gcContext, threadCount);

        // Collect allocated objects
        ProtoContext *currentContext = thread->context;
        while (currentContext) {
            Cell * currentCell = currentContext->lastAllocatedCell;
            while (currentCell) {
                if (! cellSet->has(context, (ProtoObject *) currentCell)) {
                    cellSet = cellSet->add(context, (ProtoObject *) currentCell);
                }

                currentCell = currentCell->nextCell;
            }

            if (currentContext->localsBase) {
                ProtoObjectPointer *p = currentContext->localsBase;
                for (int n = currentContext->localsCount;
                     n > 0;
                     n--) {
                    if (p->op.pointer_tag == POINTER_TAG_CELL) {
                        if (p->cell.cell->type != CELL_TYPE_UNASSIGNED && p->cell.cell->type < CELL_TYPE_UNASSIGNED_C) {
                            if (! cellSet->has(context, (ProtoObject *) p)) {
                                cellSet = cellSet->add(context, (ProtoObject *) p);
                            }
                        }
                    }
                }
            }

            currentContext = currentContext->previous;
        }
    }

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

            if (!cellSet->has(&gcContext, (ProtoObject *) block)) {
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
    BOOLEAN oldValue = FALSE;
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

    while (space->state == SPACE_STATE_RUNNING) {
        if (space->dirtySegments) {
            gcScan(&gcContext, space);
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(GC_SLEEP_MILLISECONDS));
    }
};

ProtoSpace::ProtoSpace() {
    Cell *firstCell = this->getFreeCells();
    ProtoThread *firstThread = (ProtoThread *) firstCell;
    firstThread->freeCells = (BigCell *) firstCell->nextCell;
    firstThread->nextCell = NULL;
    // Get current thread id from OS
    firstThread->osThread = NULL;
    firstThread->space = this;

    firstCell->nextCell = NULL;

    this->state = SPACE_STATE_RUNNING;

    ProtoContext creationContext(
        NULL,
        NULL,
        0,
        firstThread,
        this
    );

    this->mainThreadId = std::this_thread::get_id();

    this->mutableLock.store(FALSE);
    this->threadsLock.store(FALSE);
    this->gcLock.store(FALSE);
    this->mutableRoot.store(new(&creationContext) IdentityDict(&creationContext));
    this->gcThread = new std::thread(
        (void (*)(ProtoSpace *)) (&gcThreadLoop), 
        this
    );
    this->maxAllocatedCellsPerContext = MAX_ALLOCATED_CELLS_PER_CONTEXT;
    this->blocksPerAllocation = BLOCKS_PER_ALLOCATION;
    this->heapSize = 0;
    this->freeCellsCount = 0;

    ProtoObject *threadName = creationContext.literalFromUTF8String((char *) "Main thread");
    firstThread->name = threadName;
    this->threads = (new(&creationContext) ProtoList(
        &creationContext,
        firstThread
    ))->appendFirst(&creationContext, firstThread);
};

void scanThreads(ProtoContext *context, void *self, ProtoObject *value) {
    ProtoList **threadList = (ProtoList **) self;

    *threadList = (*threadList)->appendLast(context, value);
}

ProtoSpace::~ProtoSpace() {
    ProtoContext finalContext(NULL);

    IdentityDict *threads = ((IdentityDict *) (this->threads->currentValue(&finalContext)));
    ProtoList *threadList = new(&finalContext) ProtoList(&finalContext);

    threads->processValues(&finalContext, &threadList, scanThreads);

    int threadCount = threadList->getSize(&finalContext);

    this->state = SPACE_STATE_ENDING;

    // Wait till all threads are ended
    for (int i = 0; i < threadCount; i++) {
        ProtoThread *t = (ProtoThread *) threadList->getAt(
            &finalContext,
            i
        );
        t->join(&finalContext);
    }

    this->gcThread->join();
};

void ProtoSpace::allocThread(ProtoContext *context, ProtoThread *thread) {
    BOOLEAN oldValue = FALSE;
    while (this->threadsLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    this->threads = this->threads->appendLast(context, thread);

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

Cell *ProtoSpace::getFreeCells(){
    Cell *freeBlocks = NULL;
    Cell *newBlocks, *newBlock;
    AllocatedSegment *newSegment;

    BOOLEAN oldValue = FALSE;
    while (this->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    for (int i=0; i < BLOCKS_PER_ALLOCATION; i++) {
        if (! this->freeCells) {
            // Alloc from OS
            // TODO: Limit the amount of RAM to ask to the OS per space
            // TODO: Record in space the total amount of RAM requested for Heap
            // TODO: Make the number of cells to be requested to the OS configurable per space
            // TODO: Make the number of cells returned to thread per request configurable per space

            BigCell *newBlocks = (BigCell *) malloc(sizeof(BigCell) * this->blocksPerAllocation);
            if (!newBlocks) {
                printf("\nPANIC ERROR: Not enough MEMORY! Exiting ...\n");
                std::exit(1);
            }

            BigCell *currentBlock = newBlocks;
            Cell *lastBlock = this->freeCells;
            for (int n = 0; n < this->blocksPerAllocation; n++) {
                // Clear new allocated block
                void **p =(void **) newBlocks;
                for (int count = 0;
                    count < sizeof(BigCell) / sizeof(void *);
                    count++)
                    *p++ = NULL;

                // Chain new blocks as a list
                currentBlock->nextCell = lastBlock;
                lastBlock = currentBlock++;
            }

            this->freeCells = newBlocks;

            this->heapSize += sizeof(BigCell) * this->blocksPerAllocation;
            this->freeCellsCount += this->blocksPerAllocation;
        }

        if (this->freeCells) {
            newBlock = this->freeCells->nextCell;
            this->freeCells = newBlock->nextCell;

            this->freeCellsCount -= 1;
            newBlock->nextCell = newBlocks;
            newBlocks = newBlock;
        }
    }

    this->gcLock.store(FALSE);

    return freeBlocks;
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
