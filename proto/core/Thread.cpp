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

#define BLOCKS_PER_ALLOCATION           1024
#define BLOCKS_PER_MALLOC_REQUEST       8 * BLOCKS_PER_ALLOCATION

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

Cell *ProtoThread::allocCell() {
    Cell *newCell;

    if (!this->freeCells) {
        if (this->state == THREAD_STATE_MANAGED && 
            this->context->space->state != SPACE_STATE_RUNNING) {

            if (this->state != SPACE_STATE_RUNNING && this->state == THREAD_STATE_MANAGED) {
                if (this->space->state != SPACE_STATE_STOPPING_WORLD) {
                    this->state = THREAD_STATE_STOPPING;
                    this->space->stopTheWorldCV.notify_one();

                    // Wait for GC to start to stop the world
                    do {
                        std::unique_lock lk(globalMutex);
                        this->space->restartTheWorldCV.wait(lk);
                    } while (this->space->state != SPACE_STATE_WORLD_TO_STOP);

                    this->state = THREAD_STATE_STOPPED;
                    // Wait for GC to complete
                    do {
                        std::unique_lock lk(globalMutex);
                        this->space->restartTheWorldCV.wait(lk);
                    } while (this->space->state != SPACE_STATE_RUNNING);

                    this->state = THREAD_STATE_MANAGED;
                }
            }
        }

        this->freeCells = (BigCell *) this->space->getFreeCells();

        if (!this->freeCells) {
            // Alloc from OS
            // TODO: Limit the amount of RAM to ask to the OS per space

            BigCell *newBlocks = (BigCell *) malloc(sizeof(BigCell) * BLOCKS_PER_MALLOC_REQUEST);
            if (!newBlocks) {
                printf("\nPANIC ERROR: Not enough MEMORY! Exiting ...\n");
                std::exit(1);
            }

            BigCell *currentBlock = newBlocks;
            Cell *lastBlock = NULL;
            for (int n = 0; n < BLOCKS_PER_MALLOC_REQUEST; n++) {
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
        }
    }

    // Dealloc first free cell
    newCell = this->freeCells;
    this->freeCells = (BigCell *) newCell->nextCell;

    // Add it to current working set
    this->context->lastAllocatedCell = newCell;

    return newCell;
};


};
