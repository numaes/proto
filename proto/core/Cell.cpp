/*
 * Cell.cpp
 *
 *  Created on: 2020-05-01
 *      Author: gamarino
 */


#include "../headers/proto.h"

namespace proto {

Cell::Cell (
    ProtoContext *context, 
    long unsigned type,
    long unsigned height,
    long unsigned count
) {
    this->context = context;
    this->type = type;
    this->height = height;
    this->count = count;
};

Cell::~Cell() {

};

void Cell::finalize() {
    switch (this->type) {
        case CELL_TYPE_BYTE_BUFFER:
            ((ProtoByteBuffer *) this)->~ProtoByteBuffer();
            break;

        case CELL_TYPE_IDENTITY_DICT:
            ((IdentityDict *) this)->~IdentityDict();
            break;

        case CELL_TYPE_PARENT_LINK:
            ((ParentLink *) this)->~ParentLink();
            break;

        case CELL_TYPE_PROTO_LIST:
            ((ProtoList *) this)->~ProtoList();
            break;

        case CELL_TYPE_PROTO_SET:
            ((ProtoSet *) this)->~ProtoSet();
            break;

        case CELL_TYPE_PROTO_THREAD:
            ((ProtoThread *) this)->~ProtoThread();
            break;

        case CELL_TYPE_PROTO_OBJECT:
            ((ProtoObjectCell *) this)->~ProtoObjectCell();
            break;

        case CELL_TYPE_EXTERNAL_POINTER:
            ((ProtoExternalPointer *) this)->~ProtoExternalPointer();
            break;

        case CELL_TYPE_METHOD:
            ((ProtoMethodCell *) this)->~ProtoMethodCell();
            break;

        case CELL_TYPE_MUTABLE_REFERENCE:
            ((ProtoMutableReference *) this)->~ProtoMutableReference();
            break;

    }
};

void *Cell::operator new(size_t size, ProtoContext *context) {
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
) {
    switch (this->type) {
        case CELL_TYPE_BYTE_BUFFER:
            ((ProtoByteBuffer *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_IDENTITY_DICT:
            ((IdentityDict *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_PARENT_LINK:
            ((ParentLink *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_PROTO_LIST:
            ((ProtoList *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_PROTO_SET:
            ((ProtoSet *) this)->processReferences(
                context, 
                self,
                method
            );
            break;

        case CELL_TYPE_PROTO_THREAD:
            ((ProtoThread *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_PROTO_OBJECT:
            ((ProtoObjectCell *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_EXTERNAL_POINTER:
            ((ProtoExternalPointer *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_METHOD:
            ((ProtoMethodCell *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;

        case CELL_TYPE_MUTABLE_REFERENCE:
            ((ProtoMutableReference *) this)->processReferences(
                context, 
                self, 
                method
            );
            break;
    }
};

};


