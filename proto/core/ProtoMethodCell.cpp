/*
 * parent_link.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {


ProtoMethodCell::~ProtoMethodCell() {

}

ProtoMethodCell::ProtoMethodCell(
    ProtoContext *context,
    ProtoObject  *self,
    ProtoMethod	 method
) : Cell(context) {
    this->self = self;
    this->method = method;
};

void ProtoMethodCell::processReferences(
	ProtoContext *context, 
	void *self,
	void (*method)(
		ProtoContext *context, 
		void *self,
		Cell *cell
	)
) {

}

void ProtoMethodCell::finalize(ProtoContext *context) {};

ProtoObject *ProtoMethodCell::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_METHOD_CELL;

    return p.oid.oid;
};

unsigned long ProtoMethodCell::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

};

