/*
 * Thread.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"

#include <thread>

using namespace std;
namespace proto {

ProtoThread::ProtoThread(
		ProtoContext *context,

		ProtoObject *name,
		ProtoSpace	*space,
		ProtoMethod *code,
		ProtoObject *args,
		ProtoObject *kwargs
) : Cell(
    context, 
    type = CELL_TYPE_PROTO_THREAD
) {
    this->name = name;
    this->space = space;
    this->freeCells = NULL;

    // register the thread in the corresponding space
    this->space->allocThread(context, this);

    // Create and start the OS Thread
    this->osThread = new std::thread(
        (void (*)(ProtoContext *, ProtoObject *, ProtoObject *)) code, 
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
    if (this->osThread)
        this->osThread->join();
};

void ProtoThread::exit(ProtoContext *context) {
    if (this->osThread->get_id() == std::this_thread::get_id()) {
        this->space->deallocThread(context, this);
        this->osThread->~thread();
    }
};

Cell *ProtoThread::allocCell() {
    Cell *newCell;

    if (!this->freeCells) {
        this->freeCells = (BigCell *) this->space->getFreeCells();
    }

    // Dealloc first free cell
    newCell = this->freeCells;
    this->freeCells = (BigCell *) newCell->nextCell;

    // Add it to current working set
    newCell->nextCell =this->context->lastAllocatedCell;
    this->context->lastAllocatedCell = newCell;

    return newCell;
};


};
