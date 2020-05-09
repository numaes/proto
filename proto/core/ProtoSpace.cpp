/*
 * core.cpp
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"
#include <malloc.h>

#define BLOCKS_PER_ALLOCATION           1024
#define BLOCKS_PER_MALLOC_REQUEST       8 * BLOCKS_PER_ALLOCATION


ProtoSpaceImplementation::ProtoSpaceImplementation() {
    Cell *firstCell = this->getFreeCells();
    ProtoThread *firstThread = (ProtoThread *) firstCell;
    firstThread->freeCells = firstCell->nextCell;
    firstThread->currentWorkingSet = firstThread;
    // Get current thread id from OS
    firstThread->osThreadId = 0;
    firstThread->space = this;

    firstCell->nextCell = NULL;

    ProtoContextImplementation *creationContext = new ProtoContextImplementation(
        NULL,
        this,
        firstThread
    );
    
    this->threads = new(creationContext) ProtoSet(creationContext);
    ProtoObject *threadName = creationContext->literalFromString("Main thread");
    firstThread->name = threadName;
    this->threads = (new(creationContext) ProtoSet(creationContext))->add(creationContext, firstThread);
};

ProtoSpaceImplementation::~ProtoSpaceImplementation() {
    ProtoContextImplementation *finalContext = new ProtoContextImplementation(NULL);

    ProtoList *threads = this->threads->asList(finalContext);
    int threadCount = threads->getSize(finalContext);

    // Wait till all threads are ended
    for (int i = 0; i < threadCount; i++) {
        ProtoThread *t = (ProtoThread *) threads->getAt(
            finalContext,
            finalContext->fromInteger(i)
        );
        t->join(finalContext);
    }

    this->deallocMemory();
};


ProtoObject *ProtoSpace::getLiteralFromString(
    ProtoContext *context,
    char *zeroTerminatedUtf8String
) {
    ProtoSpaceImplementation *self = (ProtoSpaceImplementation *) this;

    return literalRoot.load()->getFromZeroTerminatedString(
        context, 
        zeroTerminatedUtf8String
    );
};

ProtoThread *ProtoSpace::getNewThread() {
    ProtoSpaceImplementation *self = (ProtoSpaceImplementation *) this;

    return NULL;
};

ProtoObject *ProtoSpace::getThreads() {
    ProtoSpaceImplementation *self = (ProtoSpaceImplementation *) this;

    return self->threads;
};

Cell *ProtoSpaceImplementation::getFreeCells(){
    Cell *freeBlocks = NULL;
    Cell *newBlocks, *newBlock;
    AllocatedSegment *newSegment;

    for (int i=0; i < BLOCKS_PER_ALLOCATION; i++) {
        if (this->blocksInCurrentSegment <= 0) {
            // Get a new segment from OS
            newBlocks = (Cell *) malloc(sizeof(BigCell) * BLOCKS_PER_MALLOC_REQUEST);
            newSegment = new AllocatedSegment();
            newSegment->memoryBlock = newBlocks;
            newSegment->cellsCount = BLOCKS_PER_ALLOCATION;
            newSegment->nextBlock = this->segments;

            this->segments = newSegment;
            this->blocksInCurrentSegment = BLOCKS_PER_MALLOC_REQUEST;
        }

        newBlock = &(this->segments->memoryBlock[
            this->segments->cellsCount - this->blocksInCurrentSegment--]);
        newBlock->nextCell = freeBlocks;
        freeBlocks = newBlock;
    }

    return freeBlocks;
};

void ProtoSpaceImplementation::analyzeUsedCells(Cell *cellsChain) {
    DirtySegment *newChain;

    newChain = new DirtySegment();
    newChain->cellChain = cellsChain;
    newChain->nextSegment = this->dirtySegments;
    this->dirtySegments = newChain;
};

void ProtoSpaceImplementation::deallocMemory(){
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
