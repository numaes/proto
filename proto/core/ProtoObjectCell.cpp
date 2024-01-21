/*
 * proto_object.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto internal.h"

namespace proto {

ProtoObjectCellImplementation::ProtoObjectCellImplementation(
	ProtoContext *context,
	ParentLinkImplementation	*parent,
	unsigned long mutable_ref,
	ProtoSparseListImplementation *attributes
) : Cell(context) {
	this->parent = parent;
	this->mutable_ref = mutable_ref;
	this->attributes = attributes;
};

ProtoObjectCellImplementation::~ProtoObjectCellImplementation() {

};

ProtoObjectCellImplementation *ProtoObjectCellImplementation::addParent(
	ProtoContext *context, 
	ProtoObjectCellImplementation *object
) {
	return new(context) ProtoObjectCellImplementation(
		context,
		new(context) ParentLinkImplementation(
			context,
			this->parent,
			object
		)
	);
};

void ProtoObjectCellImplementation::finalize(ProtoContext *context) {};

// Apply method recursivelly to all referenced objects, except itself
void ProtoObjectCellImplementation::processReferences(
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

ProtoObject *ProtoObjectCellImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_OBJECT;

    return p.oid.oid;
};

unsigned long ProtoObjectCellImplementation::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};


};
