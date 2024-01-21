/*
 * ProtoExternalPointerImplementation.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto internal.h"

namespace proto {

ProtoExternalPointerImplementation::ProtoExternalPointerImplementation (
    ProtoContext *context,
    void 		 *pointer
) : Cell (context) {
    this->pointer = pointer;
};

ProtoExternalPointerImplementation::~ProtoExternalPointerImplementation() {

};

void ProtoExternalPointerImplementation::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {

};

ProtoObject *ProtoExternalPointerImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_EXTERNAL_POINTER;

    return p.oid.oid;
};

unsigned long ProtoExternalPointerImplementation::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

void ProtoExternalPointerImplementation::finalize(ProtoContext *context) {};

};
