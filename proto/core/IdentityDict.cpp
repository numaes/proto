/*
 * IdentityDict.cpp
 *
 *  Created on: 2017-05-01
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {

#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

IdentityDict::IdentityDict(
	ProtoContext *context,

	ProtoObject *key,
	ProtoObject *value,
	IdentityDict *previous,
	IdentityDict *next
) : Cell(
	context,
	type = CELL_TYPE_IDENTITY_DICT,
    height = 1 + max(previous? previous->height : 0, next? next->height : 0),
    count = (key? 1: 0) + (previous? previous->count : 0) + (next? next->count : 0)
) {
	this->key = key;
	this->value = value;
	this->previous = previous;
	this->next = next;
    this->hash = context->fromInteger(
        (((long) value) ^ 
         (previous? (long) previous->hash : 0L) ^ 
         (next? (long) next->hash : 0L)));
};

IdentityDict::~IdentityDict() {

};

int getBalance(IdentityDict *self) {
	if (!self)
		return 0;
	if (self->next && self->previous)
		return self->next->height - self->previous->height;
	else if (self->previous)
		return -self->previous->height;
	else if (self->next)
		return self->next->height;
	else
		return 0;
}

// A utility function to right rotate subtree rooted with y
// See the diagram given above.
IdentityDict *rightRotate(ProtoContext *context, IdentityDict *n)
{
    IdentityDict *newRight = new(context) IdentityDict(
        context,
        n->key,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) IdentityDict(
        context,
        n->previous->key,
        n->previous->value,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
IdentityDict *leftRotate(ProtoContext *context, IdentityDict *n) {
    IdentityDict *newLeft = new(context) IdentityDict(
        context,
        n->key,
        n->value,
        n->previous,
        n->next->previous
    );
    return new(context) IdentityDict(
        context,
        n->next->key,
        n->next->value,
        newLeft,
        n->next->next
    );
};

IdentityDict *rebalance(ProtoContext *context, IdentityDict *newNode) {
    while (TRUE) {
        int balance = getBalance(newNode);

        // If this node becomes unbalanced, then
        // there are 4 cases

        // Left Left Case
        if (balance < -1 && getBalance(newNode->previous) < 0) {
            newNode = rightRotate(context, newNode);
        }
        else {
            // Right Right Case
            if (balance > 1 && getBalance(newNode->previous) > 0) {
                newNode = leftRotate(context, newNode);
            }
            // Left Right Case
            else {
                if (balance < 0 && getBalance(newNode->previous) > 0) {
                    newNode = new(context) IdentityDict(
                        context,
						newNode->key,
                        newNode->value,
                        leftRotate(context, newNode->previous),
                        newNode->next
                    );
                    newNode = rightRotate(context, newNode);
                }
                else {
                    // Right Left Case
                    if (balance > 0 && getBalance(newNode->next) < 0) {
                        newNode = new(context) IdentityDict(
                            context,
							newNode->key,
                            newNode->value,
                            newNode->previous,
                            rightRotate(context, newNode->next)
                        );
                        newNode = leftRotate(context, newNode);
                    }
                    else
                        return newNode;
                }
            }
        }
    }
}

BOOLEAN IdentityDict::has(ProtoContext *context, ProtoObject *key) {
	if (!this->key)
		return FALSE;

    IdentityDict *node = this;
	while (node) {
		if (node->key == key)
			return TRUE;
        int cmp = ((long) key) - ((long) this->key);
        if (cmp < 0)
            node = node->previous;
        else if (cmp > 1)
            node = node->next;
	}

    if (node)
        return TRUE;
    else
        return FALSE;
};

ProtoObject *IdentityDict::getAt(ProtoContext *context, ProtoObject *key) {
	if (!this->key)
		return PROTO_NONE;

    IdentityDict *node = this;
	while (node) {
		if (node->key == key)
			return node->value;
        int cmp = ((long) key) - ((long) this->key);
        if (cmp < 0)
            node = node->previous;
        else if (cmp > 1)
            node = node->next;
	}

    if (node)
        return node->value;
    else
        return PROTO_NONE;

};

IdentityDict *IdentityDict::setAt(ProtoContext *context, ProtoObject *key, ProtoObject *value) {
	IdentityDict *newNode;
	int cmp;

	// Empty tree case
	if (!this->key)
        return new(context) IdentityDict(
            context,
            key,
			value
        );

    cmp = ((long) key) - ((long)this->key);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) IdentityDict(
                context,
                this->key,
				this->value,
                this->previous,
                this->next->setAt(context, key, value)
            );
        }
        else {
            newNode = new(context) IdentityDict(
                context,
                this->key,
				this->value,
                previous = this->previous,
                next = new(context) IdentityDict(
                    context,
                    key = this->key
                )
            );
        }
    }
    else {
		if (cmp < 0) {
			if (this->previous) {
				newNode = new(context) IdentityDict(
					context,
					this->key,
					this->value,
					previous = this->previous->setAt(context, key, value),
					next = this->next
				);
			}
			else {
				newNode = new(context) IdentityDict(
					context,
					this->key,
					this->value,
					new(context) IdentityDict(
						context,
						key = this->key
					),
					this->next
				);
			}
		}
		else 
			return this;
	}

	return rebalance(context, newNode);
};

IdentityDict *IdentityDict::removeAt(ProtoContext *context, ProtoObject *key) {
	if (key == this->key && !this->next && !this->previous)
		return new(context) IdentityDict(context);

	if (key == this->key) {
		if (!this->next && this->previous)
			return this->previous;

		return this->next;
	}

	int cmp = ((long) key) - ((long) this->key);
	IdentityDict *newNode;
	if (cmp < 0) {
		if (!this->previous)
			return this;
		newNode = this->previous->removeAt(context, key);
		if (newNode->getSize(context) == 0)
			newNode = new(context) IdentityDict(
				context,
				this->key,
				this->value,
				NULL,
				this->next
			);
	}
	else {
		if (!this->next)
			return this;
		newNode = this->next->removeAt(context, key);
		if (newNode->getSize(context) == 0)
			newNode = new(context) IdentityDict(
				context,
				this->key,
				this->value,
				this->previous,
				NULL
			);
	}

	return rebalance(context, newNode);
};

struct matchState {
	IdentityDict *otherDictionary;
	BOOLEAN match;
};

void match(ProtoContext *context, void *self, ProtoObject *key, ProtoObject *value) {
	struct matchState *state = (struct matchState *) self;

	if (!state->otherDictionary->has(context, key) || 
	    state->otherDictionary->getAt(context, key) != value)
		state->match = FALSE;

};

int	IdentityDict::isEqual(ProtoContext *context, IdentityDict *otherDict) {
	if (this->count != otherDict->count)
		return FALSE;

	struct matchState state;
	state.otherDictionary = otherDict;
	state.match = TRUE;

	this->processElements(context, (void *) &state, match);
	
	return state.match;
};

unsigned long IdentityDict::getSize(ProtoContext *context) {
	return this->count;
};

void IdentityDict::processReferences (
	ProtoContext *context,
	void *self,
	void (*method) (
		ProtoContext *context,
		void *self,
		Cell *cell
	)
) {
	if (this->previous)
		this->previous->processReferences(context, self, method);

	ProtoObjectPointer p;

	if (this->key != NULL) {
		p.oid.oid = this->key;
		if (p.op.pointer_tag == POINTER_TAG_CELL)
			method(context, self, p.cell.cell);

		p.oid.oid = this->value;
		if (p.op.pointer_tag == POINTER_TAG_CELL)
			method(context, self, p.cell.cell);
	}

	if (this->next)
		this->next->processReferences(context, self, method);

	method(context, self, this);
};

void IdentityDict::processElements (
	ProtoContext *context,
	void *self,
	void (*method) (
		ProtoContext *context,
		void *self,
		ProtoObject *key,
		ProtoObject *value
	)
) {
	if (this->previous)
		this->previous->processElements(context, self, method);

	if (this->key != NULL)
		method(context, self, this->key, this->value);

	if (this->next)
		this->next->processElements(context, self, method);

};

void IdentityDict::processKeys (
	ProtoContext *context,
	void *self,
	void (*method) (
		ProtoContext *context,
		void *self,
		ProtoObject *element
	)
) {
	if (this->previous)
		this->previous->processKeys(context, self, method);

	if (this->key != NULL)
		method(context, self, this->key);

	if (this->next)
		this->next->processKeys(context, self, method);

};

void IdentityDict::processValues (
	ProtoContext *context,
	void *self,
	void (*method) (
		ProtoContext *context,
		void *self,
		ProtoObject *element
	)
) {
	if (this->previous)
		this->previous->processValues(context, self, method);

	if (this->key != NULL)
		method(context, self, this->value);

	if (this->next)
		this->next->processValues(context, self, method);

};

};
