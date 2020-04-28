/*
 * parent_link.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "proto.h"
#include "proto_internal.h"

ParentLink::~ParentLink() {

}

ParentLink::ParentLink(ParentLink *currentParent, ProtoObject *newParent) {
	this->parent = newParent;
	this->nextInChain = currentParent;
}

virtual void ParentLink::processReferences(void *callback(Cell *subCell)) {

}

virtual void ParentLink::beforeDeleting() {

}



