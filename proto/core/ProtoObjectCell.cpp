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
	ProtoMutableReference *mutable_ref,
	ProtoSparseList *attributes
) : Cell(context) {
	this->parent = parent;
	this->mutable_ref = mutable_ref;
	this->attributes = attributes;
};

ProtoObjectCell::~ProtoObjectCell() {

};

ProtoObjectCell *ProtoObjectCell::addParent(
	ProtoContext *context, 
	ProtoObjectCell *object
) {
	return new(context) ProtoObjectCell(
		context,
		new(context) ParentLink(
			context,
			this->parent,
			object
		)
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

	if (this->attributes)
		method(context, self, this->attributes);

};

ProtoObject *ProtoObjectCell::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_OBJECT;

    return p.oid.oid;
};

unsigned long ProtoObjectCell::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};


};
