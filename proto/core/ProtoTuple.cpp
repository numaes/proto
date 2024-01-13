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

// TODO Redefinir la implementación de Tuplas

ProtoTupleIterator::ProtoTupleIterator(
		ProtoContext *context,
        ProtoTuple *base,
		unsigned long currentIndex
	) : Cell(context) {
    this->base = base;
	this->currentIndex = currentIndex;
};

ProtoTupleIterator::~ProtoTupleIterator() {};

int ProtoTupleIterator::hasNext(ProtoContext *context) {
    if (this->currentIndex >= this->base->getSize(context))
        return FALSE;
    else
        return TRUE;
};

ProtoObject *ProtoTupleIterator::next(ProtoContext *context) {
    return this->base->getAt(context, this->currentIndex);
};

ProtoTupleIterator *ProtoTupleIterator::advance(ProtoContext *context) {
    return new(context) ProtoTupleIterator(context, this->base, this->currentIndex);
};

ProtoObject	  *ProtoTupleIterator::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;

    return p.oid.oid;
};

void ProtoTupleIterator::finalize(ProtoContext *context) {};

void ProtoTupleIterator::processReferences(
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

ProtoTuple::ProtoTuple(
    ProtoContext *context,
    unsigned long elementCount,
    unsigned long height,
    ProtoObject **data,
    ProtoTuple **indirect
) : Cell(context) {
    this->elementCount = elementCount;
    this->height = height;
    if (height > 0) {
        for (int i = 0; i < TUPLE_SIZE; i++)
            this->pointers.indirect[i] = indirect[i]; 
    }
    else {
        for (int i = 0; i < TUPLE_SIZE; i++)
            this->pointers.data[i] = data[i]; 
    }
};

ProtoTuple::~ProtoTuple() {};

ProtoObject   *ProtoTuple::getAt(ProtoContext *context, int index) {
    if (index < 0)
        index = ((int) this->elementCount) + index;

    if (index < 0)
        index = 0;

    int rest = index % TUPLE_SIZE;
    ProtoTuple *node = this;
    for (int i = this->height; i > 0; i--) {
        index = index / TUPLE_SIZE;
        node = node->pointers.indirect[index];
    }

    return node->pointers.data[rest];
};

ProtoObject   *ProtoTuple::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
};

ProtoObject   *ProtoTuple::getLast(ProtoContext *context) {
    if (this->elementCount > 0)
        return this->getAt(context, this->elementCount - 1);
    
    return PROTO_NONE;
};

ProtoTuple *ProtoTuple::getSlice(ProtoContext *context, int from, int to) {
    int thisSize = this->elementCount;
    if (from < 0) {
        from = thisSize + from;
        if (from < 0)
            from = 0;
    }

    if (to < 0) {
        to = thisSize + to;
        if (to < 0)
            to = 0;
    }

    ProtoList *sourceList = context->newList();
    for (int i = from; i <= to; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    
};

unsigned long  ProtoTuple::getSize(ProtoContext *context) {
    return this->elementCount;
};

BOOLEAN ProtoTuple::has(ProtoContext *context, ProtoObject* value) {
    for (unsigned long i = 0; i < this->elementCount; i++)
        if (value == this->getAt(context, i))
            return TRUE;
    
    return FALSE;
};

ProtoTuple *ProtoTuple::setAt(ProtoContext *context, int index, ProtoObject* value) {
  	if (!value) {
		return NULL;
    }

    int thisSize = this->elementCount;

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (index >= thisSize) {
        return NULL;
    }

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    sourceList = sourceList->appendLast(context, value);

    for (int i = index + 1; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::insertAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!value) {
		return NULL;
    }

    int thisSize = this->elementCount;

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (index >= thisSize) {
        return NULL;
    }

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    sourceList = sourceList->appendLast(context, value);

    for (int i = index; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::appendFirst(ProtoContext *context, ProtoTuple* otherTuple) {
	if (!otherTuple) {
		return NULL;
    }

    int thisSize = this->elementCount;

    ProtoList *sourceList = context->newList();
    int otherSize = otherTuple->getSize(context);
    for (int i = 0; i < otherSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    for (int i = 0; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    

};

ProtoTuple 	  *ProtoTuple::appendLast(ProtoContext *context, ProtoTuple *otherTuple) {
	if (!otherTuple) {
		return NULL;
    }

    int thisSize = this->elementCount;

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    int otherSize = otherTuple->getSize(context);
    for (int i = 0; i < otherSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::splitFirst(ProtoContext *context, int count) {
    int thisSize = this->elementCount;

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::splitLast(ProtoContext *context, int count) {
    int thisSize = this->elementCount;
    int first = thisSize - count;
    if (first < 0)
        first = 0;

    ProtoList *sourceList = context->newList();
    for (int i = first; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::removeFirst(ProtoContext *context, int count) {
    int thisSize = this->elementCount;

    ProtoList *sourceList = context->newList();
    for (int i = count; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::removeLast(ProtoContext *context, int count) {
    int thisSize = this->elementCount;

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < thisSize - count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        else
            break;

    return context->tupleFromList(sourceList);

};

ProtoTuple *ProtoTuple::removeAt(ProtoContext *context, int index) {
    int thisSize = this->elementCount;

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (index >= thisSize) {
        return NULL;
    }

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    for (int i = index + 1; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return context->tupleFromList(sourceList);    

};

ProtoTuple *ProtoTuple::removeSlice(ProtoContext *context, int from, int to) {
    int thisSize = this->elementCount;

    if (from < 0) {
        from = thisSize + from;
        if (from < 0)
            from = 0;
    }

    if (to < 0) {
        to = thisSize + from;
        if (to < 0)
            to = 0;
    }

    ProtoList *sourceList = context->newList();
    for (int i = from; i < to; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        else
            break;

    return context->tupleFromList(sourceList);    

};

ProtoList *ProtoTuple::asList(ProtoContext *context) {
    ProtoList *sourceList = context->newList();
    for (unsigned long i = 0; i < this->elementCount; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return sourceList;    

};

void ProtoTuple::finalize(ProtoContext *context) {
    TupleDictionary *newRoot, *currentRoot;
    do {
        currentRoot = context->space->tupleRoot.load();
        newRoot = currentRoot->removeAt(context, this);
    } while (context->space->tupleRoot.compare_exchange_strong(
        currentRoot,
        newRoot
    ));
};

void ProtoTuple::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    int size = (this->elementCount > TUPLE_SIZE)? TUPLE_SIZE : this->elementCount;
    for (int i = 0; i < size; i++)
        if (this->height > 0)
            method(context, self, this->pointers.indirect[i]);
        else {
            if (this->pointers.data[i]->isCell(context))
                method(context, self, this->pointers.data[i]->asCell(context));
        }
};

ProtoObject *ProtoTuple::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_TUPLE;

    return p.oid.oid;
};

unsigned long ProtoTuple::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

};
