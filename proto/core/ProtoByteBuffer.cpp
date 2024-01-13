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
    unsigned long size,
    char *buffer
) : Cell (context) {
    this->size = size;
    this->buffer = buffer;
};

ProtoByteBuffer::~ProtoByteBuffer() {
    if (this->buffer)
        free((void *) this->buffer);
    this->buffer = NULL;
};

char ProtoByteBuffer::getAt(ProtoContext *context, int index) {
    if (index < 0)
        index = this->size + index;

    if (index >= this->size)
        index = this->size - 1;

    if (index < 0)
        index = 0;

    if (index < size)
        return this->buffer[index];
    else
        return 0;
}

void ProtoByteBuffer::setAt(ProtoContext *context, int index, char value) {
    if (index < 0)
        index = this->size + index;

    if (index >= this->size)
        index = this->size - 1;

    if (index < 0)
        index = 0;

    if (index < size)
        this->buffer[index] = value;

}

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

ProtoObject *ProtoByteBuffer::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_BYTE_BUFFER;

    return p.oid.oid;
};

unsigned long ProtoByteBuffer::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};


};
