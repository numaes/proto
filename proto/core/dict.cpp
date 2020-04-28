/*
 * dict.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "proto.h"
#include "proto_internal.h"

DictNode::DictNode() {
	this->TreeNode(NULL);
	this->value = PROTO_NONE;
}

void DictNode::processReferences(void *callback(Cell *subCell)) {

}

void DictNode::beforeDeleting() {

}

TreeNode *DictNode::clone() {
	DictNode *newNode = DictNode();

	newNode->TreeNode(this->key, this->previous, this->next);
	newNode->value = this->value;

	return newNode;
}

ProtoObject *DictNode::at(ProtoObject *key, ProtoObject *newValue) {
	DictNode *item = (DictNode *) this->get(key);

	if (item)
		return item->value;
	else
		return PROTO_NONE;
}

void addKey(ListNode *l, DictNode *n) {
	if (n->previous)
		addKey(l, (DictNode *) n->previous);

	if (n->key)
		l->add(n->key);

	if (n->next)
		addKey(l, (DictNode *) n->next);
}

ProtoObject *DictNode::keys() {
	ListNode *l = ListNode();

	addKey(l, this);

	return l;
}






