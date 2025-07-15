/*
 * parent_link.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

namespace proto {


ParentLinkImplementation::~ParentLinkImplementation() {

};

ParentLinkImplementation::ParentLinkImplementation(
	ProtoContext *context,
	ParentLinkImplementation *parent,
	ProtoObjectCellImplementation *object
) : Cell(context) {
	this->parent = parent;
	this->object = object;
};

void ParentLinkImplementation::processReferences(
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

void ParentLinkImplementation::finalize(ProtoContext *context) {};

};
