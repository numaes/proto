/*
 * ProtoByteBuffer.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto.h"

#include <malloc.h>

namespace proto {


ProtoByteBuffer::ProtoByteBuffer (
    ProtoContext *context,
    unsigned long size
) : Cell (
    context,
    type = CELL_TYPE_BYTE_BUFFER
) {
    this->size = size;
    this->buffer = (char *) malloc((size_t) size);
};

ProtoByteBuffer::~ProtoByteBuffer() {
    if (this->buffer)
        free((void *) this->buffer);
    this->buffer = NULL;
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

};
