/*
 * ProtoSet.cpp
 *
 *  Created on: 2017-08-01
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {


#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

ProtoSet::ProtoSet(
    ProtoContext *context,

    ProtoObject *value,
    ProtoSet *previous,
    ProtoSet *next
) : Cell(
	context,
	type = CELL_TYPE_PROTO_SET,
    height = 1 + max(previous? previous->height : 0, next? next->height : 0),
    count = (value? 1: 0) + (previous? previous->count : 0) + (next? next->count : 0)
) {
    this->value = value;
    this->previous = previous;
    this->next = next;
    this->hash = context->fromInteger(
        (((long) value) ^ 
         (previous? (long) previous->hash : 0L) ^ 
         (next? (long) next->hash : 0L)));
};

ProtoSet::~ProtoSet() {

};

int getBalance(ProtoSet *self) {
    if (!self) {
        return 0;
    }

	if (self->next && self->previous)
		return self->next->height - self->previous->height;
	else {
        if (self->previous)
            return -self->previous->height;
        else {
            if (self->next)
                return self->next->height;
        };
    };

	return 0;
};

// A utility function to right rotate subtree rooted with y
// See the diagram given above.
ProtoSet *rightRotate(ProtoContext *context, ProtoSet *n)
{
    ProtoSet *newRight = new(context) ProtoSet(
        context,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) ProtoSet(
        context,
        n->previous->value,
        n->previous->previous,
        newRight
    );
};

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
ProtoSet *leftRotate(ProtoContext *context, ProtoSet *n) {
    ProtoSet *newLeft = new(context) ProtoSet(
        context,
        n->value,
        n->previous,
        n->next->previous
    );
    return new(context) ProtoSet(
        context,
        n->next->value,
        newLeft,
        n->next->next
    );
};

ProtoSet *rebalance(ProtoContext *context, ProtoSet *newNode) {
    while (TRUE) {
        int balance = getBalance(newNode);

        // If this node becomes unbalanced, then
        // there are 4 cases

        // Left Left Case
        if (balance < -1) {
            newNode = rightRotate(context, newNode);
        }
        else {
            // Right Right Case
            if (balance > 1) {
                newNode = leftRotate(context, newNode);
            }
            // Left Right Case
            else {
                if (balance < 0 && getBalance(newNode->previous) > 0) {
                    newNode = new(context) ProtoSet(
                        context,
                        newNode->value,
                        leftRotate(context, newNode->previous),
                        newNode->next
                    );
                    newNode = rightRotate(context, newNode);
                }
                else {
                    // Right Left Case
                    if (balance > 0 && getBalance(newNode->next) < 0) {
                        newNode = new(context) ProtoSet(
                            context,
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
};

BOOLEAN ProtoSet::has(ProtoContext *context, ProtoObject *value) {
	if (!this->value)
		return FALSE;

    ProtoSet *node = this;
	while (node) {
		if (node->value == value)
			return TRUE;
        long cmp = ((long) value) - ((long) node->value);
        if (cmp < 0)
            node = node->previous;
        else 
            node = node->next;
	}

    if (node)
        return TRUE;
    else
        return FALSE;
};

ProtoSet *ProtoSet::add(ProtoContext *context, ProtoObject *value) {
	ProtoSet *newNode;
	long cmp;

	// Empty tree case
	if (!this->value)
        return new(context) ProtoSet(
            context,
			value = value
        );

    cmp = ((long) value) - ((long)this->value);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) ProtoSet(
                context,
                this->value,
                this->previous,
                this->next->add(context, value)
            );
        }
        else {
            newNode = new(context) ProtoSet(
                context,
                this->value,
                this->previous,
                new(context) ProtoSet(
                    context,
                    value
                )
            );
        }
    }
    else if (cmp < 0) {
        if (this->previous) {
            newNode = new(context) ProtoSet(
                context,
                this->value,
                this->previous->add(context, value),
                this->next
            );
        }
        else {
            newNode = new(context) ProtoSet(
                context,
                this->value,
                new(context) ProtoSet(
                    context,
                    value
                ),
                this->next
            );
        }
    }
    else 
        return this;

    return rebalance(context, newNode);
};

ProtoSet *ProtoSet::removeAt(ProtoContext *context, ProtoObject *value) {
	if (value == this->value && !this->next && !this->previous)
		return new(context) ProtoSet(context);

	if (value == this->value) {
		if (!this->next && this->previous)
			return this->previous;

		return this->next;
	}

	long cmp = ((long) value) - ((long) this->value);
	ProtoSet *newNode;
	if (cmp < 0) {
		if (!this->previous)
			return this;
		newNode = this->previous->removeAt(context, value);
		if (newNode->getSize(context) == 0)
			newNode = new(context) ProtoSet(
				context,
				this->value,
				NULL,
				this->next
			);
	}
	else {
		if (!this->next)
			return this;
		newNode = this->next->removeAt(context, value);
		if (newNode->getSize(context) == 0)
			newNode = new(context) ProtoSet(
				context,
				this->value,
				this->previous,
				NULL
			);
	}

    return rebalance(context, newNode);
};

struct matchState {
	ProtoSet *otherSet;
	BOOLEAN match;
};

void match(ProtoContext *context, void *self, ProtoObject *value) {
	struct matchState *state = (struct matchState *) self;

	if (!state->otherSet->has(context, value))
		state->match = FALSE;

};

int	ProtoSet::isEqual(ProtoContext *context, ProtoSet *otherSet) {
	if (this->count != otherSet->count)
		return FALSE;

	struct matchState state;
	state.otherSet = otherSet;
	state.match = TRUE;

	this->processValues(context, (void *) &state, match);
	
	return state.match;
};

unsigned long ProtoSet::getSize(ProtoContext *context) {
	return this->count;
};

struct asListState {
	ProtoList *outputList;
};

void asListIterator(ProtoContext *context, void *self, ProtoObject *value) {
	struct asListState *state = (struct asListState *) self;

    state->outputList = state->outputList->appendLast(context, value);
};

ProtoList *ProtoSet::asList(ProtoContext *context) {
    struct asListState state;
    state.outputList = new(context) ProtoList(context);
    this->processValues(context, (void *) &state, asListIterator);

    return state.outputList;
};

struct unionState {
	ProtoSet *outputSet;
};

void unionIterator(ProtoContext *context, void *self, ProtoObject *value) {
	struct unionState *state = (struct unionState *) self;

    state->outputSet = state->outputSet->add(context, value);
};

ProtoSet *ProtoSet::getUnion(ProtoContext *context, ProtoSet* otherSet) {
    struct unionState state;
    state.outputSet = new(context) ProtoSet(context);
    this->processValues(context, (void *) &state, unionIterator);

    otherSet->processValues(context, (void *) &state, unionIterator);

    return state.outputSet;
};

struct intersectionState {
	ProtoSet *outputSet;
    ProtoSet *otherSet;
};

void intersectionStateIterator(ProtoContext *context, void *self, ProtoObject *value) {
	struct intersectionState *state = (struct intersectionState *) self;

    if (state->otherSet->has(context, value))
        state->outputSet = state->outputSet->add(context, value);
};

ProtoSet *ProtoSet::getIntersection(ProtoContext *context, ProtoSet* otherSet) {
    struct intersectionState state;
    state.outputSet = new(context) ProtoSet(context);
    state.otherSet = otherSet;
    this->processValues(context, (void *) &state, intersectionStateIterator);
    return state.outputSet;
};

struct lessState {
	ProtoSet *outputSet;
    ProtoSet *otherSet;
};

void lessStateIterator(ProtoContext *context, void *self, ProtoObject *value) {
	struct lessState *state = (struct lessState *) self;

    if (!state->otherSet->has(context, value))
        state->outputSet = state->outputSet->add(context, value);
};

ProtoSet *ProtoSet::getLess(ProtoContext *context, ProtoSet* otherSet) {
    struct intersectionState state;
    state.outputSet = new(context) ProtoSet(context);
    state.otherSet = otherSet;
    this->processValues(context, (void *) &state, intersectionStateIterator);
    return state.outputSet;
};

void ProtoSet::processReferences (
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

	if (this->value != NULL) {
		p.oid.oid = this->value;
		if (p.op.pointer_tag == POINTER_TAG_CELL)
			method(context, self, p.cell.cell);
	}

	if (this->next)
		this->next->processReferences(context, self, method);

    method(context, self, this);
};

void ProtoSet::processValues (
	ProtoContext *context,
	void *self,
	void (*method) (
		ProtoContext *context,
		void *self,
		ProtoObject *value
	)
) {
	if (this->previous)
		this->previous->processValues(context, self, method);

	if (this->value != NULL)
		method(context, self, this->value);

	if (this->next)
		this->next->processValues(context, self, method);

};

};
