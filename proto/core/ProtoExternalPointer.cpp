/*
 * ProtoExternalPointer.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {

ProtoExternalPointer::ProtoExternalPointer (
    ProtoContext *context,
    void 		 *pointer
) : Cell (context) {
    this->pointer = pointer;
};

ProtoExternalPointer::~ProtoExternalPointer() {

};

void ProtoExternalPointer::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    method(context, self, this);
};

ProtoObject *ProtoExternalPointer::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_EXTERNAL_POINTER;

    return p.oid.oid;
};

unsigned long ProtoExternalPointer::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

};
