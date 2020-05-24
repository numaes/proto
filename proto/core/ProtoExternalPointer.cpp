/*
 * ProtoExternalPointer.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"


ProtoExternalPointer::ProtoExternalPointer (
    ProtoContext *context,
    void 		 *pointer
) : Cell (
    context,
    type = CELL_TYPE_EXTERNAL_POINTER
) {
    this->pointer = pointer;
};

ProtoExternalPointer::~ProtoExternalPointer() {

};

void ProtoByteBuffer::processReferences(
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

