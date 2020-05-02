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

// Apply method recursivelly to all referenced objects, except itself
void Cell::processReferences(
    ProtoContext *context, 
    void (*method)(ProtoContext *context, Cell *cell)
) {

};

ProtoObject *Cell::getHash(ProtoContext *context) {

};

Cell *Cell::clone(){

};

