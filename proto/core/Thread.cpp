/*
 * Thread.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"
#include "../headers/proto_internal.h"

ProtoThread::ProtoThread(
		ProtoContext *context,

		ProtoObject *name = NULL,
		Cell		*currentWorkingSet = NULL,
		Cell		*freeCells = NULL,
		ProtoSpaceImplementation	*space = NULL
) : Cell(context, nextCell, CELL_TYPE_PROTO_THREAD) {

    this->name = name;
    this->currentWorkingSet = NULL;
    this->freeCells = freeCells;
    this->space = space;

    this->osThreadId = 0;
};

ProtoThread::~ProtoThread(

) {
    // os kill(this->osThreadId)
};

void ProtoThread::run(
    ProtoContext *context=NULL, 
    ProtoObject *parameter=NULL, 
    ProtoMethod *code=NULL
) {
    if (!run) {
        // this is the main thread
        // Set osThreadId to current thread, and that is all
        // So allways the osThreadId is real
    }
    else {
        // It is a real new thread
        // Create and run the new thread for the same space
    }
}

ProtoObject	*ProtoThread::end(
    ProtoContext *context, 
    ProtoObject *exitCode
) {
    if (this->osThreadId) {
        // Kill the thread
    }
};

ProtoObject *ProtoThread::join(
    ProtoContext *context
) {

};

void ProtoThread::kill(
    ProtoContext *context
) {

};

void ProtoThread::suspend(
    long ms
) {

};

long ProtoThread::getCPUTimestamp() {
    return 0;
};

Cell *ProtoThread::allocCell() {
    Cell *newCell;

    if (!this->freeCells) {
        this->freeCells = this->space->getFreeCells();
    }

    // Dealloc first free cell
    newCell = this->freeCells;
    this->freeCells = newCell->nextCell;

    // Add it to current working set
    newCell->nextCell = this->currentWorkingSet;
    this->currentWorkingSet = newCell;

    return newCell;
};

Cell *ProtoThread::getLastAllocatedCell() {
    return this->currentWorkingSet;
};

void ProtoThread::setLastAllocatedCell(
    Cell *someCell
) {
    this->currentWorkingSet = someCell;
};

