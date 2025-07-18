/*
 * ProtoByteBufferImplementation.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

#include <malloc.h>

namespace proto {


ProtoByteBufferImplementation::ProtoByteBufferImplementation (
    ProtoContext *context,
    unsigned long size,
    char *buffer
) : Cell (context) {
    this->size = size;
    this->freeOnExit = false;
    if (buffer)
        this->buffer = buffer;
    else {
        this->buffer = (char *) malloc(size);
        this->freeOnExit = true;
    }
};

ProtoByteBufferImplementation::~ProtoByteBufferImplementation() {
    if (this->buffer and this->freeOnExit)
        free((void *) this->buffer);
    this->buffer = NULL;
};

char ProtoByteBufferImplementation::getAt(ProtoContext *context, int index) {
    if (index < 0)
        index = this->size + index;

    if (index >= (int) this->size)
        index = this->size - 1;

    if (index < 0)
        index = 0;

    if (index < (int) this->size)
        return this->buffer[index];
    else
        return 0;
}

void ProtoByteBufferImplementation::setAt(ProtoContext *context, int index, char value) {
    if (index < 0)
        index = this->size + index;

    if (index >= (int) this->size)
        index = this->size - 1;

    if (index < 0)
        index = 0;

    if (index < (int) this->size)
        this->buffer[index] = value;

}

void ProtoByteBufferImplementation::processReferences(
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

ProtoObject *ProtoByteBufferImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_BYTE_BUFFER;

    return p.oid.oid;
};

unsigned long ProtoByteBufferImplementation::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

void ProtoByteBufferImplementation::finalize(ProtoContext *context) {};

};
