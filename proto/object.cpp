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

Object::Object(Object *newParent, DictNode *currentValue = NULL, BOOLEAN isMutant=PROTO_FALSE) {
	this->parent = newParent;
	this->isMutant = isMutant;
	this->currentValue = currentValue;
}

ProtoObject *Object::clone() {
	return new Object(this->parent, this->currentValue, this->isMutant);
}

ProtoObject *Object::newChild() {
	Object *newObject = new Object(this->parent, NULL, this->isMutant);
	return newObject;
}

ProtoObject *Object::getAttribute(ProtoObject *name) {
	if (this->currentValue) {
		Object *currentObject = this;
		ParentLink *currentPL;
		BOOLEAN hasObject;

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
		BOOLEAN hasObject;

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

	if (this->isMutant) {
		returnObject = this;
		returnObject->currentValue = returnObject->currentValue->at(name, value);
	}
	else {
		BOOLEAN mutant = this->isMutant;
		if (!mutant && value)
			mutant = value->isMutable();
		returnObject = new Object(this->parent, TreeNode(value), mutant);
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

	if (this->isMutant) {
		this->parent = new ParentLink(this->parent, newParent);
		return this;
	}
	else
		return new Object(new ParentLink(this->parent, newParent),
						  this->currentValue,
						  this->isMutant);
}

ProtoObject *Object::getHash() {
	return NULL;
}

ProtoObject *Object::isInstanceOf(ProtoObject *prototype) {

}

ProtoObject *Object::isModifiable() {
	return this->isMutant;
}

ProtoObject *Object::activate(ProtoObject *unnamedParameters,
					          ProtoObject *keywordParameters) {

}

