/*
 * proto_object.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"

ProtoObjectCell::ProtoObjectCell(
	ProtoContext *context,
	ParentLink	*parent = NULL,
	IdentityDict *attributes = NULL
) : Cell(
	context,
	type = CELL_TYPE_PROTO_OBJECT
) {

};

ProtoObjectCell::~ProtoObjectCell() {

};

ProtoObject *ProtoObjectCell::addParent(
	ProtoContext *context, 
	ProtoObject *object
) {

};

// Apply method recursivelly to all referenced objects, except itself
void ProtoObjectCell::processReferences(
	ProtoContext *context, 
	void *self,
	void (*method)(
		ProtoContext *context, 
		void *self,
		Cell *cell
	)
) {
	if (this->parent)
		method(context, self, this->parent);

	method(context, self, this);
};

