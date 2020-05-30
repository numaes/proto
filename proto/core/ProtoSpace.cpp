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

using namespace std;

namespace proto {


#define BLOCKS_PER_ALLOCATION           1024
#define BLOCKS_PER_MALLOC_REQUEST       8 * BLOCKS_PER_ALLOCATION

#define SPACE_STATE_RUNNING             0
#define SPACE_STATE_ENDING              1

#define GC_SLEEP_MILLISECONDS           1000

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

    // Convert the dirtyList into the list of cells to analyze
    // Acquire mutables

    BOOLEAN oldValue = FALSE;
    while (space->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    toAnalize = space->dirtySegments;
    space->dirtySegments = NULL;

    space->gcLock.store(FALSE);

    ProtoContext gcContext(context); 

    ProtoSet *cellSet = new(&gcContext) ProtoSet(&gcContext);

    ((IdentityDict *) space->mutableRoot.load())->processValues(
        &gcContext,
        &cellSet,
        gcCollectObjects
    );

    // Collect all roots
    
    Cell *freeBlocks = NULL;
    int freeCount = 0;
    while (toAnalize) {
        Cell *block = toAnalize->cellChain;

        while (block) {
            Cell *nextCell = block->nextCell;
            if (!cellSet->has(&gcContext, (ProtoObject *) block)) {
                block->~Cell();

                void **p = (void **) block;
                for (unsigned i; i < sizeof(BigCell) / sizeof(void *); i++)
                    *p++ = NULL;

                block->nextCell = freeBlocks;
                freeBlocks = block;
                freeCount++;
            }
            block = nextCell;
        }

        DirtySegment *nextToAnalize = toAnalize->nextSegment;
        delete toAnalize;
        toAnalize = nextToAnalize;
    }

    oldValue = FALSE;
    while (space->gcLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    DirtySegment *newList = new DirtySegment;
    newList->nextSegment = space->freeSegments;
    newList->cellChain = (BigCell *) freeBlocks;

    space->freeSegments = newList;

    space->gcLock.store(FALSE);
}

void gcThreadLoop(ProtoSpace *space) {
    ProtoContext gcContext;

    while (space->state == SPACE_STATE_RUNNING) {
        if (space->dirtySegments) {
            gcScan(&gcContext, space);
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(GC_SLEEP_MILLISECONDS));
    }

    std::this_thread::yield();
}

ProtoSpace::ProtoSpace() {
    Cell *firstCell = this->getFreeCells();
    ProtoThread *firstThread = (ProtoThread *) firstCell;
    firstThread->freeCells = (BigCell *) firstCell->nextCell;
    firstThread->nextCell = NULL;
    firstThread->currentWorkingSet = firstThread;
    // Get current thread id from OS
    firstThread->osThread = NULL;
    firstThread->space = this;

    firstCell->nextCell = NULL;

    ProtoContext *creationContext = new ProtoContext(
        NULL,
        this,
        firstThread
    );
    this->creationContext = creationContext;

    this->mainThreadId = std::this_thread::get_id();

    ProtoObject *threadName = creationContext->literalFromUTF8String((char *) "Main thread");
    firstThread->name = threadName;
    this->threads = new(creationContext) IdentityDict(
        creationContext,
        threadName,
        firstThread
    );

    this->mutableLock.store(FALSE);
    this->threadsLock.store(FALSE);
    this->gcLock.store(FALSE);
    this->mutableRoot.store(new(creationContext) IdentityDict(creationContext));
    this->gcThread = new std::thread(
        (void (*)(ProtoSpace *)) (&gcThreadLoop), 
        this
    );
};

void scanThreads(ProtoContext *context, void *self, ProtoObject *value) {
    ProtoList **threadList = (ProtoList **) self;

    *threadList = (*threadList)->appendLast(context, value);
}

ProtoSpace::~ProtoSpace() {
    ProtoContext finalContext(NULL, this);

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

    ProtoSet *currentSet = (ProtoSet *) this->threads->currentValue(context);
    this->threads->setValue(
        context,
        currentSet,
        currentSet->add(context, thread)
    );

    this->threadsLock.store(FALSE);

};

void ProtoSpace::deallocThread(ProtoContext *context, ProtoThread *thread) {
    BOOLEAN oldValue = FALSE;
    while (this->threadsLock.compare_exchange_strong(
        oldValue,
        TRUE
    )) std::this_thread::yield();

    ProtoSet *currentSet = (ProtoSet *) this->threads->currentValue(context);
    this->threads->setValue(
        context,
        currentSet,
        currentSet->removeAt(context, thread)
    );

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
        if (this->freeSegments) {
            Cell *nextBlock = this->freeSegments->cellChain->nextCell;
            newBlock = this->freeSegments->cellChain;
            if (nextBlock)
                this->freeSegments->cellChain = (BigCell *) nextBlock;
            else {
                this->freeSegments = this->freeSegments->nextSegment;
            }

            void **p =(void **) newBlock;
            unsigned n = 0;
            while (n < (sizeof(BigCell) / sizeof(void *)))
                *p++ = NULL;
        }    
        else {
            if (this->blocksInCurrentSegment <= 0) {
                // Get a new segment from OS
                newBlocks = (Cell *) malloc(sizeof(BigCell) * BLOCKS_PER_MALLOC_REQUEST);
                if (!newBlocks) {
                    printf("\nPANIC ERROR: Not enough MEMORY! Exiting ...\n");
                    exit(1);
                }

                // Clear new allocated blocks
                void **p =(void **) newBlocks;
                unsigned n = 0;
                while (n++ < (BLOCKS_PER_MALLOC_REQUEST * sizeof(BigCell) / sizeof(void *)))
                    *p++ = NULL;

                newSegment = new AllocatedSegment();
                newSegment->memoryBlock = (BigCell *) newBlocks;
                newSegment->cellsCount = BLOCKS_PER_MALLOC_REQUEST;
                newSegment->nextBlock = this->segments;

                this->segments = newSegment;
                this->blocksInCurrentSegment = BLOCKS_PER_MALLOC_REQUEST;
            }

            newBlock = &(this->segments->memoryBlock[
                this->segments->cellsCount - this->blocksInCurrentSegment--]);
        }

        freeBlocks = newBlock;
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

void ProtoSpace::deallocMemory(){
    AllocatedSegment *nextSegment, *currentSegment = this->segments;

    while (currentSegment) {
        if (currentSegment->memoryBlock)
            free(currentSegment->memoryBlock);
        nextSegment = currentSegment->nextBlock;
        free(currentSegment);
        currentSegment = nextSegment;
    }
    this->segments = NULL;
    this->blocksInCurrentSegment = 0;
};

};
