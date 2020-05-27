/*
 * list.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"

using namespace std;
namespace proto {


#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

ProtoList::ProtoList(
    ProtoContext *context,

    ProtoObject *value,
    ProtoList *previous,
    ProtoList *next
) : Cell(
    context,
    type = CELL_TYPE_PROTO_LIST,
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

ProtoList::~ProtoList() {

};

int getBalance(ProtoList *self) {
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
ProtoList *rightRotate(ProtoContext *context, ProtoList *n)
{
    ProtoList *newRight = new(context) ProtoList(
        context,
        n->value,
        n->previous->next,
        n->next
    );
    return new(context) ProtoList(
        context,
        n->previous->value,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
ProtoList *leftRotate(ProtoContext *context, ProtoList *n) {
    ProtoList *newLeft = new(context) ProtoList(
        context,
        n->value,
        n->previous,
        n->next->previous
    );
    return new(context) ProtoList(
        context,
        n->next->value,
        newLeft,
        n->next->next
    );
}

ProtoList *rebalance(ProtoContext *context, ProtoList *newNode) {
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
                    newNode = new(context) ProtoList(
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
                        newNode = new(context) ProtoList(
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
}

ProtoObject *ProtoList::getAt(ProtoContext *context, int index) {
	if (!this->value) {
		return PROTO_NONE;
    }

    if (index < 0) {
        index = this->count - index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned) index) >= this->count) {
        return PROTO_NONE;
    }

    ProtoList *node = this;
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

ProtoObject *ProtoList::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
};

ProtoObject *ProtoList::getLast(ProtoContext *context) {
    return this->getAt(context, -1);
};

ProtoList *ProtoList::getSlice(ProtoContext *context, int from, int to) {
    if (from < 0) {
        from = this->count - from;
        if (from < 0)
            from = 0;
    }

    if (to < 0) {
        to = this->count - to;
        if (to < 0)
            to = 0;
    }

    ProtoList *slice = new(context) ProtoList(context);
    if (to >= from)
        for (int i=from; i <= to; i++)
            slice = slice->appendLast(context, this->getAt(context, i));

    return slice;
};

unsigned long ProtoList::getSize(ProtoContext *context) {
    return this->count;
};

BOOLEAN	ProtoList::has(ProtoContext *context, ProtoObject* value) {
    for (unsigned long i=0; i <= this->count; i++)
        if (this->getAt(context, i) == value)
            return TRUE;

    return FALSE;
};

ProtoList *ProtoList::setAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!this->value) {
		return NULL;
    }

    if (index < 0) {
        index = this->count - index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count) {
        return NULL;
    }

    int thisIndex = this->previous? this->previous->count : 0;
    if (thisIndex == index) {
        return new(context) ProtoList(
            context,
            value,
            this->previous,
            this->next
        );
    }

    if (index < thisIndex)
        return new(context) ProtoList(
            context,
            value,
            this->previous->setAt(context, index, value),
            this->next
        );
    else
        return new(context) ProtoList(
            context,
            value,
            this->previous,
            this->next->setAt(context, index - thisIndex - 1, value)
        );
};

ProtoList *ProtoList::insertAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!this->value)
		return new(context) ProtoList(
            context,
            value
        );

    if (index < 0) {
        index = this->count - index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count)
        index = this->count - 1;

    unsigned long thisIndex = this->previous? this->previous->count : 0;
    ProtoList *newNode;

    if (thisIndex == ((unsigned long) index))
        newNode = new(context) ProtoList(
            context,
            value,
            this->previous,
            this->next
        );
    else {
        if (((unsigned long) index) < thisIndex)
            newNode = new(context) ProtoList(
                context,
                value,
                this->previous->insertAt(context, index, value),
                this->next
            );
        else
            newNode = new(context) ProtoList(
                context,
                value,
                this->previous,
                this->next->insertAt(context, index - thisIndex - 1, value)
            );
    }

    return rebalance(context, newNode);
};

ProtoList *ProtoList::appendFirst(ProtoContext *context, ProtoObject* value) {
	if (!this->value)
		return new(context) ProtoList(
            context,
            value
        );

    ProtoList *newNode;

    if (this->previous)
        newNode = new(context) ProtoList(
            context,
            this->value,
            this->previous->appendFirst(context, value),
            this->next
        );
    else {
        newNode = new(context) ProtoList(
            context,
            this->value,
            new(context) ProtoList(
                context,
                value
            ),
            this->next
        );
    }

    return rebalance(context, newNode);

};

ProtoList *ProtoList::appendLast(ProtoContext *context, ProtoObject* value) {
	if (!this->value)
		return new(context) ProtoList(
            context,
            value
        );

    ProtoList *newNode;

    if (this->next) {
        newNode = new(context) ProtoList(
            context,
            this->value,
            this->previous,
            this->next->appendLast(context, value)
        );
    }
    else {
        newNode = new(context) ProtoList(
            context,
            this->value,
            this->previous,
            new(context) ProtoList(
                context,
                value
            )
        );
    }

    return rebalance(context, newNode);
};

ProtoList *ProtoList::removeFirst(ProtoContext *context) {
	if (!this->value)
        return this;

    ProtoList *newNode;

    if (this->previous) {
        newNode = this->previous->removeFirst(context);
        if (newNode->value == NULL)
            newNode = NULL; 
        newNode = new(context) ProtoList(
            context,
            value,
            newNode,
            this->next
        );
    }
    else {
        newNode = new(context) ProtoList(
            context,
            this->next? this->next->getFirst(context) : NULL,
            NULL,
            this->next->removeFirst(context)
        );
    }

    return rebalance(context, newNode);
};

ProtoList *ProtoList::removeLast(ProtoContext *context) {
	if (!this->value)
        return this;

    ProtoList *newNode;

    if (this->next) {
        newNode = this->next->removeLast(context);
        if (newNode->value == NULL)
            newNode = NULL; 
        newNode = new(context) ProtoList(
            context,
            value,
            this->previous,
            newNode
        );
    }
    else {
        newNode = new(context) ProtoList(
            context,
            this->previous? this->previous->getLast(context) : NULL,
            NULL,
            this->next->removeLast(context)
        );
    }

    return rebalance(context, newNode);

};

ProtoList *ProtoList::removeAt(ProtoContext *context, int index) {
	if (!this->value) {
		return new(context) ProtoList(
            context
        );
    }

    if (index < 0) {
        index = this->count - index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count)
        return this;

    unsigned long thisIndex = this->previous? this->previous->count : 0;
    ProtoList *newNode;

    if (thisIndex == ((unsigned long) index)) {
        if (this->next)
            newNode = new(context) ProtoList(
                context,
                this->next->getFirst(context),
                this->previous,
                this->next->removeFirst(context)
            );
        else
            return this->previous;
    }
    else {
        if (((unsigned long) index) < thisIndex)
            newNode = new(context) ProtoList(
                context,
                value,
                this->previous->removeAt(context, index),
                this->next
            );
        else
            newNode = new(context) ProtoList(
                context,
                value,
                this->previous,
                this->next->removeAt(context, index - thisIndex - 1)
            );
    }

    return rebalance(context, newNode);

};

ProtoList *ProtoList::removeSlice(ProtoContext *context, int from, int to) {
    if (from < 0) {
        from = this->count - from;
        if (from < 0)
            from = 0;
    }

    if (to < 0) {
        to = this->count - to;
        if (to < 0)
            to = 0;
    }

    ProtoList *slice = new(context) ProtoList(context);
    if (to >= from) {
        for (unsigned long i=0; i <= this->count; i++)
            if (i < ((unsigned long) from) || i >= ((unsigned long) to))
                slice = slice->appendLast(context, this->getAt(context, i));
    }
    else
        return this;

    return slice;
};

void ProtoList::processReferences(
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
    p.oid.oid = this->value;
    if (p.op.pointer_tag == POINTER_TAG_CELL) {
        method(context, self, p.cell.cell);
    }

    if (this->next)
        this->next->processReferences(context, self, method);

    method(context, self, this);
};

};
