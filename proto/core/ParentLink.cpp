/*
 * parent_link.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"

ParentLink::~ParentLink() {

}

ParentLink::ParentLink(
	ProtoContext *context,
	ParentLink *parent,
	ProtoObject *object
) : Cell(
	context,
	type = CELL_TYPE_PARENT_LINK
) {
	this->parent = parent;
	this->object = object;
};

void ParentLink::processReferences(
	ProtoContext *context, 
	void *self,
	void (*method)(
		ProtoContext *context, 
		void *self,
		Cell *cell
	)
) {
	if (this->parent)
		method(context, self, parent);
	
	method(context, self, this);
}


