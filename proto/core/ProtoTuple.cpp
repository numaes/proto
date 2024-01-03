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

void ProtoTupleIterator::finalize() {};

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

ProtoTuple::~ProtoTuple() {};

ProtoObject   *getAt(ProtoContext *context, int index) {};
ProtoObject   *getFirst(ProtoContext *context) {};
ProtoObject   *getLast(ProtoContext *context) {};
ProtoList	  *getSlice(ProtoContext *context, int from, int to) {};
unsigned long  getSize(ProtoContext *context) {};
BOOLEAN		   has(ProtoContext *context, ProtoObject* value) {};
ProtoTuple    *setAt(ProtoContext *context, int index, ProtoObject* value) {};
ProtoTuple    *insertAt(ProtoContext *context, int index, ProtoObject* value) {};
ProtoTuple 	  *appendFirst(ProtoContext *context, ProtoObject* value) {};
ProtoTuple 	  *appendLast(ProtoContext *context, ProtoObject* value) {};
ProtoTuple 	  *extend(ProtoContext *context, ProtoList* other) {};
ProtoTuple	  *splitFirst(ProtoContext *context, int index) {};
ProtoTuple    *splitLast(ProtoContext *context, int index) {};
ProtoTuple	  *removeFirst(ProtoContext *context) {};
ProtoTuple	  *removeLast(ProtoContext *context) {};
ProtoTuple	  *removeAt(ProtoContext *context, int index) {};
ProtoTuple 	  *removeSlice(ProtoContext *context, int from, int to) {};
ProtoList	  *asList(ProtoContext *context) {};
ProtoObject	  *asObject(ProtoContext *context) {};

void finalize() {};

void ProtoList::processReferences(
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
