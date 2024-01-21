/*
 * Thread.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto internal.h"

#include <stdlib.h>

#include <thread>

using namespace std;
namespace proto {

int a1() {return 0;};

ProtoThreadImplementation::ProtoThreadImplementation(
		ProtoContext *context,

		ProtoString *name,
		ProtoSpace	*space,
		ProtoMethod code,
		ProtoList *args,
		ProtoSparseList *kwargs
) : Cell(context) {
    this->name = name;
    this->space = space;
    this->freeCells = NULL;

    // register the thread in the corresponding space
    this->space->allocThread(context, this);
    this->state = THREAD_STATE_MANAGED;
    this->unmanagedCount = 0;
    this->currentContext = NULL;

    // Create and start the OS Thread if needed (on space init, no code is provided)
    if (code) {
        this->osThread = new std::thread(
            [] (ProtoThreadImplementation *self, 
                ProtoMethod targetCode, 
                ProtoList *threadArgs, 
                ProtoSparseList *threadKwargs) {
                ProtoContext baseContext(NULL, NULL, 0, self, self->space);
                targetCode(
                    NULL, 
                    self->asObject(&baseContext), 
                    self->asObject(&baseContext), 
                    threadArgs, 
                    threadKwargs
                );
            },
            this,
            code,
            args,
            kwargs
        );
    }
    else {
        ProtoContext *mainBaseContext = new ProtoContext(
            NULL, NULL, 0, this, this->space
        );
        this->currentContext = mainBaseContext;
        this->space->mainThreadId = std::this_thread::get_id();
        this->space->mainThread = this;
        this->osThread = NULL;
    }
};

ProtoThreadImplementation::~ProtoThreadImplementation(

) {
    this->osThread->~thread();
};

void ProtoThreadImplementation::setUnmanaged() {
    this->unmanagedCount++;
    this->state = THREAD_STATE_UNMANAGED;
}

void ProtoThreadImplementation::setManaged() {
    if (this->unmanagedCount > 0)
        this->unmanagedCount--;
    if (this->unmanagedCount <= 0)
        this->state = THREAD_STATE_MANAGED;
}

void ProtoThreadImplementation::detach(ProtoContext *context) {
    this->osThread->detach();
};

void ProtoThreadImplementation::join(ProtoContext *context) {
    if (this->osThread)
        this->osThread->join();
};

void ProtoThreadImplementation::exit(ProtoContext *context) {
    if (this->osThread->get_id() == std::this_thread::get_id()) {
        this->space->deallocThread(context, this);
        this->osThread->~thread();
    }
};

void ProtoThreadImplementation::synchToGC() {
    if (this->state == THREAD_STATE_MANAGED && 
        this->space->state != SPACE_STATE_RUNNING) {

        if (this->state != SPACE_STATE_RUNNING && this->state == THREAD_STATE_MANAGED) {
            if (this->space->state != SPACE_STATE_STOPPING_WORLD) {
                this->state = THREAD_STATE_STOPPING;

                this->space->stopTheWorldCV.notify_one();

                // Wait for GC to start to stop the world
                while (this->space->state != SPACE_STATE_WORLD_TO_STOP) {
                    std::unique_lock lk(ProtoSpace::globalMutex);
                    this->space->restartTheWorldCV.wait(lk);
                };

                this->state = THREAD_STATE_STOPPED;

                // Wait for GC to complete
                while (this->space->state != SPACE_STATE_RUNNING) {
                    std::unique_lock lk(ProtoSpace::globalMutex);
                    this->space->restartTheWorldCV.wait(lk);
                };

                this->state = THREAD_STATE_MANAGED;
            }
        }
    }

}

Cell *ProtoThreadImplementation::allocCell() {
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

void ProtoThreadImplementation::finalize(ProtoContext *context) {

};

void ProtoThreadImplementation::processReferences(
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

ProtoObject *ProtoThreadImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_TUPLE;

    return p.oid.oid;
};

unsigned long ProtoThreadImplementation::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};



};
