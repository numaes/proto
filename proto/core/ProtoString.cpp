/*
 * list.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"
#include <string.h>

using namespace std;
namespace proto {


#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

ProtoStringIteratorImplementation::ProtoStringIteratorImplementation(
		ProtoContext *context,
        ProtoString *base,
		unsigned long currentIndex
	) : Cell(context) {
    this->base = base;
	this->currentIndex = currentIndex;
};

ProtoStringIteratorImplementation::~ProtoStringIteratorImplementation() {};

int ProtoStringIteratorImplementation::hasNext(ProtoContext *context) {
    if (this->currentIndex >= this->base->getSize(context))
        return FALSE;
    else
        return TRUE;
};

ProtoObject *ProtoStringIteratorImplementation::next(ProtoContext *context) {
    return this->base->getAt(context, this->currentIndex);
};

ProtoStringIteratorImplementation *ProtoStringIteratorImplementation::advance(ProtoContext *context) {
    return new(context) ProtoStringIteratorImplementation(context, this->base, this->currentIndex);
};

ProtoObject	  *ProtoStringIteratorImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;

    return p.oid.oid;
};

void ProtoStringIteratorImplementation::finalize(ProtoContext *context) {};

void ProtoStringIteratorImplementation::processReferences(
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

ProtoStringImplementation:ProtoStringImplementation(
    ProtoContext *context,
    ProtoTuple *baseTuple
) : Cell(context) {
    this->baseTuple = baseTuple;
};

ProtoStringImplementation::~ProtoStringImplementation() {

};

ProtoObject *ProtoStringImplementation::getAt(ProtoContext *context, int index) {
    return this->baseTuple->getAt(context, index);
};

ProtoString *ProtoStringImplementation::getSlice(ProtoContext *context, int from, int to) {
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

unsigned long ProtoStringImplementation::getSize(ProtoContext *context) {
    return this->baseTuple->getSize(context);
};

ProtoString *ProtoStringImplementation::setAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!value) {
		return NULL;
    }

    int thisSize = this->baseTuple->getSize(context);

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


    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

}

ProtoString *ProtoStringImplementation::insertAt(ProtoContext *context, int index, ProtoObject* value) {
	if (!value) {
		return NULL;
    }

    int thisSize = this->baseTuple->getSize(context);

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


    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

}

ProtoString *ProtoStringImplementation::setAtString(ProtoContext *context, int index, ProtoString* string) {
	if (!string) {
		return this;
    }

    int thisSize = this->baseTuple->getSize(context);

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

    for (unsigned long i = 0; i < string->getSize(context); i++)
        sourceList = sourceList->appendLast(context, string->getAt(context, i));

    for (int i = index + string->getSize(context); i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

}

ProtoString *ProtoStringImplementation::insertAtString(ProtoContext *context, int index, ProtoString* string) {
	if (!string) {
		return this;
    }

    int thisSize = this->baseTuple->getSize(context);

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

    for (unsigned long i = 0; i < string->getSize(context); i++)
        sourceList = sourceList->appendLast(context, string->getAt(context, i));

    for (int i = index; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return new(context) ProtoString(context, context->tupleFromList(sourceList));    

}

ProtoString *ProtoStringImplementation::appendFirst(ProtoContext *context, ProtoString* otherString) {
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

ProtoString *ProtoStringImplementation::appendLast(ProtoContext *context, ProtoString* otherString) {
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

ProtoString *ProtoStringImplementation::splitFirst(ProtoContext *context, int count) {
    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoStringImplementation::splitLast(ProtoContext *context, int count) {
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

ProtoString *ProtoStringImplementation::removeFirst(ProtoContext *context, int count) {
    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = count; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoStringImplementation::removeLast(ProtoContext *context, int count) {
    int thisSize = this->baseTuple->getSize(context);

    ProtoList *sourceList = context->newList();
    for (int i = 0; i < thisSize - count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        else
            break;

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
};

ProtoString *ProtoStringImplementation::removeAt(ProtoContext *context, int index) {
    int thisSize = this->baseTuple->getSize(context);

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

    return new(context) ProtoString(context, context->tupleFromList(sourceList));    
}

ProtoString *ProtoStringImplementation::removeSlice(ProtoContext *context, int from, int to) {
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
};

ProtoList *ProtoStringImplementation::asList(ProtoContext *context) {
    ProtoList *result = context->newList();

    int thisSize = this->getSize(context);
    for (int i = 0; i < thisSize; i++)
        result = result->appendLast(context, this->getAt(context, i));

    return result;
};

void ProtoStringImplementation::finalize(ProtoContext *context) {};

void ProtoStringImplementation::processReferences(
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

ProtoObject *ProtoStringImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_STRING;

    return p.oid.oid;
};

unsigned long ProtoStringImplementation::getHash(ProtoContext *context) {
    return (unsigned long) this->baseTuple;
};

ProtoStringIteratorImplementation *ProtoStringImplementation::getIterator(ProtoContext *context) {
    return new(context) ProtoStringIteratorImplementation(context, this, 0);
}


};

