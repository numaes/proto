/*
 * ProtoSparseList.cpp
 *
 *  Created on: 2017-05-01
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

namespace proto {

#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

template<class T>
ProtoSparseListIteratorImplementation<T>::ProtoSparseListIteratorImplementation(
		ProtoContext *context,
		int state,
		ProtoSparseListImplementation<T> *current,
		ProtoSparseListIteratorImplementation<T> *queue
	) : Cell(context) {
	this->state = state;
	this->current = current;
	this->queue = queue;
};

template<class T>
ProtoSparseListIteratorImplementation<T>::~ProtoSparseListIteratorImplementation() {};

template<class T>
int ProtoSparseListIteratorImplementation<T>::hasNext(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_PREVIOUS && this->current->previous)
		return TRUE;
	if (this->state == ITERATOR_NEXT_THIS)
		return TRUE;
	if (this->state == ITERATOR_NEXT_NEXT && this->current->next)
		return TRUE;
	if (this->queue)
		return this->queue->hasNext(context);

	return FALSE;
};

template<class T>
unsigned long ProtoSparseListIteratorImplementation<T>::nextKey(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_THIS) {
		return this->current->index;
	}
	return NULL;
};

template<class T>
T* ProtoSparseListIteratorImplementation<T>::nextValue(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_THIS) {
		return this->current->value;
	}
	return NULL;
};

template<class T>
ProtoSparseListIteratorImplementation<T> *ProtoSparseListIteratorImplementation<T>::advance(ProtoContext *context) {
	if (this->state == ITERATOR_NEXT_PREVIOUS)
		return new(context) ProtoSparseListIteratorImplementation(
			context,
			ITERATOR_NEXT_THIS,
			this->current,
			this->queue
		);

	if (this->state == ITERATOR_NEXT_THIS && this->current->next)
		return this->current->next->getIterator(context);

	if (this->state == ITERATOR_NEXT_THIS) {
		if (this->queue) {
			ProtoSparseListIteratorImplementation *newState = new(context) ProtoSparseListIteratorImplementation(
				context,
				ITERATOR_NEXT_NEXT,
				this->current,
				this->queue
			);
			ProtoSparseListImplementation<T> *node = this->queue->current;
			while (node->previous) {
				newState = new(context) ProtoSparseListIteratorImplementation(
					context,
					node->previous? ITERATOR_NEXT_PREVIOUS : ITERATOR_NEXT_THIS,
					node,
					newState
				);
				node = node->previous;
			}
			return newState;
		}
		else		
			return NULL;
	}

	if (this->state == ITERATOR_NEXT_NEXT && this->current->next)
		if (this->queue)
			return this->queue->advance(context);

	return NULL;
};

template<class T>
ProtoObject	  *ProtoSparseListIteratorImplementation<T>::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_SPARSE_LIST_ITERATOR;

    return p.oid.oid;
};

template<class T>
void ProtoSparseListIteratorImplementation<T>::finalize(ProtoContext *context) {};

template<class T>
void ProtoSparseListIteratorImplementation<T>::processReferences(
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

template<class T>
ProtoSparseListImplementation<T>::ProtoSparseListImplementation(
	ProtoContext *context,

	unsigned long index,
	ProtoObject *value,
	ProtoSparseListImplementation<T> *previous,
	ProtoSparseListImplementation<T> *next
) : Cell(context) {
	this->index = index;
	this->value = value;
	this->previous = previous;
	this->next = next;
    this->hash = index ^
				 (value? value->getHash(context) : 0) ^
                 (previous? previous->hash : 0) ^
                 (next? next->hash : 0);

    this->count = (value? 1 : 0) + 
                  (previous? previous->count : 0) + 
                  (next? next->count : 0);

    unsigned long previous_height = previous? previous->height : 0;
    unsigned long next_height = next? next->height : 0;
    this->height = (value? 1 : 0) + 
                   (previous_height > next_height? previous_height : next_height);

};

template<class T>
ProtoSparseListImplementation<T>::~ProtoSparseListImplementation() {

};

template<class T>
int getBalance(ProtoSparseListImplementation<T> *self) {
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
template<class T>
ProtoSparseListImplementation<T> *rightRotate(ProtoContext *context, ProtoSparseListImplementation<T> *n)
{
	if (!n->previous)
		return n;

    ProtoSparseListImplementation<T> *newRight = new(context) ProtoSparseListImplementation<T>(
        context,
        n->index,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) ProtoSparseListImplementation(
        context,
        n->previous->index,
        n->previous->value,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
template<class T>
ProtoSparseListImplementation<T> *leftRotate(ProtoContext *context, ProtoSparseListImplementation<T> *n) {
    if (!n->next)
        return n;

    ProtoSparseListImplementation<T> *newLeft = new(context) ProtoSparseListImplementation<T>(
        context,
        n->index,
        n->value,
        n->previous,
        n->next->previous
    );
    return new(context) ProtoSparseListImplementation(
        context,
        n->next->index,
        n->next->value,
        newLeft,
        n->next->next
    );
};

template<class T>
ProtoSparseListImplementation<T> *rebalance(ProtoContext *context, ProtoSparseListImplementation<T> *newNode) {
    while (TRUE) {
        int balance = getBalance(newNode);

        // If this node becomes unbalanced, then
        // there are 4 cases

        if (balance >= -1 && balance <= 1)
            return newNode;
            
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
                    newNode = new(context) ProtoSparseListImplementation(
                        context,
						newNode->index,
                        newNode->value,
                        leftRotate(context, newNode->previous),
                        newNode->next
                    );
                    if (!newNode->previous)
                        return newNode;
                    newNode = rightRotate(context, newNode);
                }
                else {
                    // Right Left Case
                    if (balance > 0 && getBalance(newNode->next) < 0) {
                        newNode = new(context) ProtoSparseListImplementation(
                            context,
							newNode->index,
                            newNode->value,
                            newNode->previous,
                            rightRotate(context, newNode->next)
                        );
                        if (!newNode->next)
                            return newNode;
                        newNode = leftRotate(context, newNode);
                    }
                    else
                        return newNode;
                }
            }
        }
    }
}

template<class T>
BOOLEAN ProtoSparseListImplementation<T>::has(ProtoContext *context, unsigned long index) {
	if (!this->index)
		return FALSE;

    ProtoSparseListImplementation *node = this;
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

template<class T>
T *ProtoSparseListImplementation<T>::getAt(ProtoContext *context, unsigned long index) {
	if (!this->index)
		return PROTO_NONE;

    ProtoSparseListImplementation *node = this;
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

template<class T>
ProtoSparseListImplementation<T> *ProtoSparseListImplementation<T>::setAt(ProtoContext *context, unsigned long index, T *value) {
	ProtoSparseListImplementation *newNode;
	int cmp;

	// Empty tree case
	if (!this->index)
        return new(context) ProtoSparseListImplementation(
            context,
            index,
			value
        );

    cmp = ((long) index) - ((long)this->index);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) ProtoSparseListImplementation(
                context,
                this->index,
				this->value,
                this->previous,
                this->next->setAt(context, index, value)
            );
        }
        else {
            newNode = new(context) ProtoSparseListImplementation(
                context,
                this->index,
				this->value,
                previous = this->previous,
                next = new(context) ProtoSparseListImplementation(
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
				newNode = new(context) ProtoSparseListImplementation(
					context,
					this->index,
					this->value,
					previous = this->previous->setAt(context, index, value),
					this->next
				);
			}
			else {
				newNode = new(context) ProtoSparseListImplementation(
					context,
					this->index,
					this->value,
					new(context) ProtoSparseListImplementation(
						context,
						index,
						value
					),
					this->next
				);
			}
		}
		else {
			newNode = new(context) ProtoSparseListImplementation(
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

template<class T>
unsigned long firstindex(ProtoContext *context, ProtoSparseListImplementation<T> *self) {
	while (self->previous)
		self = self->previous;
	
	return self->index;
};

template<class T>
T* firstValue(ProtoContext *context, ProtoSparseListImplementation<T> *self) {
	while (self->previous)
		self = self->previous;
	
	return self->value;
};

template<class T>
ProtoSparseListImplementation<T> *removeFirst(ProtoContext *context, ProtoSparseListImplementation<T> *self) {
	if (!self->index)
        return self;

    ProtoSparseListImplementation<T> *newNode;

    if (self->previous) {
        newNode = removeFirst(context, self->previous);
        newNode = new(context) ProtoSparseListImplementation<T>(
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
        
        newNode = new(context) ProtoSparseListImplementation<T>(
            context
		);
    }

    return rebalance(context, newNode);
}

template<class T>
ProtoSparseListImplementation<T> *ProtoSparseListImplementation<T>::removeAt(ProtoContext *context, unsigned long index) {
	if (index == this->index && !this->next && !this->previous)
		return new(context) ProtoSparseListImplementation(context);

	if (index == this->index) {
		if (!this->next && this->previous)
			return this->previous;

		return this->next;
	}

	int cmp = ((long) index) - ((long) this->index);
	ProtoSparseListImplementation *newNode;
	if (cmp < 0) {
		if (!this->previous)
			return this;
		newNode = this->previous->removeAt(context, index);
		if (newNode->getSize(context) == 0)
			newNode = new(context) ProtoSparseListImplementation(
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

			newNode = new(context) ProtoSparseListImplementation(
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
				newNode = new(context) ProtoSparseListImplementation(
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

template<class T>
struct matchState {
	ProtoSparseListImplementation<T> *otherDictionary;
	BOOLEAN match;
};

template<class T>
void match(ProtoContext *context, void *self, unsigned long index, T *value) {
	struct matchState<T> *state = (struct matchState<T> *) self;

	if (!state->otherDictionary->has(context, index) || 
	    state->otherDictionary->getAt(context, index) != value)
		state->match = FALSE;

};

template<class T>
int	ProtoSparseListImplementation<T>::isEqual(ProtoContext *context, ProtoSparseList<T> *otherDict) {
	if (this->count != otherDict->getSize(context))
		return FALSE;

	struct matchState<T> state;
	state.otherDictionary = (ProtoSparseListImplementation<T> *) otherDict;
	state.match = TRUE;

	this->processElements(context, (void *) &state, match);
	
	return state.match;
};

template<class T>
unsigned long ProtoSparseListImplementation<T>::getSize(ProtoContext *context) {
	return this->count;
};

template<class T>
void ProtoSparseListImplementation<T>::finalize(ProtoContext *context) {};

template<class T>
void ProtoSparseListImplementation<T>::processReferences (
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

	if (this->next)
		this->next->processReferences(context, self, method);

	method(context, self, this);
};

template<class T>
void ProtoSparseListImplementation<T>::processElements (
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

	if (this->value != NULL)
		method(context, self, this->index, this->value);

	if (this->next)
		this->next->processElements(context, self, method);

};

template<class T>
void ProtoSparseListImplementation<T>::processValues (
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

	method(context, self, this->value);

	if (this->next)
		this->next->processValues(context, self, method);

};

template<class T>
ProtoObject *ProtoSparseListImplementation<T>::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_SPARSE_LIST;

    return p.oid.oid;
};

template<class T>
unsigned long ProtoSparseListImplementation<T>::getHash(ProtoContext *context) {
    return this->hash;
};

template<class T>
ProtoSparseListIteratorImplementation<T> *ProtoSparseListImplementation<T>::getIterator(ProtoContext *context) {
	ProtoSparseListIteratorImplementation<T> *newState = new(context) ProtoSparseListIteratorImplementation<T>(
				context,
				this->previous? ITERATOR_NEXT_PREVIOUS : ITERATOR_NEXT_THIS,
				this,
				NULL
	);

	ProtoSparseListImplementation<T> *node = this;
	while (node->previous) {
		newState = new(context) ProtoSparseListIteratorImplementation<T>(
			context,
			node->previous? ITERATOR_NEXT_PREVIOUS : ITERATOR_NEXT_THIS,
			node,
			newState
		);
		node = node->previous;
	}

	return newState;

};

};
