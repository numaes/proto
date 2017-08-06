/*
 * proto_internal.h
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#ifndef PROTO_INTERNAL_H_
#define PROTO_INTERNAL_H_

#include "proto.h"

class ProtoMM;

extern ProtoMM *mm;

class Cell: ProtoObject {
public:
	virtual ~Cell();

	virtual void processReferences(void *callback(Cell *subCell));
	virtual void beforeDeleting();
};

class ProtoMM {
public:
	void ProtoMM();
	virtual void ~ProtoMM();

	virtual Cell *allocCell();
	virtual void threadCapture();
	virtual void addReference(Cell *from, Cell *to);

};

class TreeNode: Cell {
public:
	virtual ~TreeNode();

	TreeNode(ProtoObject *key, TreeNode *previous = NULL, TreeNode *next = NULL);

	TreeNode	*previous;
	TreeNode	*next;
	ProtoObject *key;

	int			count;
	int			height;

	virtual void        processReferences(void *callback(Cell *subCell));
	virtual void        beforeDeleting();
	virtual TreeNode    *clone();

	virtual TreeNode	*get(ProtoObject *key);
	virtual int         has(ProtoObject *key);
	virtual TreeNode    *add(ProtoObject *key);
	virtual TreeNode    *remove(ProtoObject *key);
	virtual int         *size();

};

class DictNode: TreeNode {
public:
	virtual ~DictNode();

	DictNode();

	ProtoObject			*value;

	virtual void        processReferences(void *callback(Cell *subCell));
	virtual void        beforeDeleting();
	virtual TreeNode    *clone();

	virtual ProtoObject *at(ProtoObject *key, ProtoObject *newValue);
	virtual ProtoObject *keys();
};

class SetNode: TreeNode {
public:
	virtual ~SetNode();

	SetNode();

	virtual void        processReferences(void *callback(Cell *subCell));
	virtual void        beforeDeleting();
	virtual TreeNode    *clone();

	virtual ProtoObject *add(ProtoObject *newValue);
	virtual ProtoObject *has(ProtoObject *value);
	virtual ProtoObject *remove(ProtoObject *value);
	virtual ProtoObject *size();
	virtual ProtoObject *values();
};

class ListNode: TreeNode {
public:
	virtual ~ListNode();

	ListNode();

	virtual void        processReferences(void *callback(Cell *subCell));
	virtual void        beforeDeleting();
	virtual TreeNode    *clone();

	virtual TreeNode    *get(ProtoObject *key);
	virtual TreeNode    *has(ProtoObject *key);
	virtual TreeNode    *set(ProtoObject *key, ProtoObject *value);
	virtual TreeNode    *remove(ProtoObject *key);

	virtual ProtoObject *at(ProtoObject *index);
	virtual ProtoObject *atSet(ProtoObject *index, ProtoObject *value);
	virtual ProtoObject *keys();
	virtual ProtoObject *size();
	virtual ProtoObject *getFirst();
	virtual ProtoObject *getLast();

	virtual ProtoObject *removeFirst();
	virtual ProtoObject *removeLast();
	virtual ProtoObject *removeAt(ProtoObject *index);
	virtual ProtoObject *removeSlice(ProtoObject *indexFrom, ProtoObject *indexTo);

	virtual ProtoObject *addFirst();
	virtual ProtoObject *addLast();
	virtual ProtoObject *insertAt(ProtoObject *index, ProtoObject *value);
	virtual ProtoObject *insertTreeAt(ProtoObject *index, TreeNode *tree);

	virtual ProtoObject	*slice(ProtoObject *from, ProtoObject *to);

	virtual ProtoObject	*extendFirst(ProtoObject *otherTree);
	virtual ProtoObject	*extendLast(ProtoObject *otherTree);
};

class ParentLink: Cell {
public:
	virtual ~ParentLink();

	ParentLink(ParentLink *currentParent, ProtoObject *newParent);

	virtual void processReferences(void *callback(Cell *subCell));
	virtual void beforeDeleting();

	ProtoObject *parent;
	ParentLink  *nextInChain;
};

class Object: Cell {
public:
	virtual ~Object();

	virtual void processReferences(void *callback(Cell *subCell));
	virtual void beforeDeleting();

	ParentLink  *parent;
	DictNode    *currentValue;
	BOOLEAN     isMutant;

	Object(Object *newParent, DictNode *currentValue = NULL, BOOLEAN isMutant=PROTO_FALSE);

	virtual ProtoObject *clone();
	virtual ProtoObject *newChild();
	virtual ProtoObject *getAttribute(ProtoObject *name);
	virtual ProtoObject *hasAttribute(ProtoObject *name);
	virtual ProtoObject *setAttribute(ProtoObject *name, ProtoObject *value);
	virtual ProtoObject *getAttributes();
	virtual ProtoObject *getParents();
	virtual ProtoObject *addParent(ProtoObject *newParent);
	virtual ProtoObject *getHash();
	virtual ProtoObject *isInstanceOf(ProtoObject *prototype);
	virtual ProtoObject *isModifiable();
	virtual ProtoObject *activate(ProtoObject *unnamedParameters,
			              ProtoObject *keywordParameters);
};

#endif /* PROTO_INTERNAL_H_ */
