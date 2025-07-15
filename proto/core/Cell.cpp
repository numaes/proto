/*
 * Cell.cpp
 *
 *  Created on: 2020-05-01
 *      Author: gamarino
 */


#include "../headers/proto_internal.h"

namespace proto {

Cell::Cell (ProtoContext *context) {
    context->addCell2Context(this);
};

Cell::~Cell() {};

unsigned long Cell::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
}

void Cell::finalize(ProtoContext *context) {};

void *Cell::operator new(unsigned long size, ProtoContext *context) {
    return context->allocCell();
};

// Apply method recursivelly to all referenced objects, except itself
void Cell::processReferences(
    ProtoContext *context, 
    void *self,
    void (*method)(
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {};

};


