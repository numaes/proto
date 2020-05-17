/*
 * Thread.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"
#include "../headers/proto_internal.h"
#include <thread>

ProtoThread::ProtoThread(
		ProtoContext *context,

		ProtoObject *name,
		ProtoSpace	*space,
		ProtoMethod *code = NULL,
		ProtoObject *args = NULL,
		ProtoObject *kwargs = NULL
) : Cell(context, nextCell, CELL_TYPE_PROTO_THREAD) {

    this->name = name;
    this->space = space;
    this->currentWorkingSet = NULL;
    this->freeCells = NULL;
    ProtoContext *newContext = new ProtoContext(
        NULL, 
        context->space,
        this
    );

    // Copy parameters to new context to avoid loosing the references
    ProtoObjectPointer p;
    BigCell *bc;

    p.oid = args;
    if (args && p.op.pointer_tag == POINTER_TAG_CELL) {
        bc = (BigCell *) newContext->allocCell();
        *bc = *((BigCell *) args);
    }

    p.oid = kwargs;
    if (kwargs && p.op.pointer_tag == POINTER_TAG_CELL) {
        bc = (BigCell *) newContext->allocCell();
        *bc = *((BigCell *) kwargs);
    }

    // register the thread in the corresponding space
    this->space->allocThread(newContext, this);

    // Create and start the OS Thread
    this->osThread = new std::thread(
        (void (*)(ProtoContext *, ProtoObject *, ProtoObject *)) code, 
        newContext, 
        args, 
        kwargs
    );
};

ProtoThread::~ProtoThread(

) {
    this->osThread->~thread();
};

void ProtoThread::detach(ProtoContext *context) {
    this->osThread->detach();
};

void ProtoThread::join(ProtoContext *context) {
    this->osThread->join();
};

void ProtoThread::exit(ProtoContext *context) {
    if (this->osThread->get_id() == std::this_thread::get_id()) {
        this->space->deallocThread(context, this);
        this->space->analyzeUsedCells(this->currentWorkingSet);
        this->osThread->~thread();
    }
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

