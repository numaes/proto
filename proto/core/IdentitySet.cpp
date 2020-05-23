/*
 * ProtoSet.cpp
 *
 *  Created on: 2017-08-01
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"


#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

ProtoSet::ProtoSet(
    ProtoContext *context,

    ProtoObject *value = PROTO_NONE,
    ProtoSet *previous = NULL,
    ProtoSet *next = NULL
) : Cell(
	context,
	type = CELL_TYPE_PROTO_SET,
    height = 1 + max(previous? previous->height : 0, next? next->height : 0),
    count = (value? 1: 0) + (previous? previous->count : 0) + (next? next->count : 0)
) {
    this->value = value;
    this->previous = previous;
    this->next = next;
    this->hash = PROTO_NONE;
};

ProtoSet::~ProtoSet() {

};

int getBalance(ProtoSet *self) {
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

BOOLEAN ProtoSet::has(ProtoContext *context, ProtoObject *value) {
	if (!this->value)
		return FALSE;

    ProtoSet *node = this;
	while (node) {
		if (node->value == value)
			return TRUE;
        int cmp = ((int) value) - ((int) this->value);
        if (cmp < 0)
            node = node->previous;
        else if (cmp > 1)
            node = node->next;
	}

    if (node)
        return TRUE;
    else
        return NULL;
};

ProtoSet *ProtoSet::add(ProtoContext *context, ProtoObject *value) {
	ProtoSet *newNode;
	ProtoSet *newAux;
	int cmp;

	// Empty tree case
	if (!this->value)
        return new(context) ProtoSet(
            context,
			value = value
        );

    cmp = ((int) value) - ((int)this->value);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) ProtoSet(
                context,
                value = this->value,
                previous = this->previous,
                next = this->next->add(context, value)
            );
        }
        else {
            newNode = new(context) ProtoSet(
                context,
                value = this->value,
                previous = this->previous,
                next = new(context) ProtoSet(
                    context,
                    value = this->value
                )
            );
        }
    }
    else if (cmp < 0) {
        if (this->previous) {
            newNode = new(context) ProtoSet(
                context,
                value = this->value,
                previous = this->previous->add(context, value),
                next = this->next
            );
        }
        else {
            newNode = new(context) ProtoSet(
                context,
                value = this->value,
                previous = new(context) ProtoSet(
                    context,
                    value = this->value
                ),
                next = this->next
            );
        }
    }
    else 
        return this;

    int balance = getBalance(newNode);

    // If this node becomes unbalanced, then
    // there are 4 cases

    // Left Left Case
    if (balance < 1 && ((int) value) - ((int) newNode->previous->value) < 0)
        return rightRotate(context, newNode);

    // Right Right Case
    if (balance > 1 && ((int) value) - ((int) newNode->previous->value) > 0)
        return leftRotate(context, newNode);

    // Left Right Case
    if (balance < 1 && ((int) value) - ((int) newNode->previous->value) > 0) {
        newNode = new(context) ProtoSet(
            context,
			value = newNode->value,
            previous = leftRotate(context, newNode->previous),
            next = newNode->next
        );
        return rightRotate(context, newNode);
    }

    // Right Left Case
    if (balance > 1 && ((int) value) - ((int) newNode->previous->value) < 0) {
        newNode = new(context) ProtoSet(
            context,
            value = newNode->value,
            previous = newNode->previous,
            next = rightRotate(context, newNode->next)
        );
        return leftRotate(context, newNode);
    }

    return newNode;

};

ProtoSet *ProtoSet::removeAt(ProtoContext *context, ProtoObject *value) {
	if (value == this->value && !this->next && !this->previous)
		return new(context) ProtoSet(context);

	if (value == this->value) {
		if (!this->next && this->previous)
			return this->previous;

		return this->next;
	}

	int cmp = ((int) value) - ((int) this->value);
	ProtoSet *newNode;
	if (cmp < 0) {
		if (!this->previous)
			return this;
		newNode = this->previous->removeAt(context, value);
		if (newNode->getSize(context) == 0)
			newNode = new(context) ProtoSet(
				context,
				value = this->value,
				previous = NULL,
				next = this->next
			);
	}
	else {
		if (!this->next)
			return this;
		newNode = this->next->removeAt(context, value);
		if (newNode->getSize(context) == 0)
			newNode = new(context) ProtoSet(
				context,
				value = this->value,
				previous = this->previous,
				next = NULL
			);
	}

	int balance = getBalance(newNode);

    // If this node becomes unbalanced, then
    // there are 4 cases

    // Left Left Case
    if (balance < 1 && ((int) value) - ((int) newNode->previous->value) < 0)
        return rightRotate(context, newNode);

    // Right Right Case
    if (balance > 1 && ((int) value) - ((int) newNode->previous->value) > 0)
        return leftRotate(context, newNode);

    // Left Right Case
    if (balance < 1 && ((int) value) - ((int) newNode->previous->value) > 0) {
        newNode = new(context) ProtoSet(
            context,
            value = newNode->value,
            previous = leftRotate(context, newNode->previous),
            next = newNode->next
        );
        return rightRotate(context, newNode);
    }

    // Right Left Case
    if (balance > 1 && ((int) value) - ((int) newNode->previous->value) < 0) {
        newNode = new(context) ProtoSet(
            context,
            value = newNode->value,
            previous = newNode->previous,
            next = rightRotate(context, newNode->next)
        );
        return leftRotate(context, newNode);
    }

    return newNode;
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
		p.oid = this->value;
		if (p.op.pointer_tag == POINTER_TAG_CELL)
			method(context, self, p.cell);
	}

	if (this->next)
		this->next->processReferences(context, self, method);
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

	ProtoObjectPointer p;

	if (this->value != NULL)
		method(context, self, this->value);

	if (this->next)
		this->next->processValues(context, self, method);

};

