/*
 * proto_object.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {

ProtoObjectCell::ProtoObjectCell(
	ProtoContext *context,
	ParentLink	*parent,
	IdentityDict *attributes
) : Cell(
	context,
	type = CELL_TYPE_PROTO_OBJECT
) {
	this->parent = parent;
	this->attributes = attributes;
};

ProtoObjectCell::~ProtoObjectCell() {

};

ProtoObject *ProtoObjectCell::addParent(
	ProtoContext *context, 
	ProtoObject *object
) {
	return new(context) ProtoObjectCell(
		context,
		new(context) ParentLink(
			context,
			this->parent,
			(ProtoObjectCell *) object
		),
		this->attributes
	);
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

};
