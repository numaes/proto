/*
 * Thread.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"

#include <stdlib.h>

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
    this->state = THREAD_STATE_MANAGED;
    this->unmanagedCount = 0;

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

void ProtoThread::setUnmanaged() {
    this->unmanagedCount++;
    this->state = THREAD_STATE_UNMANAGED;
}

void ProtoThread::setManaged() {
    if (this->unmanagedCount > 0)
        this->unmanagedCount--;
    if (this->unmanagedCount <= 0)
        this->state = THREAD_STATE_MANAGED;
}

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

void synchToGC(ProtoThread *thread) {
    if (thread->state == THREAD_STATE_MANAGED && 
        thread->context->space->state != SPACE_STATE_RUNNING) {

        if (thread->state != SPACE_STATE_RUNNING && thread->state == THREAD_STATE_MANAGED) {
            if (thread->space->state != SPACE_STATE_STOPPING_WORLD) {
                thread->state = THREAD_STATE_STOPPING;

                thread->space->stopTheWorldCV.notify_one();

                // Wait for GC to start to stop the world
                while (thread->space->state != SPACE_STATE_WORLD_TO_STOP) {
                    std::unique_lock lk(globalMutex);
                    thread->space->restartTheWorldCV.wait(lk);
                };

                thread->state = THREAD_STATE_STOPPED;

                // Wait for GC to complete
                while (thread->space->state != SPACE_STATE_RUNNING) {
                    std::unique_lock lk(globalMutex);
                    thread->space->restartTheWorldCV.wait(lk);
                };

                thread->state = THREAD_STATE_MANAGED;
            }
        }
    }

}

Cell *ProtoThread::allocCell() {
    Cell *newCell;

    if (!this->freeCells) {
        synchToGC(this);
        this->freeCells = (BigCell *) this->space->getFreeCells();
    }

    this->context->checkCellsCount();

    // Dealloc first free cell
    newCell = this->freeCells;
    this->freeCells = (BigCell *) newCell->nextCell;

    // Add it to current working set
    this->context->lastAllocatedCell = newCell;

    return newCell;
};


};
