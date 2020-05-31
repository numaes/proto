/*
 * ProtoMutableReference.cpp
 *
 *  Created on: 2020-05-30
 *      Author: gamarino
 */


#include "../headers/proto.h"

namespace proto {

ProtoMutableReference::ProtoMutableReference (
    ProtoContext *context, 
    ProtoObject *reference
) : Cell (
    context,
    type = CELL_TYPE_MUTABLE_REFERENCE
) {
    this->reference = reference;
};

ProtoMutableReference::~ProtoMutableReference() {

};

// Apply method recursivelly to all referenced objects, except itself
void ProtoMutableReference::processReferences(
    ProtoContext *context, 
    void *self,
    void (*method)(
        ProtoContext *context, 
        void *self,
        Cell *cell
    )
) {
    method(context, self, (Cell *) this->reference);
    method(context, self, this);
};

};
