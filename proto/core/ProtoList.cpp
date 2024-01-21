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

ProtoListIteratorImplementation::ProtoListIteratorImplementation(
		ProtoContext *context,
        ProtoListImplementation *base,
		unsigned long currentIndex
	) : Cell(context) {
    this->base = base;
	this->currentIndex = currentIndex;
};

ProtoListIteratorImplementation::~ProtoListIteratorImplementation() {};

int ProtoListIteratorImplementation::hasNext(ProtoContext *context) {
    if (this->currentIndex >= this->base->getSize(context))
        return FALSE;
    else
        return TRUE;
};

ProtoObject *ProtoListIteratorImplementation::next(ProtoContext *context) {
    return this->base->getAt(context, this->currentIndex);
};

ProtoListIteratorImplementation *ProtoListIteratorImplementation::advance(ProtoContext *context) {
    return new(context) ProtoListIteratorImplementation(context, this->base, this->currentIndex);
};

ProtoObject	  *ProtoListIteratorImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;

    return p.oid.oid;
};

void ProtoListIteratorImplementation::finalize(ProtoContext *context) {};

void ProtoListIteratorImplementation::processReferences(
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


ProtoListImplementation::ProtoListImplementation(
    ProtoContext *context,

    ProtoObject *newValue,
    ProtoListImplementation *newPrevious,
    ProtoListImplementation *newNext
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

ProtoListImplementation::~ProtoListImplementation() {

};

int getBalance(ProtoListImplementation *self) {
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
ProtoListImplementation *rightRotate(ProtoContext *context, ProtoListImplementation *n) {
    if (!n->previous)
        return n;

    ProtoListImplementation *newRight = new(context) ProtoListImplementation(
        context,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) ProtoListImplementation(
        context,
        n->previous->value,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
ProtoListImplementation *leftRotate(ProtoContext *context, ProtoListImplementation *n) {
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

ProtoListImplementation *rebalance(ProtoContext *context, ProtoListImplementation *newNode) {
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

ProtoObject *ProtoListImplementation::getAt(ProtoContext *context, int index) {
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

    ProtoListImplementation *node = this;
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

ProtoObject *ProtoListImplementation::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
};

ProtoObject *ProtoListImplementation::getLast(ProtoContext *context) {
    return this->getAt(context, -1);
};

ProtoListImplementation *ProtoListImplementation::getSlice(ProtoContext *context, int from, int to) {
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

unsigned long ProtoListImplementation::getSize(ProtoContext *context) {
    return this->count;
};

BOOLEAN	ProtoListImplementation::has(ProtoContext *context, ProtoObject* value) {
    for (unsigned long i=0; i <= this->count; i++)
        if (this->getAt(context, i) == value)
            return TRUE;

    return FALSE;
};

ProtoListImplementation *ProtoListImplementation::setAt(ProtoContext *context, int index, ProtoObject* value) {
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

ProtoListImplementation *ProtoListImplementation::insertAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!this->value)
		return new(context) ProtoListImplementation(
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
        newNode = new(context) ProtoListImplementation(
            context,
            value,
            this->previous,
            this->next
        );
    else {
        if (((unsigned long) index) < thisIndex)
            newNode = new(context) ProtoListImplementation(
                context,
                value,
                this->previous->insertAt(context, index, value),
                this->next
            );
        else
            newNode = new(context) ProtoListImplementation(
                context,
                value,
                this->previous,
                this->next->insertAt(context, index - thisIndex - 1, value)
            );
    }

    return rebalance(context, newNode);
};

ProtoListImplementation *ProtoListImplementation::appendFirst(ProtoContext *context, ProtoObject* value) {
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

ProtoListImplementation *ProtoListImplementation::appendLast(ProtoContext *context, ProtoObject* value) {
	if (!this->value)
		return new(context) ProtoListImplementation(
            context,
            value
        );

    ProtoListImplementation *newNode;

    if (this->next) {
        newNode = new(context) ProtoListImplementation(
            context,
            this->value,
            this->previous,
            this->next->appendLast(context, value)
        );
    }
    else {
        newNode = new(context) ProtoListImplementation(
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

ProtoListImplementation *ProtoListImplementation::extend(ProtoContext *context, ProtoList *other) {
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

ProtoListImplementation *ProtoListImplementation::splitFirst(ProtoContext *context, int index) {
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

ProtoListImplementation *ProtoListImplementation::splitLast(ProtoContext *context, int index) {
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

ProtoListImplementation *ProtoListImplementation::removeFirst(ProtoContext *context) {
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

ProtoListImplementation *ProtoListImplementation::removeLast(ProtoContext *context) {
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

ProtoListImplementation *ProtoListImplementation::removeAt(ProtoContext *context, int index) {
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

ProtoListImplementation *ProtoListImplementation::removeSlice(ProtoContext *context, int from, int to) {
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

ProtoTuple *ProtoListImplementation::asTuple(ProtoContext *context) {
    return ProtoTupleImplementation::tupleFromList(context, this);
};

ProtoObject *ProtoListImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST;

    return p.oid.oid;
};

unsigned long ProtoListImplementation::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

ProtoListIteratorImplementation *ProtoListImplementation::getIterator(ProtoContext *context) {
    return new(context) ProtoListIteratorImplementation(context, NULL, 0);
}

void ProtoListImplementation::finalize(ProtoContext *context) {};

void ProtoListImplementation::processReferences(
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
