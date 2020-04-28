/*
 * cell.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "proto.h"
#include "proto_internal.h"

void Object::processReferences(void *callback(Cell *subCell)) {

}

void Object::beforeDeleting() {

}

Object::Object(ProtoSpace *space, ParentLink *newParent, DictNode *currentValue = NULL, int isMutable=PROTO_FALSE) {
	this->space = space,
	this->parent = ParentLink(newParent, NULL);
	this->isMutable = isMutable;
	this->currentValue = currentValue;
}

ProtoObject *Object::clone() {
	return new Object(this->space, this->parent, this->currentValue, this->isMutable);
}

ProtoObject *Object::newChild() {
	Object *newObject = new Object(this->space, ParentLink(this->space, this), DictNode(this->space), this->isMutable);
	return newObject;
}

ProtoObject *Object::getAttribute(ProtoObject *name) {
	if (this->currentValue) {
		Object *currentObject = this;
		ParentLink *currentPL;
		int hasObject;

		do {
			hasObject = currentObject->hasAttribute(name);
			if (!hasObject) {
				currentPL = currentPL->nextInChain;
				if (currentPL)
					currentObject = currentPL->parent;
			}
		} while (!hasObject && currentPL);

		if (hasObject)
			return currentObject->getAttribute(name);
	}

	return PROTO_NONE;
}

ProtoObject *Object::hasAttribute(ProtoObject *name) {
	if (this->currentValue) {
		Object *currentObject = this;
		ParentLink *currentPL;
		int hasObject;

		do {
			if (currentObject->currentValue)
				hasObject = currentObject->currentValue->hasAttribute(name);
			else
				hasObject = FALSE;
			if (!hasObject) {
				currentPL = currentPL->nextInChain;
				if (currentPL)
					currentObject = currentPL->parent;
			}
		} while (!hasObject && currentPL);

		if (hasObject)
			return PROTO_TRUE;
	}

	return PROTO_FALSE;
}

ProtoObject *Object::setAttribute(ProtoObject *name, ProtoObject *value) {
	Object *returnObject;

	if (this->isMutable) {
		returnObject = this;
		returnObject->currentValue = returnObject->currentValue->at(name, value);
	}
	else {
		int mutableMark;
		mutableMark = this->isMutable;
		if (!mutableMark && value)
			mutableMark = value->isMutable();
		returnObject = new Object(this->parent, TreeNode(value), mutableMark);
	}
	return returnObject;
}

ProtoObject *Object::getAttributes() {
	return PROTO_NONE;
}

ProtoObject *Object::getParents() {
	return PROTO_NONE;
}

ProtoObject *Object::addParent(ProtoObject *newParent) {
	Object *returnObject;

	if (this->isMutable) {
		this->parent = new ParentLink(this->parent, newParent);
		return this;
	}
	else
		return new Object(new ParentLink(this->parent, newParent),
						  this->currentValue,
						  this->isMutable);
}

ProtoObject *Object::getHash() {
	return NULL;
}

ProtoObject *Object::isInstanceOf(ProtoObject *prototype) {

}

ProtoObject *Object::isMutable() {
	return this->isMutable;
}

ProtoObject *Object::call(ProtoObject *method,
						  ProtoObject *unnamedParameters,
					      ProtoObject *keywordParameters) {

}

