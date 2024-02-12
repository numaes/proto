/*
 * list.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto internal.h"
#include <string.h>

using namespace std;
namespace proto {


#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

template<class T> ProtoListIteratorImplementation<T>::ProtoListIteratorImplementation(
		ProtoContext *context,
        ProtoListImplementation<T> *base,
		unsigned long currentIndex
	) : Cell(context) {
    this->base = base;
	this->currentIndex = currentIndex;
};

template<class T> ProtoListIteratorImplementation<T>::~ProtoListIteratorImplementation() {};

template<class T> int ProtoListIteratorImplementation<T>::hasNext(ProtoContext *context) {
    if (this->currentIndex >= this->base->getSize(context))
        return FALSE;
    else
        return TRUE;
};

template<class T> T *ProtoListIteratorImplementation<T>::next(ProtoContext *context) {
    return this->base->getAt(context, this->currentIndex);
};

template<class T> ProtoListIteratorImplementation<T> *ProtoListIteratorImplementation<T>::advance(ProtoContext *context) {
    return new(context) ProtoListIteratorImplementation(context, this->base, this->currentIndex);
};

template<class T> ProtoObject *ProtoListIteratorImplementation<T>::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;

    return p.oid.oid;
};

template<class T> void ProtoListIteratorImplementation<T>::finalize(ProtoContext *context) {};

template<class T> void ProtoListIteratorImplementation<T>::processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	) {

    method(context, self, this->base);

};


template<class T> ProtoListImplementation<T>::ProtoListImplementation(
    ProtoContext *context,

    ProtoObject *newValue,
    ProtoListImplementation<T> *newPrevious,
    ProtoListImplementation<T> *newNext
) : Cell(context) {
    this->value = newValue;
    this->previous = newPrevious;
    this->next = newNext;
    this->hash = (newValue? newValue->getHash(context) : 0) ^
                 (newPrevious? newPrevious->hash : 0) ^
                 (newNext? newNext->hash : 0);
    this->count = (newValue? 1 : 0) + 
                  (newPrevious? newPrevious->count : 0) + 
                  (newNext? newNext->count : 0);

    unsigned long previous_height = newPrevious? newPrevious->height : 0;
    unsigned long next_height = newNext? newNext->height : 0;
    this->height = (newValue? 1 : 0) + 
                   (previous_height > next_height? previous_height : next_height);

};

template<class T> ProtoListImplementation<T>::~ProtoListImplementation() {};

template<class T> 
int getBalance(ProtoListImplementation<T> *self) {
    if (!self) {
        return 0;
    }

	if (self->next && self->previous)
		return self->next->height - self->previous->height;
	else {
        if (self->previous)
            return -self->previous->height;
        else if (self->next)
            return self->next->height;
    }

	return 0;
}

// A utility function to right rotate subtree rooted with y
// See the diagram given above.

template<class T> 
ProtoListImplementation<T> *rightRotate(ProtoContext *context, ProtoListImplementation<T> *n) {
    if (!n->previous)
        return n;

    ProtoListImplementation *newRight = new(context) ProtoListImplementation<T>(
        context,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) ProtoListImplementation<T>(
        context,
        n->previous->value,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
template<class T> 
ProtoListImplementation<T> *leftRotate(ProtoContext *context, ProtoListImplementation<T> *n) {
    if (!n->next)
        return n;

    ProtoListImplementation *newLeft = new(context) ProtoListImplementation(
        context,
        n->value,
        n->previous,
        n->next->previous
    );
    return new(context) ProtoListImplementation(
        context,
        n->next->value,
        newLeft,
        n->next->next
    );
}

template<class T> 
ProtoListImplementation<T> *rebalance(ProtoContext *context, ProtoListImplementation<T> *newNode) {
    while (TRUE) {
        int balance = getBalance(newNode);

        // If this node becomes unbalanced, then
        // there are 4 cases

        if (balance >= -1 && balance <= 1)
            return newNode;
            
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
                if (balance < 0 && newNode->previous) {
                    newNode = new(context) ProtoListImplementation(
                        context,
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
                    if (balance > 0 && newNode->next) {
                        newNode = new(context) ProtoListImplementation(
                            context,
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
T *ProtoListImplementation<T>::getAt(ProtoContext *context, int index) {
	if (!this->value) {
		return PROTO_NONE;
    }

    if (index < 0) {
        index = this->count + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned) index) >= this->count) {
        return PROTO_NONE;
    }

    ProtoListImplementation<T> *node = this;
	while (node) {
        unsigned long thisIndex = node->previous? node->previous->count : 0;
		if (((unsigned long) index) == thisIndex)
			return node->value;
        if (((unsigned long) index) < thisIndex)
            node = node->previous;
        else {
            node = node->next;
            index -= thisIndex + 1;
        }
	}

    if (node)
        return node->value;
    else
        return PROTO_NONE;
};

template<class T> 
T *ProtoListImplementation<T>::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
};

template<class T> 
T *ProtoListImplementation<T>::getLast(ProtoContext *context) {
    return this->getAt(context, -1);
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::getSlice(ProtoContext *context, int from, int to) {
    if (from < 0) {
        from = this->count + from;
        if (from < 0)
            from = 0;
    }

    if (to < 0) {
        to = this->count + to;
        if (to < 0)
            to = 0;
    }

    if (to >= from) {
        ProtoListImplementation *upperPart = this->splitLast(context, from);
        return upperPart->splitFirst(context, to - from);
    }
    else
        return new(context) ProtoListImplementation(context);
};

template<class T> 
unsigned long ProtoListImplementation<T>::getSize(ProtoContext *context) {
    return this->count;
};

template<class T> 
BOOLEAN	ProtoListImplementation<T>::has(ProtoContext *context, T *value) {
    for (unsigned long i=0; i <= this->count; i++)
        if (this->getAt(context, i) == value)
            return TRUE;

    return FALSE;
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::setAt(ProtoContext *context, int index, T *value) {
	if (!this->value) {
		return NULL;
    }

    if (index < 0) {
        index = this->count + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count) {
        return NULL;
    }

    int thisIndex = this->previous? this->previous->count : 0;
    if (thisIndex == index) {
        return new(context) ProtoListImplementation(
            context,
            value,
            this->previous,
            this->next
        );
    }

    if (index < thisIndex)
        return new(context) ProtoListImplementation(
            context,
            value,
            this->previous->setAt(context, index, value),
            this->next
        );
    else
        return new(context) ProtoListImplementation(
            context,
            value,
            this->previous,
            this->next->setAt(context, index - thisIndex - 1, value)
        );
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::insertAt(ProtoContext *context, int index, T* value) {
	if (!this->value)
		return new(context) ProtoListImplementation<T>(
            context,
            value
        );

    if (index < 0) {
        index = this->count + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count)
        index = this->count - 1;

    unsigned long thisIndex = this->previous? this->previous->count : 0;
    ProtoListImplementation *newNode;

    if (thisIndex == ((unsigned long) index))
        newNode = new(context) ProtoListImplementation<T>(
            context,
            value,
            this->previous,
            this->next
        );
    else {
        if (((unsigned long) index) < thisIndex)
            newNode = new(context) ProtoListImplementation<T>(
                context,
                value,
                this->previous->insertAt(context, index, value),
                this->next
            );
        else
            newNode = new(context) ProtoListImplementation<T>(
                context,
                value,
                this->previous,
                this->next->insertAt(context, index - thisIndex - 1, value)
            );
    }

    return rebalance(context, newNode);
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::appendFirst(ProtoContext *context, T *value) {
	if (!this->value)
		return new(context) ProtoListImplementation(
            context,
            value
        );

    ProtoListImplementation *newNode;

    if (this->previous)
        newNode = new(context) ProtoListImplementation(
            context,
            this->value,
            this->previous->appendFirst(context, value),
            this->next
        );
    else {
        newNode = new(context) ProtoListImplementation(
            context,
            this->value,
            new(context) ProtoListImplementation(
                context,
                value
            ),
            this->next
        );
    }

    return rebalance(context, newNode);

};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::appendLast(ProtoContext *context, T *value) {
	if (!this->value)
		return new(context) ProtoListImplementation<T>(
            context,
            value
        );

    ProtoListImplementation *newNode;

    if (this->next) {
        newNode = new(context) ProtoListImplementation<T>(
            context,
            this->value,
            this->previous,
            this->next->appendLast(context, value)
        );
    }
    else {
        newNode = new(context) ProtoListImplementation<T>(
            context,
            this->value,
            this->previous,
            new(context) ProtoListImplementation(
                context,
                value
            )
        );
    }

    return rebalance(context, newNode);
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::extend(ProtoContext *context, ProtoList<T> *other) {
    if (this->count == 0)
        return (ProtoListImplementation *) other;

    unsigned long otherCount = other->getSize(context);

    if (otherCount == 0)
        return this;

    if (this->count < otherCount)
        return rebalance(
            context, 
            new(context) ProtoListImplementation(
                context,
                this->getLast(context),
                this->removeLast(context),
                (ProtoListImplementation *) other
            ));
    else
        return rebalance(
            context, 
            new(context) ProtoListImplementation(
                context,
                other->getFirst(context),
                this,
                (ProtoListImplementation *) other->removeFirst(context)
            ));
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::splitFirst(ProtoContext *context, int index) {
	if (!this->value)
        return this;

    if (index < 0) {
        index = (int) this->count + index;
        if (index < 0)
            index = 0;
    }

    if (index >= (int) this->count)
        index = (int) this->count - 1;

    if (index == (int) this->count - 1)
        return this;

    if (index == 0)
        return new(context) ProtoListImplementation(context);

    ProtoListImplementation *newNode = NULL;

    int thisIndex = (this->previous? this->previous->count : 0);

    if (thisIndex == index)
        return this->previous;
    else {
        if (index > thisIndex) {
            ProtoListImplementation *newNext = this->next->splitFirst(context, (unsigned long) index - thisIndex - 1);
            if (newNext->count == 0)
                newNext = NULL;
            newNode = new(context) ProtoListImplementation(
                context,
                this->value,
                this->previous,
                newNext
            );
        }
        else {
            if (this->previous)
                return this->previous->splitFirst(context, index);
            else
                newNode = new(context) ProtoListImplementation(
                    context,
                    value,
                    NULL,
                    this->next->splitFirst(context, index - thisIndex - 1)
                );
        }
    }

    return rebalance(context, newNode);
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::splitLast(ProtoContext *context, int index) {
	if (!this->value)
        return this;

    if (index < 0) {
        index = this->count + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count)
        index = this->count - 1;

    if (index == 0)
        return this;

    ProtoListImplementation *newNode = NULL;

    int thisIndex = (this->previous? this->previous->count : 0);

    if (thisIndex == index) {
        if (!this->previous)
            return this;
        else {
            newNode = new(context) ProtoListImplementation(
                context,
                value,
                NULL,
                this->next
            );
        }
    }
    else {
        if (index < thisIndex) {
            newNode = new(context) ProtoListImplementation(
                context,
                this->value,
                this->previous->splitLast(context, index),
                this->next
            );
        }
        else {
            if (!this->next)
                // It should not happen!
                return new(context) ProtoListImplementation(context);
            else {
                return this->next->splitLast(
                    context, 
                    ((unsigned long) index - thisIndex - 1)
                );
            }
        }
    }

    return rebalance(context, newNode);
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::removeFirst(ProtoContext *context) {
	if (!this->value)
        return this;

    ProtoListImplementation *newNode;

    if (this->previous) {
        newNode = this->previous->removeFirst(context);
        if (newNode->value == NULL)
            newNode = NULL; 
        newNode = new(context) ProtoListImplementation(
            context,
            value,
            newNode,
            this->next
        );
    }
    else {
        if (this->next)
            return this->next;
        
        newNode = new(context) ProtoListImplementation(
            context,
            NULL,
            NULL,
            NULL
        );
    }

    return rebalance(context, newNode);
};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::removeLast(ProtoContext *context) {
	if (!this->value)
        return this;

    ProtoListImplementation *newNode;

    if (this->next) {
        newNode = this->next->removeLast(context);
        if (newNode->value == NULL)
            newNode = NULL; 
        newNode = new(context) ProtoListImplementation(
            context,
            value,
            this->previous,
            newNode
        );
    }
    else {
        if (this->previous)
            return this->previous;
        
        newNode = new(context) ProtoListImplementation(
            context,
            NULL,
            NULL,
            NULL
        );
    }

    return rebalance(context, newNode);

};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::removeAt(ProtoContext *context, int index) {
	if (!this->value) {
		return new(context) ProtoListImplementation(
            context
        );
    }

    if (index < 0) {
        index = this->count + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count)
        return this;

    unsigned long thisIndex = this->previous? this->previous->count : 0;
    ProtoListImplementation *newNode;

    if (thisIndex == ((unsigned long) index)) {
        if (this->next) {
            newNode = this->next->removeFirst(context);
            if (newNode->count == 0)
                newNode = NULL;
            newNode = new(context) ProtoListImplementation(
                context,
                this->next->getFirst(context),
                this->previous,
                newNode
            );
        }
        else
            return this->previous;
    }
    else {
        if (((unsigned long) index) < thisIndex) {
            newNode = this->previous->removeAt(context, index);
            if (newNode->count == 0)
                newNode = NULL;
            newNode = new(context) ProtoListImplementation(
                context,
                value,
                newNode,
                this->next
            );
        }
        else {
            newNode = this->next->removeAt(context, index - thisIndex - 1);
            if (newNode->count == 0)
                newNode = NULL;
            newNode = new(context) ProtoListImplementation(
                context,
                value,
                this->previous,
                newNode
            );
        }
    }

    return rebalance(context, newNode);

};

template<class T> 
ProtoListImplementation<T> *ProtoListImplementation<T>::removeSlice(ProtoContext *context, int from, int to) {
    if (from < 0) {
        from = this->count + from;
        if (from < 0)
            from = 0;
    }

    if (to < 0) {
        to = this->count + to;
        if (to < 0)
            to = 0;
    }

    ProtoListImplementation *slice = new(context) ProtoListImplementation(context);
    if (to >= from) {
        return this->splitFirst(context, from)->extend(
            context,
            this->splitLast(context, from)
        );
    }
    else
        return this;

    return slice;
};

template<class T> 
ProtoTuple<T> *ProtoListImplementation<T>::asTuple(ProtoContext *context) {
    return ProtoTupleImplementation::tupleFromList(context, this);
};

template<class T> 
ProtoObject *ProtoListImplementation<T>::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST;

    return p.oid.oid;
};

template<class T> 
unsigned long ProtoListImplementation<T>::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

template<class T> 
ProtoListIteratorImplementation<T> *ProtoListImplementation<T>::getIterator(ProtoContext *context) {
    return new(context) ProtoListIteratorImplementation(context, NULL, 0);
}

template<class T> 
void ProtoListImplementation<T>::finalize(ProtoContext *context) {};

template<class T> 
void ProtoListImplementation<T>::processReferences(
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

    if (this->value)
        this->value->processReferences(context, self, method);
};

};
