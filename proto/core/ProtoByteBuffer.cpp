/*
 * ProtoByteBuffer.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"


ProtoByteBuffer::ProtoByteBuffer (
    ProtoContext *context,
    char		*buffer,
    unsigned long size
) : Cell (
    context,
    type = CELL_TYPE_BYTE_BUFFER
) {
    this->buffer = buffer;
    this->size = size;
};

ProtoByteBuffer::~ProtoByteBuffer() {

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

