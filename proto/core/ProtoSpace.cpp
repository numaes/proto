/*
 * core.cpp
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"
#include <malloc.h>
#include <stdio.h>
#include <thread>

#define BLOCKS_PER_ALLOCATION           1024
#define BLOCKS_PER_MALLOC_REQUEST       8 * BLOCKS_PER_ALLOCATION

ProtoSpace::ProtoSpace() {
    Cell *firstCell = this->getFreeCells();
    ProtoThread *firstThread = (ProtoThread *) firstCell;
    firstThread->freeCells = firstCell->nextCell;
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
    
    this->threads = new(creationContext) ProtoSet(creationContext);
    ProtoObject *threadName = creationContext->literalFromString("Main thread");
    firstThread->name = threadName;
    this->threads = creationContext->newMutable();
    ProtoSet *threads = new(creationContext) ProtoSet(creationContext);
    this->threads->setValue(creationContext, threads->add(creationContext, firstThread));
};

ProtoSpace::~ProtoSpace() {
    ProtoContext *finalContext = new ProtoContext(NULL);

    ProtoList *threads = ((ProtoSet *)this->threads)->asList(finalContext);
    int threadCount = threads->getSize(finalContext);

    // Wait till all threads are ended
    for (int i = 0; i < threadCount; i++) {
        ProtoThread *t = (ProtoThread *) threads->getAt(
            finalContext,
            finalContext->fromInteger(i)
        );
        t->join(finalContext);
    }
};

ProtoThread *ProtoSpace::allocThread(ProtoContext *context, ProtoThread *thread) {
    while (this->threadsLock.load.compare_exchange_strong(
        FALSE,
        TRUE
    )) std::this_thread::yield();

    this->threads->setValue(
        context,
        ((ProtoSet *) this->threads->currentValue())->add(context, thread)
    );

    this->threadsLock.store(FALSE);
};

ProtoThread *ProtoSpace::deallocThread(ProtoContext *context, ProtoThread *thread) {
    while (this->threadsLock.load.compare_exchange_strong(
        FALSE,
        TRUE
    )) std::this_thread::yield();

    this->threads->setValue(
        context,
        ((ProtoSet *) this->threads->currentValue())->removeAt(context, thread)
    );

    this->threadsLock.store(FALSE);
};

Cell *ProtoSpace::getFreeCells(){
    Cell *freeBlocks = NULL;
    Cell *newBlocks, *newBlock;
    AllocatedSegment *newSegment;

    for (int i=0; i < BLOCKS_PER_ALLOCATION; i++) {
        if (this->blocksInCurrentSegment <= 0) {
            // Get a new segment from OS
            newBlocks = (Cell *) malloc(sizeof(BigCell) * BLOCKS_PER_MALLOC_REQUEST);
            if (!newBlocks) {
                printf("\nPANIC ERROR: Not enough MEMORY! Exiting ...\n");
                exit(1);
            }

            // Clear new allocated blocks
            void **p =(void **) newBlocks;
            int n = 0;
            while (n < (BLOCKS_PER_MALLOC_REQUEST * sizeof(BigCell) / sizeof(void *)))
                *p++ = NULL;

            newSegment = new AllocatedSegment();
            newSegment->memoryBlock = newBlocks;
            newSegment->cellsCount = BLOCKS_PER_ALLOCATION;
            newSegment->nextBlock = this->segments;

            this->segments = newSegment;
            this->blocksInCurrentSegment = BLOCKS_PER_MALLOC_REQUEST;
        }

        newBlock = &(this->segments->memoryBlock[
            this->segments->cellsCount - this->blocksInCurrentSegment--]);
        new(NULL) Cell(NULL, freeBlocks, CELL_TYPE_UNASSIGNED);

        freeBlocks = newBlock;
    }

    return freeBlocks;
};

void ProtoSpace::analyzeUsedCells(Cell *cellsChain) {
    DirtySegment *newChain;

    newChain = new DirtySegment();
    newChain->cellChain = cellsChain;
    newChain->nextSegment = this->dirtySegments;
    this->dirtySegments = newChain;
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
