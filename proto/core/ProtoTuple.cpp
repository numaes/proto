/*
 * list.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include <string.h>

using namespace std;
namespace proto {


#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

ProtoTuple::ProtoTuple(
    ProtoContext *context,
    unsigned long element_count,
    ProtoIndirectTuple *continuation,
    ProtoObject **data
) : Cell(context) {
    this->element_count = element_count;
    this->continuation = continuation;
    for (int i = 0; i < 4; i++) {
        if (i < element_count)
            this->data[i] = data[i];
        else 
            this->data[i] = NULL;
    }
};

ProtoTuple::~ProtoTuple() {

};

ProtoObject *ProtoTuple::getAt(ProtoContext *context, int index) {
    if (index < 0) {
        index = this->element_count + index;
        if (index < 0)
            index = 0;
    }

    if (index < this->element_count)
        return this->data[element_count];
    else {
        ProtoTuple *first_indirection = (ProtoTuple *) this->continuation->getAt(context, (index - 4) / 4); 
        return first_indirection->data[(index - 4) % 4];
    }
};

ProtoObject *ProtoTuple::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
};

ProtoObject *ProtoTuple::getLast(ProtoContext *context) {
    return this->getAt(context, -1);
};

ProtoTuple *ProtoTuple::getSlice(ProtoContext *context, int from, int to) {
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
        ProtoList *upperPart = this->splitLast(context, from);
        return upperPart->splitFirst(context, to - from);
    }
    else
        return new(context) ProtoList(context);
};

unsigned long ProtoTuple::getSize(ProtoContext *context) {
    return this->count;
};

BOOLEAN	ProtoTuple::has(ProtoContext *context, ProtoObject* value) {
    for (unsigned long i=0; i <= this->count; i++)
        if (this->getAt(context, i) == value)
            return TRUE;

    return FALSE;
};

ProtoTuple *ProtoTuple::setAt(ProtoContext *context, int index, ProtoObject* value) {
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

ProtoTuple *ProtoTuple::insertAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!this->value)
		return new(context) ProtoList(
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

ProtoTuple *ProtoTuple::appendFirst(ProtoContext *context, ProtoObject* value) {
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

ProtoTuple *ProtoTuple::appendLast(ProtoContext *context, ProtoObject* value) {
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

ProtoTuple *ProtoTuple::extend(ProtoContext *context, ProtoList *other) {
    if (this->count == 0)
        return other;

    if (other->count == 0)
        return this;

    if (this->count < other->count)
        return rebalance(
            context, 
            new(context) ProtoList(
                context,
                this->getLast(context),
                this->removeLast(context),
                other
            ));
    else
        return rebalance(
            context, 
            new(context) ProtoList(
                context,
                other->getFirst(context),
                this,
                other->removeFirst(context)
            ));
};

ProtoTuple *ProtoTuple::splitFirst(ProtoContext *context, int index) {
	if (!this->value)
        return this;

    if (index < 0) {
        index = this->count + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= this->count)
        index = this->count - 1;

    if (index == this->count - 1)
        return this;

    if (index == 0)
        return new(context) ProtoList(context);

    ProtoList *newNode = NULL;

    int thisIndex = (this->previous? this->previous->count : 0);

    if (thisIndex == ((unsigned long) index))
        return this->previous;
    else {
        if (index > thisIndex) {
            ProtoList *newNext = this->next->splitFirst(context, index - thisIndex - 1);
            if (newNext->count == 0)
                newNext = NULL;
            newNode = new(context) ProtoList(
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
                newNode = new(context) ProtoList(
                    context,
                    value,
                    NULL,
                    this->next->splitFirst(context, index - thisIndex - 1)
                );
        }
    }

    return rebalance(context, newNode);
};

ProtoTuple *ProtoTuple::splitLast(ProtoContext *context, int index) {
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

    ProtoList *newNode = NULL;

    int thisIndex = (this->previous? this->previous->count : 0);

    if (thisIndex == ((unsigned long) index)) {
        if (!this->previous)
            return this;
        else {
            newNode = new(context) ProtoList(
                context,
                value,
                NULL,
                this->next
            );
        }
    }
    else {
        if (((unsigned long) index) < thisIndex) {
            newNode = new(context) ProtoList(
                context,
                this->value,
                this->previous->splitLast(context, index),
                this->next
            );
        }
        else {
            if (!this->next)
                // It should not happen!
                return new(context) ProtoList(context);
            else {
                return this->next->splitLast(
                    context, 
                    ((unsigned long) index - thisIndex)
                );
            }
        }
    }

    return rebalance(context, newNode);
};

ProtoTuple *ProtoTuple::removeFirst(ProtoContext *context) {
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
        if (this->next)
            return this->next;
        
        newNode = new(context) ProtoList(
            context,
            NULL,
            NULL,
            NULL
        );
    }

    return rebalance(context, newNode);
};

ProtoTuple *ProtoTuple::removeLast(ProtoContext *context) {
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
        if (this->previous)
            return this->previous;
        
        newNode = new(context) ProtoList(
            context,
            NULL,
            NULL,
            NULL
        );
    }

    return rebalance(context, newNode);

};

ProtoTuple *ProtoTuple::removeAt(ProtoContext *context, int index) {
	if (!this->value) {
		return new(context) ProtoList(
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
    ProtoList *newNode;

    if (thisIndex == ((unsigned long) index)) {
        if (this->next) {
            newNode = this->next->removeFirst(context);
            if (newNode->count == 0)
                newNode = NULL;
            newNode = new(context) ProtoList(
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
            newNode = new(context) ProtoList(
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
            newNode = new(context) ProtoList(
                context,
                value,
                this->previous,
                newNode
            );
        }
    }

    return rebalance(context, newNode);

};

ProtoTuple *ProtoTuple::removeSlice(ProtoContext *context, int from, int to) {
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

    ProtoList *slice = new(context) ProtoList(context);
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

int ProtoTuple::fillUTF8Buffer(ProtoContext *context, char *buffer, size_t size) {
    char singleCharBuffer[5];

    unsigned fillCount = 0;
    unsigned index = 0;

    while (fillCount < size) {
        ProtoObject *unicodeChar = this->getAt(context, index);
        size_t unicodeCharSize;

        ProtoObjectPointer p;
        p.oid.oid = unicodeChar;
        if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
            p.op.embedded_type == EMBEDED_TYPE_UNICODECHAR) {

            if (p.unicodeChar.unicodeValue <= 0x7f) {
                singleCharBuffer[0] = (char) p.unicodeChar.unicodeValue;
                singleCharBuffer[1] = 0;
                unicodeCharSize = 1;
            }
            else {
                if (p.unicodeChar.unicodeValue >= 0x80 &&
                    p.unicodeChar.unicodeValue <= 0x7ff) {
                    long u = p.unicodeChar.unicodeValue - 0x80;
                    singleCharBuffer[0] = (char) (u >> 6) & 0x07;
                    singleCharBuffer[1] = (char) u & 0x3F;
                    singleCharBuffer[2] = 0;
                    unicodeCharSize = 2;
                }
                else {
                    if (p.unicodeChar.unicodeValue >= 0x800 &&
                        p.unicodeChar.unicodeValue <=0x10ffff) {
                        long u = p.unicodeChar.unicodeValue - 0x800;
                        singleCharBuffer[0] = (char) (u >> 12) & 0x07;
                        singleCharBuffer[1] = (char) (u >> 6) & 0x3F;
                        singleCharBuffer[2] = (char) u & 0x3F;
                        singleCharBuffer[3] = 0;
                        unicodeCharSize = 3;
                    }
                    else {
                        long u = p.unicodeChar.unicodeValue - 0x10000;
                        singleCharBuffer[0] = (char) (u >> 18) & 0x07;
                        singleCharBuffer[1] = (char) (u >> 12) & 0x3F;
                        singleCharBuffer[2] = (char) (u >> 12) & 0x3F;
                        singleCharBuffer[3] = (char) u & 0x3F;
                        singleCharBuffer[4] = 0;
                        unicodeCharSize = 4;
                    }
                }
            }
        }
        // fill single char buffer

        if ((fillCount + unicodeCharSize) < size) {
            strncpy(buffer, singleCharBuffer, unicodeCharSize);
            buffer += unicodeCharSize;
            fillCount += unicodeCharSize;
            index += 1;
        }
        else
            break;
    };

    if (size)
        *buffer = 0;

    return index;    
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
