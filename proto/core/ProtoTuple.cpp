/*
 * tuple.cpp
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

class TupleDictionary: public Cell {
private:
    TupleDictionary *next;
    TupleDictionary *previous;
    ProtoTupleImplementation* key;
    int count;
    int height;

    int compareTuple(ProtoContext *context, ProtoTuple *tuple);
    TupleDictionary *rightRotate(ProtoContext *context);
    TupleDictionary *leftRotate(ProtoContext *context);
    TupleDictionary *rebalance(ProtoContext *context);
	TupleDictionary *removeFirst(ProtoContext *context);
	ProtoTupleImplementation* getFirst(ProtoContext *context);

public:
    TupleDictionary(
        ProtoContext *context,
        ProtoTupleImplementation* key = NULL,
        TupleDictionary *next = NULL,
        TupleDictionary *previous = NULL
    );

	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

    int compareList(ProtoContext *context, ProtoList<ProtoObject> *list);
    int hasList(ProtoContext *context, ProtoList<ProtoObject> *list);
    int has(ProtoContext *context, ProtoTupleImplementation *tuple);
    ProtoTupleImplementation* getAt(ProtoContext *context, ProtoTupleImplementation* tuple);
    TupleDictionary *set(ProtoContext *context, ProtoTupleImplementation* tuple);
    TupleDictionary *removeAt(ProtoContext *context, ProtoTupleImplementation* tuple);
};

TupleDictionary::TupleDictionary(
        ProtoContext *context,
        ProtoTupleImplementation* key,
        TupleDictionary *next,
        TupleDictionary *previous
    ): Cell(context) {
    this->key = key;
    this->next = next;
    this->previous = previous;
    this->height = (key? 1 : 0) + max((previous? previous->height : 0), (next? next->height : 0)),
    this->count = (previous? previous->count : 0) + (key? 1 : 0) + (next? next->count : 0);
};

void TupleDictionary::finalize(ProtoContext *context) {};

void TupleDictionary::processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	) {
    if (this->next)
        this->next->processReferences(context, self, method);
    if (this->previous)
        this->previous->processReferences(context, self, method);
    (*method)(context, this, this);
};

int TupleDictionary::compareList(ProtoContext *context, ProtoList<ProtoObject> *list) {
    int thisSize = this->key->getSize(context);
    int listSize = list->getSize(context);

    int cmpSize = (thisSize < listSize)? thisSize : listSize;
    int i;
    for (i = 0; i <= cmpSize; i++) {
        int thisElementHash = this->key->getAt(context, i)->getHash(context);
        int tupleElementHash = list->getAt(context, i)->getHash(context);
        if (thisElementHash > tupleElementHash)
            return 1;
        else if (thisElementHash < tupleElementHash)
            return 1;
    }
    if (i > thisSize)
        return -1;
    else if (i > listSize)
        return 1;
    return 0;
};

int TupleDictionary::hasList(ProtoContext *context, ProtoList<ProtoObject> *list) {
    TupleDictionary *node = this;
    int cmp;

    // Empty tree case
    if (!this->key)
        return false;

    while (node) {
        cmp = node->compareList(context, list);
        if (cmp == 0)
            return true;
        if (cmp > 0)
            node = node->next;
        else
            node = node->previous;
    }
    return false;
};

int TupleDictionary::has(ProtoContext *context, ProtoTupleImplementation *tuple) {
    TupleDictionary *node = this;
    int cmp;

    // Empty tree case
    if (!this->key)
        return false;

    while (node) {
        cmp = node->compareTuple(context, tuple);
        if (cmp == 0)
            return true;
        if (cmp > 0)
            node = node->next;
        else
            node = node->previous;
    }
    return false;
};

ProtoTupleImplementation* TupleDictionary::getAt(ProtoContext *context, ProtoTupleImplementation* tuple) {
    TupleDictionary *node = this;
    int cmp;

    // Empty tree case
    if (!this->key)
        return NULL;

    while (node) {
        cmp = node->compareTuple(context, tuple);
        if (cmp == 0)
            return node->key;
        if (cmp > 0)
            node = node->next;
        else
            node = node->previous;
    }
    return NULL;
};

TupleDictionary* TupleDictionary::set(ProtoContext *context, ProtoTupleImplementation* tuple) {
    TupleDictionary *newNode;
    int cmp;

    // Empty tree case
    if (!this->key)
        return new(context) TupleDictionary(
            context,
            tuple
        );

    cmp = this->compareTuple(context, tuple);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->previous,
                this->next->set(context, tuple)
            );
        }
        else {
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->previous,
                new(context) TupleDictionary(
                    context,
                    tuple
                )
            );
        }
    }
    else if (cmp < 0) {
        if (this->previous) {
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->previous->set(context, tuple),
                this->next
            );
        }
        else {
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                new(context) TupleDictionary(
                    context,
                    tuple
                ),
                this->next
            );
        }
    }
    else 
        return this;

    return newNode->rebalance(context);
};

TupleDictionary* TupleDictionary::removeFirst(ProtoContext *context) {
    TupleDictionary *newNode;

    if (this->previous) {
        if (this->previous->previous)
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->next,
                this->previous->removeFirst(context)
            );
        else if (this->previous->next)
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->next,
                this->previous->next
            );
        else
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->next,
                NULL
            );
    }
    else {
        return this->next;
    }

    return newNode->rebalance(context);
};

ProtoTupleImplementation *TupleDictionary::getFirst(ProtoContext *context) {
    TupleDictionary *node = this;

    while (node) {
        if (node->previous)
            node = node->previous;
        return node->key;
    }
    return NULL;
};

TupleDictionary *TupleDictionary::removeAt(ProtoContext *context, ProtoTupleImplementation *key) {
    TupleDictionary *newNode;

    if (this->key == key) {
        if (this->previous && this->next) {
            if (!this->previous->previous && !this->previous->next)
                newNode = new(context) TupleDictionary(
                    context,
                    this->previous->key,
                    this->next
                );
            else {
                ProtoTupleImplementation *first = this->next->getFirst(context);
                newNode = new(context) TupleDictionary(
                    context,
                    first,
                    this->next->removeFirst(context),
                    this->previous
                );
            }
        }
        else if (this->previous)
            newNode = this->previous;
        else
            newNode = this->next;
    }
    else {
        if (key->getHash(context) < this->key->getHash(context)) {
            newNode = this->previous->removeAt(context, key);
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                newNode,
                this->next
            );
        }
        else {
            newNode = this->next->removeAt(context, key);
            newNode = new(context) TupleDictionary(
                context,
                this->key,
                this->previous,
                newNode
            );
        }
    }

    return newNode->rebalance(context);
};

int TupleDictionary::compareTuple(ProtoContext *context, ProtoTuple *tuple) {
    int thisSize = this->key->getSize(context);
    int tupleSize = tuple->getSize(context);

    int cmpSize = (thisSize < tupleSize)? thisSize : tupleSize;
    int i;
    for (i = 0; i < cmpSize; i++) {
        int thisElementHash = this->key->getAt(context, i)->getHash(context);
        int tupleElementHash = tuple->getAt(context, i)->getHash(context);
        if (thisElementHash > tupleElementHash)
            return 1;
        else if (thisElementHash < tupleElementHash)
            return 1;
    }
    if (i > thisSize)
        return -1;
    else if (i > tupleSize)
        return 1;
    return 0;
}

// A utility function to right rotate subtree rooted with y
// See the diagram given above.
TupleDictionary *TupleDictionary::rightRotate(ProtoContext *context)
{
    if(!this->previous)
        return this;

    TupleDictionary *newRight = new(context) TupleDictionary(
        context,
        this->key,
        this->previous->next,
        this->next
    );
    return new(context) TupleDictionary(
        context,
        this->previous->key,
        this->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
TupleDictionary *TupleDictionary::leftRotate(ProtoContext *context) {
    if(!this->next)
        return this;

    TupleDictionary *newLeft = new(context) TupleDictionary(
        context,
        this->key,
        this->previous,
        this->next->previous
    );
    return new(context) TupleDictionary(
        context,
        this->next->key,
        newLeft,
        this->next->next
    );
}

TupleDictionary *TupleDictionary::rebalance(ProtoContext *context) {
    TupleDictionary *newNode = this;
    while (true) {
        // If this node becomes unbalanced, then
        // there are 4 cases

        int balance = (newNode->next? newNode->next->height : 0) - (newNode->previous? newNode->previous->height : 0);

        if (balance >= -1 && balance <= 1)
            return newNode;

        // Left Left Case
        if (balance < -1) {
            newNode = newNode->rightRotate(context);
        }
        else {
            // Right Right Case
            if (balance > 1) {
                newNode = newNode->leftRotate(context);
            }
            // Left Right Case
            else {
                if (balance < 0 && newNode->previous) {
                    newNode = new(context) TupleDictionary(
                        context,
                        newNode->key,
                        newNode->previous->leftRotate(context),
                        newNode->next
                    );
                    if (!newNode->previous)
                        return newNode;
                    newNode = newNode->rightRotate(context);
                }
                else {
                    // Right Left Case
                    if (balance > 0 && newNode->next) {
                        newNode = new(context) TupleDictionary(
                            context,
                            newNode->key,
                            newNode->previous,
                            newNode->next->rightRotate(context)
                        );
                        if (!newNode->next)
                            return newNode;
                        newNode = newNode->leftRotate(context);
                    }
                    else
                        return newNode;
                }
            }
        }
    }
}


ProtoTupleIteratorImplementation::ProtoTupleIteratorImplementation(
		ProtoContext *context,
        ProtoTupleImplementation *base,
		unsigned long currentIndex
	) : Cell(context) {
    this->base = base;
	this->currentIndex = currentIndex;
};

ProtoTupleIteratorImplementation::~ProtoTupleIteratorImplementation() {};

int ProtoTupleIteratorImplementation::hasNext(ProtoContext *context) {
    if (this->currentIndex >= this->base->getSize(context))
        return false;
    else
        return true;
};

ProtoObject *ProtoTupleIteratorImplementation::next(ProtoContext *context) {
    return this->base->getAt(context, this->currentIndex);
};

ProtoTupleIteratorImplementation *ProtoTupleIteratorImplementation::advance(ProtoContext *context) {
    return new(context) ProtoTupleIteratorImplementation(context, this->base, this->currentIndex);
};

ProtoObject	  *ProtoTupleIteratorImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;

    return p.oid.oid;
};

void ProtoTupleIteratorImplementation::finalize(ProtoContext *context) {};

void ProtoTupleIteratorImplementation::processReferences(
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

ProtoTupleImplementation::ProtoTupleImplementation(
    ProtoContext *context,
    unsigned long elementCount,
    unsigned long height,
    ProtoObject **data,
    ProtoTupleImplementation **indirect
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

ProtoTupleImplementation::~ProtoTupleImplementation() {};

ProtoTupleImplementation *ProtoTupleImplementation::tupleFromList(ProtoContext *context, ProtoList<ProtoObject> *list) {
    unsigned long size = list->getSize(context);
    ProtoTupleImplementation *newTuple = NULL;
    ProtoList<ProtoObject> *nextLevel, *lastLevel = NULL;
    ProtoTupleImplementation *indirectData[TUPLE_SIZE];
    ProtoObject *data[TUPLE_SIZE];

    ProtoList<ProtoObject> *indirectPointers = 
        (ProtoList<ProtoObject> *) context->newList();
    unsigned long i, j;
    for (i = 0, j = 0; i < size; i++) {
        data[j++] = list->getAt(context, i);

        if (j == TUPLE_SIZE) {
            newTuple = new(context) ProtoTupleImplementation(
                context,
                TUPLE_SIZE,
                0,
                data
            );
            indirectPointers = indirectPointers->appendLast(context, (ProtoObject *) newTuple);
            j = 0;
        }
    }

    if (j != 0) {
        unsigned long lastSize = j;
        for (; j < TUPLE_SIZE; j++)
            data[j] = NULL;
        newTuple = new(context) ProtoTupleImplementation(
            context,
            lastSize,
            0,
            data
        );
        indirectPointers = indirectPointers->appendLast(context, (ProtoObject *) newTuple);
    }

    if (size >= TUPLE_SIZE) {
        int indirectSize = 0;
        int levelCount = 0;
        while (indirectPointers->getSize(context) > 0) {
            nextLevel = (ProtoList<ProtoObject> *) context->newList();
            levelCount++;
            indirectSize = 0;
            for (i = 0, j = 0; i < indirectPointers->getSize(context); i++) {
                indirectData[j] = (ProtoTupleImplementation *) indirectPointers->getAt(context, i);
                indirectSize += indirectData[j]->elementCount;
                j++;
                if (j == TUPLE_SIZE) {
                    newTuple = new(context) ProtoTupleImplementation(
                        context,
                        indirectSize,
                        levelCount,
                        NULL,
                        indirectData
                    );
                    indirectSize = 0;
                    j = 0;
                    nextLevel = nextLevel->appendLast(context, (ProtoObject *) newTuple);
                }
            }
            if (j != 0) {
                for (; j < TUPLE_SIZE; j++)
                    indirectData[j] = NULL;
                newTuple = new(context) ProtoTupleImplementation(
                    context,
                    indirectSize,
                    levelCount,
                    NULL,
                    indirectData
                );
                nextLevel = nextLevel->appendLast(context, (ProtoObject *) newTuple);
            }

            lastLevel = (ProtoList<ProtoObject> *) indirectPointers;
            indirectPointers = nextLevel;

            if (nextLevel->getSize(context) <= TUPLE_SIZE)
                break;
        };

        if (indirectPointers->getSize(context) > 1) {
            for (j = 0; j < TUPLE_SIZE; j++)
                if (j < indirectPointers->getSize(context))
                    indirectData[j] = (ProtoTupleImplementation *) lastLevel->getAt(context, j);
                else
                    indirectData[j] = NULL;
            newTuple = new(context) ProtoTupleImplementation(
                context,
                list->getSize(context),
                levelCount + 1,
                NULL,
                indirectData
            );
        }
    }

    TupleDictionary *currentRoot, *newRoot;
    currentRoot = context->space->tupleRoot.load();
    do {
        if (currentRoot->has(context, newTuple)) {
            newTuple = currentRoot->getAt(context, newTuple);
            break;
        }
        else
            newRoot = currentRoot->set(context, newTuple);

    } while (context->space->tupleRoot.compare_exchange_strong(
        currentRoot,
        newRoot
    ));

    return newTuple;
}

TupleDictionary *ProtoTupleImplementation::createTupleRoot(ProtoContext *context) {
    return new(context) TupleDictionary(context);
}


ProtoObject   *ProtoTupleImplementation::getAt(ProtoContext *context, int index) {
    if (index < 0)
        index = ((int) this->elementCount) + index;

    if (index < 0)
        index = 0;

    int rest = index % TUPLE_SIZE;
    ProtoTupleImplementation *node = this;
    for (unsigned int i = this->height; i > 0; i--) {
        index = index / TUPLE_SIZE;
        node = node->pointers.indirect[index];
    }

    return node->pointers.data[rest];
};

ProtoObject   *ProtoTupleImplementation::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
};

ProtoObject   *ProtoTupleImplementation::getLast(ProtoContext *context) {
    if (this->elementCount > 0)
        return this->getAt(context, this->elementCount - 1);
    
    return PROTO_NONE;
};

ProtoTupleImplementation *ProtoTupleImplementation::getSlice(ProtoContext *context, int from, int to) {
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

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = from; i <= to; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return (ProtoTupleImplementation *) context->tupleFromList(sourceList);    
};

unsigned long  ProtoTupleImplementation::getSize(ProtoContext *context) {
    return this->elementCount;
};

BOOLEAN ProtoTupleImplementation::has(ProtoContext *context, ProtoObject* value) {
    for (unsigned long i = 0; i < this->elementCount; i++)
        if (value == this->getAt(context, i))
            return true;
    
    return false;
};

ProtoTupleImplementation *ProtoTupleImplementation::setAt(ProtoContext *context, int index, ProtoObject* value) {
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

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    sourceList = sourceList->appendLast(context, value);

    for (int i = index + 1; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::insertAt(ProtoContext *context, int index, ProtoObject* value) {
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

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    sourceList = sourceList->appendLast(context, value);

    for (int i = index; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));


    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::appendFirst(ProtoContext *context, ProtoTuple* otherTuple) {
	if (!otherTuple) {
		return NULL;
    }

    int thisSize = this->elementCount;

    ProtoList<ProtoObject> *sourceList = context->newList();
    int otherSize = otherTuple->getSize(context);
    for (int i = 0; i < otherSize; i++)
        sourceList = sourceList->appendLast(context, otherTuple->getAt(context, i));

    for (int i = 0; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::appendLast(ProtoContext *context, ProtoTuple *otherTuple) {
	if (!otherTuple) {
		return NULL;
    }

    int thisSize = this->elementCount;

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = 0; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    int otherSize = otherTuple->getSize(context);
    for (int i = 0; i < otherSize; i++)
        sourceList = sourceList->appendLast(context, otherTuple->getAt(context, i));

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::splitFirst(ProtoContext *context, int count) {
    int thisSize = this->elementCount;

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = 0; i < count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::splitLast(ProtoContext *context, int count) {
    int thisSize = this->elementCount;
    int first = thisSize - count;
    if (first < 0)
        first = 0;

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = first; i < thisSize; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::removeFirst(ProtoContext *context, int count) {
    int thisSize = this->elementCount;

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = count; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::removeLast(ProtoContext *context, int count) {
    int thisSize = this->elementCount;

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = 0; i < thisSize - count; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        else
            break;

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::removeAt(ProtoContext *context, int index) {
    int thisSize = this->elementCount;

    if (index < 0) {
        index = thisSize + index;
        if (index < 0)
            index = 0;
    }

    if (index >= thisSize) {
        return NULL;
    }

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = 0; i < index; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));

    for (int i = index + 1; i < thisSize; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoTupleImplementation *ProtoTupleImplementation::removeSlice(ProtoContext *context, int from, int to) {
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

    ProtoList<ProtoObject> *sourceList = context->newList();
    for (int i = from; i < to; i++)
        if (i < thisSize)
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        else
            break;

    return ProtoTupleImplementation::tupleFromList(context, sourceList);    

};

ProtoList<ProtoObject> *ProtoTupleImplementation::asList(ProtoContext *context) {
    ProtoList<ProtoObject> *sourceList = context->newList();
    for (unsigned long i = 0; i < this->elementCount; i++)
        sourceList = sourceList->appendLast(context, this->getAt(context, i));

    return sourceList;    

};

void ProtoTupleImplementation::finalize(ProtoContext *context) {
    TupleDictionary *newRoot, *currentRoot;
    do {
        currentRoot = context->space->tupleRoot.load();
        newRoot = currentRoot->removeAt(context, this);
    } while (context->space->tupleRoot.compare_exchange_strong(
        currentRoot,
        newRoot
    ));
};

void ProtoTupleImplementation::processReferences(
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

ProtoObject *ProtoTupleImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_TUPLE;

    return p.oid.oid;
};

unsigned long ProtoTupleImplementation::getHash(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
};

ProtoTupleIteratorImplementation *ProtoTupleImplementation::getIterator(ProtoContext *context) {
    return new(context) ProtoTupleIteratorImplementation(context, this, 0);
};

}