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

		ProtoString *name,
		ProtoSpace	*space,
		ProtoMethod *code,
		ProtoObject *args,
		ProtoObject *kwargs
) : Cell(context) {
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

void ProtoThread::synchToGC() {
    if (this->state == THREAD_STATE_MANAGED && 
        this->space->state != SPACE_STATE_RUNNING) {

        if (this->state != SPACE_STATE_RUNNING && this->state == THREAD_STATE_MANAGED) {
            if (this->space->state != SPACE_STATE_STOPPING_WORLD) {
                this->state = THREAD_STATE_STOPPING;

                this->space->stopTheWorldCV.notify_one();

                // Wait for GC to start to stop the world
                while (this->space->state != SPACE_STATE_WORLD_TO_STOP) {
                    std::unique_lock lk(globalMutex);
                    this->space->restartTheWorldCV.wait(lk);
                };

                this->state = THREAD_STATE_STOPPED;

                // Wait for GC to complete
                while (this->space->state != SPACE_STATE_RUNNING) {
                    std::unique_lock lk(globalMutex);
                    this->space->restartTheWorldCV.wait(lk);
                };

                this->state = THREAD_STATE_MANAGED;
            }
        }
    }

}

Cell *ProtoThread::allocCell() {
    Cell *newCell;

    if (!this->freeCells) {
        this->synchToGC();
        this->freeCells = (BigCell *) this->space->getFreeCells(this);
    }

    // Dealloc first free cell
    newCell = this->freeCells;
    this->freeCells = (BigCell *) newCell->nextCell;

    return newCell;
};

void ProtoThread::finalize(ProtoContext *context) {

};

void ProtoThread::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    this->name->processReferences(context, self, method);
};

ProtoObject *ProtoThread::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_TUPLE;

    return p.oid.oid;
};

unsigned long ProtoThread::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};



};
