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

ProtoStringIterator::ProtoStringIterator(
		ProtoContext *context,
        ProtoString *base,
		unsigned long currentIndex
	) : Cell(context) {
    this->base = base;
	this->currentIndex = currentIndex;
};

ProtoStringIterator::~ProtoStringIterator() {};

int ProtoStringIterator::hasNext(ProtoContext *context) {
    if (this->currentIndex >= this->base->getSize(context))
        return FALSE;
    else
        return TRUE;
};

ProtoObject *ProtoStringIterator::next(ProtoContext *context) {
    return this->base->getAt(context, this->currentIndex);
};

ProtoStringIterator *ProtoStringIterator::advance(ProtoContext *context) {
    return new(context) ProtoStringIterator(context, this->base, this->currentIndex);
};

ProtoObject	  *ProtoStringIterator::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;

    return p.oid.oid;
};

void ProtoStringIterator::finalize(ProtoContext *context) {};

void ProtoStringIterator::processReferences(
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

ProtoString::ProtoString(
    ProtoContext *context,
    ProtoTuple *baseTuple
) : Cell(context) {
    this->baseTuple = baseTuple;
};

ProtoTuple::~ProtoTuple() {

};

ProtoObject *ProtoString::getAt(ProtoContext *context, int index) {
    return this->baseTuple->getAt(context, index);
};

ProtoString *ProtoString::getSlice(ProtoContext *context, int from, int to) {
    int thisSize = this->baseTuple->getSize(context);
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

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

};

unsigned long ProtoString::getSize(ProtoContext *context) {
    return this->baseTuple->getSize(context);
};

ProtoString *ProtoString::setAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!value) {
		return NULL;
    }

    int thisSize = this->baseTuple->getSize(context);

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= thisSize) {
        return NULL;
    }

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    sourceList = sourceList->appendLast(context, value);

    for (int i = index + 1; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

}

ProtoString *ProtoString::insertAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!value) {
		return NULL;
    }

    int thisSize = this->baseTuple->getSize(context);

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= thisSize) {
        return NULL;
    }

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    sourceList = sourceList->appendLast(context, value);

    for (int i = index; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

}

ProtoString *ProtoString::appendFirst(ProtoContext *context, ProtoString* otherString) {
	if (!otherString) {
		return NULL;
    }

    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    int otherSize = otherString->getSize(context);
    for (int i = 0; i < otherSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    for (int i = 0; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

};

ProtoString *ProtoString::appendLast(ProtoContext *context, ProtoString* otherString) {
	if (!otherString) {
		return NULL;
    }

    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    int otherSize = otherString->getSize(context);
    for (int i = 0; i < otherSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

};

ProtoString *ProtoString::splitFirst(ProtoContext *context, int count) {
    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoString::splitLast(ProtoContext *context, int count) {
    int thisSize = this->baseTuple->getSize(context);
    int first = thisSize - count;
    if (first < 0)
        first = 0;

    ProtoList *sourceList = context->newList();
    for (int i = first; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoString::removeFirst(ProtoContext *context, int count = 1) {
    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = count; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoString::removeLast(ProtoContext *context, int count = 1) {
    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < thisSize - count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        else
            break;

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoString::removeAt(ProtoContext *context, int index) {
    int thisSize = this->baseTuple->getSize(context);

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (((unsigned long) index) >= thisSize) {
        return NULL;
    }

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    for (int i = index + 1; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
}

ProtoString *ProtoString::removeSlice(ProtoContext *context, int from, int to) {
    int thisSize = this->baseTuple->getSize(context);

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

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
}

void finalize() {};

void ProtoString::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    method(context, self, this->baseTuple);
};

ProtoObject *ProtoString::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_STRING;

    return p.oid.oid;
};

unsigned long ProtoString::getHash(ProtoContext *context) {
    return (unsigned long) this->baseTuple;
};


};
