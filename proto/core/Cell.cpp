/*
 * Cell.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"
#include "../headers/proto_internal.h"

void *Cell::operator new(size_t size, ProtoContext *context) {
    return context->allocCell();
};

Cell::Cell (
    ProtoContext *context, 
    unsigned long type = CELL_TYPE_UNASSIGNED_F,
    unsigned long height = 0,
    unsigned long count = 0
) {

};

Cell::~Cell() {
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
    }
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
    }
};



