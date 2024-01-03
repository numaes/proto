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
ProtoTupleIterator::ProtoTupleIterator (ProtoContext *context) : Cell(context) {

};

ProtoTupleIterator::~ProtoTupleIterator() {};

ProtoTuple::ProtoTuple(
		ProtoContext *context,
		unsigned long element_count = 0,
		ProtoObject **data = NULL,
		ProtoTuple **indirect = NULL
) : Cell(context) {
    this->element_count = element_count;
    if (element_count <= TUPLE_SIZE) {
        for (int i = 0; i < TUPLE_SIZE; i++) {
            if (i < element_count)
                this->pointers.data[i] = data[i];
            else 
                this->pointers.data[i] = NULL;
        }
    }
    else {
        for (int i = 0; i < TUPLE_SIZE; i++) {
            if (i < element_count)
                this->pointers.indirect[i] = indirect[i];
            else 
                this->pointers.indirect[i] = NULL;
        }

    }
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
