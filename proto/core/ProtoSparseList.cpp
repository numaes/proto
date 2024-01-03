/*
 * ProtoSparseList.cpp
 *
 *  Created on: 2017-05-01
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {

#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

ProtoSparseListIterator::ProtoSparseListIterator(
		ProtoContext *context,
		int state,
		ProtoSparseList *current,
		ProtoSparseListIterator *queue = NULL
	) : Cell(context) {
	this->state = state;
	this->current = current;
	this->queue = queue;
};

ProtoSparseListIterator::~ProtoSparseListIterator() {};

int ProtoSparseListIterator::hasNext(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_PREVIOUS && this->current->previous)
		return TRUE;
	if (this->state == ITERATOR_NEXT_THIS)
		return TRUE;
	if (this->state == ITERATOR_NEXT_NEXT && this->current->next)
		return TRUE;
	if (this->queue)
		return this->queue->hasNext(context);
};

ProtoObject *ProtoSparseListIterator::next(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_PREVIOUS && this->current->previous)
		return this->current->previous->value;
	if (this->state == ITERATOR_NEXT_THIS)
		return this->current->value;
	if (this->state == ITERATOR_NEXT_NEXT && this->current->next)
		return this->current->next->value;
};

ProtoSparseListIterator *ProtoSparseListIterator::advance(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_PREVIOUS)
		return new(context) ProtoSparseListIterator(
			context,
			ITERATOR_NEXT_THIS,
			this->current,
			this->queue
		);
	if (this->state == ITERATOR_NEXT_THIS && this->current->next)
		return this->current->next->getIterator(context);
	if (this->state == ITERATOR_NEXT_THIS)
		if (this->queue)
			return this->queue->advance(context);
		return NULL;
	if (this->state == ITERATOR_NEXT_NEXT && this->current->next)
		if (this->queue)
			return this->queue->advance(context);
	return NULL;

};

ProtoObject	  *ProtoSparseListIterator::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_SPARSE_LIST_ITERATOR;

    return p.oid.oid;
};

void ProtoSparseListIterator::finalize() {};

void ProtoSparseListIterator::processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	) {

	// TODO

};

ProtoSparseList::ProtoSparseList(
	ProtoContext *context,

	unsigned long index = 0,
	ProtoObject *value = PROTO_NONE,
	ProtoSparseList *previous = NULL,
	ProtoSparseList *next = NULL
) : Cell(context) {
	this->index = index;
	this->value = value;
	this->previous = previous;
	this->next = next;
};

ProtoSparseList::~ProtoSparseList() {

};

int getBalance(ProtoSparseList *self) {
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
ProtoSparseList *rightRotate(ProtoContext *context, ProtoSparseList *n)
{
    ProtoSparseList *newRight = new(context) ProtoSparseList(
        context,
        n->index,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) ProtoSparseList(
        context,
        n->previous->index,
        n->previous->value,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
ProtoSparseList *leftRotate(ProtoContext *context, ProtoSparseList *n) {
    ProtoSparseList *newLeft = new(context) ProtoSparseList(
        context,
        n->index,
        n->value,
        n->previous,
        n->next->previous
    );
    return new(context) ProtoSparseList(
        context,
        n->next->index,
        n->next->value,
        newLeft,
        n->next->next
    );
};

ProtoSparseList *rebalance(ProtoContext *context, ProtoSparseList *newNode) {
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
                    newNode = new(context) ProtoSparseList(
                        context,
						newNode->index,
                        newNode->value,
                        leftRotate(context, newNode->previous),
                        newNode->next
                    );
                    newNode = rightRotate(context, newNode);
                }
                else {
                    // Right Left Case
                    if (balance > 0 && getBalance(newNode->next) < 0) {
                        newNode = new(context) ProtoSparseList(
                            context,
							newNode->index,
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

BOOLEAN ProtoSparseList::has(ProtoContext *context, unsigned long index) {
	if (!this->index)
		return FALSE;

    ProtoSparseList *node = this;
	while (node) {
		if (node->index == index)
			return TRUE;
        int cmp = ((long) index) - ((long) this->index);
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

ProtoObject *ProtoSparseList::getAt(ProtoContext *context, unsigned long index) {
	if (!this->index)
		return PROTO_NONE;

    ProtoSparseList *node = this;
	while (node) {
		if (node->index == index)
			return node->value;
        int cmp = ((long) index) - ((long) this->index);
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

ProtoSparseList *ProtoSparseList::setAt(ProtoContext *context, unsigned long index, ProtoObject *value) {
	ProtoSparseList *newNode;
	int cmp;

	// Empty tree case
	if (!this->index)
        return new(context) ProtoSparseList(
            context,
            index,
			value
        );

    cmp = ((long) index) - ((long)this->index);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) ProtoSparseList(
                context,
                this->index,
				this->value,
                this->previous,
                this->next->setAt(context, index, value)
            );
        }
        else {
            newNode = new(context) ProtoSparseList(
                context,
                this->index,
				this->value,
                previous = this->previous,
                next = new(context) ProtoSparseList(
                    context,
                    index,
					value
                )
            );
        }
    }
    else {
		if (cmp < 0) {
			if (this->previous) {
				newNode = new(context) ProtoSparseList(
					context,
					this->index,
					this->value,
					previous = this->previous->setAt(context, index, value),
					this->next
				);
			}
			else {
				newNode = new(context) ProtoSparseList(
					context,
					this->index,
					this->value,
					new(context) ProtoSparseList(
						context,
						index,
						value
					),
					this->next
				);
			}
		}
		else {
			newNode = new(context) ProtoSparseList(
				context,
				this->index,
				value,
				this->previous,
				this->next
			);
		}
	}

	return rebalance(context, newNode);
};

unsigned long firstindex(ProtoContext *context, ProtoSparseList *self) {
	while (self->previous)
		self = self->previous;
	
	return self->index;
};

ProtoObject *firstValue(ProtoContext *context, ProtoSparseList *self) {
	while (self->previous)
		self = self->previous;
	
	return self->value;
};

ProtoSparseList *removeFirst(ProtoContext *context, ProtoSparseList *self) {
	if (!self->index)
        return self;

    ProtoSparseList *newNode;

    if (self->previous) {
        newNode = removeFirst(context, self->previous);
        if (newNode->index == NULL)
            newNode = NULL; 
        newNode = new(context) ProtoSparseList(
            context,
			self->index,
			self->value,
            newNode,
            self->next
        );
    }
    else {
        if (self->next)
            return self->next;
        
        newNode = new(context) ProtoSparseList(
            context
		);
    }

    return rebalance(context, newNode);
}

ProtoSparseList *ProtoSparseList::removeAt(ProtoContext *context, unsigned long index) {
	if (index == this->index && !this->next && !this->previous)
		return new(context) ProtoSparseList(context);

	if (index == this->index) {
		if (!this->next && this->previous)
			return this->previous;

		return this->next;
	}

	int cmp = ((long) index) - ((long) this->index);
	ProtoSparseList *newNode;
	if (cmp < 0) {
		if (!this->previous)
			return this;
		newNode = this->previous->removeAt(context, index);
		if (newNode->getSize(context) == 0)
			newNode = new(context) ProtoSparseList(
				context,
				this->index,
				this->value,
				NULL,
				this->next
			);
	}
	else {
		if (cmp > 0) {
			if (!this->next)
				return this;

			newNode = this->next->removeAt(context, index);
			if (newNode->getSize(context) == 0)
				newNode = NULL;

			newNode = new(context) ProtoSparseList(
				context,
				this->index,
				this->value,
				this->previous,
				newNode
			);
		}
		else {
			if (this->next) {
				newNode = removeFirst(context, this->next);
				if (newNode->count == 0)
					newNode = NULL; 
				newNode = new(context) ProtoSparseList(
					context,
					firstindex(context, this->next),
					firstValue(context, this->next),
					this->previous,
					newNode
				);
			}
			else
				return this->previous;
		}
	}

	return rebalance(context, newNode);
};

struct matchState {
	ProtoSparseList *otherDictionary;
	BOOLEAN match;
};

void match(ProtoContext *context, void *self, unsigned long index, ProtoObject *value) {
	struct matchState *state = (struct matchState *) self;

	if (!state->otherDictionary->has(context, index) || 
	    state->otherDictionary->getAt(context, index) != value)
		state->match = FALSE;

};

int	ProtoSparseList::isEqual(ProtoContext *context, ProtoSparseList *otherDict) {
	if (this->count != otherDict->count)
		return FALSE;

	struct matchState state;
	state.otherDictionary = otherDict;
	state.match = TRUE;

	this->processElements(context, (void *) &state, match);
	
	return state.match;
};

unsigned long ProtoSparseList::getSize(ProtoContext *context) {
	return this->count;
};

void ProtoSparseList::processReferences (
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

	if (this->index != NULL) {
		p.oid.oid = this->value;
		if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE)
			method(context, self, p.cell.cell);
	}

	if (this->next)
		this->next->processReferences(context, self, method);

	method(context, self, this);
};

void ProtoSparseList::processElements (
	ProtoContext *context,
	void *self,
	void (*method) (
		ProtoContext *context,
		void *self,
		unsigned long index,
		ProtoObject *value
	)
) {
	if (this->previous)
		this->previous->processElements(context, self, method);

	if (this->index != NULL)
		method(context, self, this->index, this->value);

	if (this->next)
		this->next->processElements(context, self, method);

};

void ProtoSparseList::processValues (
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

	if (this->index != NULL)
		method(context, self, this->value);

	if (this->next)
		this->next->processValues(context, self, method);

};

ProtoObject *ProtoSparseList::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_SPARSE_LIST;

    return p.oid.oid;
};

unsigned long ProtoSparseList::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

ProtoSparseListIterator *ProtoSparseList::getIterator(ProtoContext *context) {
	ProtoSparseList *node = this;
	ProtoSparseListIterator *iterator = NULL;

	while (node) {
		if (node->previous) {
			iterator = new(context) ProtoSparseListIterator(
				context,
				node->previous->previous? ITERATOR_NEXT_PREVIOUS : ITERATOR_NEXT_THIS,
				node->previous,
				iterator
			);
			node = node->previous->previous;
		}
	}

	return iterator;
}

};
