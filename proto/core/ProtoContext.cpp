/*
 * ProtoContext.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"

#include <malloc.h>
#include <thread>
#include <stdlib.h>


using namespace std;
namespace proto {

#ifndef max
#define max(a, b) (((a) > (b))? (a):(b))
#endif

std::atomic<BOOLEAN> literalMutex(FALSE);

BigCell *literalFreeCells = NULL;
unsigned   literalFreeCellsIndex = 0;
ProtoContext globalContext;

#define BLOCKS_PER_MALLOC_REQUEST 1024U

ProtoContext::ProtoContext(
		ProtoContext *previous,
		ProtoObject **localsBase,
		unsigned int localsCount, 
		ProtoThread *thread,
		ProtoSpace *space
) {
    this->previous = previous;
    if (previous) {
        this->space = this->previous->space;
        this->thread = this->previous->thread;
        if (this->thread)
            this->thread->currentContext = this;
    }
    else {
        this->space = space;
        this->thread = thread;
        if (thread)
            this->thread->currentContext = this;
    }

    this->returnValue = NULL;
    this->lastAllocatedCell = NULL;
    this->localsBase = (ProtoObjectPointer *) localsBase;
    this->localsCount = localsCount;
    if (localsBase)
        for (int i = localsCount; i >= 0; i--)
            *localsBase++ = NULL;
    this->allocatedCellsCount = 0;
 
};

ProtoContext::~ProtoContext() {
    if (this->previous && this->space && this->lastAllocatedCell) {
        Cell *firstCell = this->lastAllocatedCell;

        // Send the list of allocated Cells to GC to analysis

        if (firstCell)
            this->space->analyzeUsedCells(firstCell);

    }
};

void ProtoContext::checkCellsCount() {
    if (this->allocatedCellsCount >= this->space->maxAllocatedCellsPerContext) {
        this->space->analyzeUsedCells(this->lastAllocatedCell);
        this->lastAllocatedCell = NULL;
        this->allocatedCellsCount = 0;
        this->space->triggerGC();
    }
}

Cell *ProtoContext::allocCell(){
    Cell *newCell;
    if (this->thread) {
        newCell = this->thread->allocCell();
        this->allocatedCellsCount += 1;
        this->checkCellsCount();
    }
    else {
        newCell = (Cell *) malloc(sizeof(BigCell));
    }

    newCell->nextCell = this->lastAllocatedCell;

    return newCell;
};

ProtoObject *ProtoContext::fromInteger(int value) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.si.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.si.embedded_type = EMBEDED_TYPE_SMALLINT;
    p.si.smallInteger = value;

    return p.oid.oid; 
};

ProtoObject *ProtoContext::fromDouble(double value) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.si.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.si.embedded_type = EMBEDED_TYPE_SMALLDOUBLE;

    union {
        unsigned long lv;
        double dv;
    } u;
    u.dv = value;
    p.sd.smallDouble = u.lv >> TYPE_SHIFT;

    return p.oid.oid; 
};

ProtoObject *ProtoContext::fromUTF8Char(const char *utf8OneCharString) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.unicodeChar.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.unicodeChar.embedded_type = EMBEDED_TYPE_UNICODECHAR;

    p.op.value = 0U;
    unsigned unicodeValue = 0U;
    if (( utf8OneCharString[0] & 0x80 ) == 0 )
        // 0000 0000-0000 007F | 0xxxxxxx
        unicodeValue = utf8OneCharString[0] & 0x7F;
    else if (( utf8OneCharString[0] & 0xE0 ) == 0xC0 )
        // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
        unicodeValue = 0x80 + 
                       ((utf8OneCharString[0] & 0x1F) << 6) + 
                       (utf8OneCharString[1] & 0x3F);
    else if (( utf8OneCharString[0] & 0xF0 ) == 0xE0 ) 
        // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
        unicodeValue = 0x800 + 
                       ((utf8OneCharString[0] & 0x0F) << 12) + 
                       ((utf8OneCharString[1] & 0x3F) << 6) + 
                       (utf8OneCharString[2] & 0x3F);
    else if (( utf8OneCharString[0] & 0xF8 ) == 0xF0 )
        // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        unicodeValue = 0x10000 + 
                       ((utf8OneCharString[0] & 0x07) << 18) + 
                       ((utf8OneCharString[1] & 0x3F) << 12) + 
                       ((utf8OneCharString[1] & 0x3F) << 6) + 
                       (utf8OneCharString[2] & 0x3F);

    p.unicodeChar.unicodeValue = unicodeValue;

    return p.oid.oid; 
};

TupleDictionary::TupleDictionary(
        ProtoContext *context,
        ProtoTuple *key,
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

int TupleDictionary::compareList(ProtoContext *context, ProtoList *list) {
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

ProtoTuple *TupleDictionary::hasList(ProtoContext *context, ProtoList *list) {
    TupleDictionary *node = this;
    int cmp;

    // Empty tree case
    if (!this->key)
        return FALSE;

    while (node) {
        cmp = node->compareList(context, list);
        if (cmp == 0)
            return node->key;
        if (cmp > 0)
            node = node->next;
        else
            node = node->previous;
    }
    return NULL;
};

int TupleDictionary::has(ProtoContext *context, ProtoTuple *tuple) {
    TupleDictionary *node = this;
    int cmp;

    // Empty tree case
    if (!this->key)
        return FALSE;

    while (node) {
        cmp = node->compareTuple(context, tuple);
        if (cmp == 0)
            return TRUE;
        if (cmp > 0)
            node = node->next;
        else
            node = node->previous;
    }
    return FALSE;
};

ProtoTuple *TupleDictionary::getAt(ProtoContext *context, ProtoTuple *tuple) {
    TupleDictionary *node = this;
    int cmp;

    // Empty tree case
    if (!this->key)
        return FALSE;

    while (node) {
        cmp = node->compareTuple(context, tuple);
        if (cmp == 0)
            return node->key;
        if (cmp > 0)
            node = node->next;
        else
            node = node->previous;
    }
    return FALSE;
};

TupleDictionary *TupleDictionary::set(ProtoContext *context, ProtoTuple *tuple) {
    TupleDictionary *newNode;
    int cmp;

    // Empty tree case
    if (!this->key)
        return new(context) TupleDictionary(
            context,
            key = tuple
        );

    cmp = this->compareTuple(context, tuple);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(context) TupleDictionary(
                context,
                key = this->key,
                previous = this->previous,
                next = this->next->set(context, tuple)
            );
        }
        else {
            newNode = new(context) TupleDictionary(
                context,
                key = this->key,
                previous = this->previous,
                next = new(context) TupleDictionary(
                    context,
                    key = this->key
                )
            );
        }
    }
    else if (cmp < 0) {
        if (this->previous) {
            newNode = new(context) TupleDictionary(
                context,
                key = this->key,
                previous = this->previous->set(context, tuple),
                next = this->next
            );
        }
        else {
            newNode = new(context) TupleDictionary(
                context,
                key = this->key,
                previous = new(context) TupleDictionary(
                    context,
                    key = this->key
                ),
                next = this->next
            );
        }
    }
    else 
        return this;

    return newNode->rebalance(context);
};

TupleDictionary *TupleDictionary::removeFirst(ProtoContext *context) {
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

ProtoTuple *TupleDictionary::getFirst(ProtoContext *context) {
    TupleDictionary *node = this;

    while (node) {
        if (node->previous)
            node = node->previous;
        return node->key;
    }
    return NULL;
};

TupleDictionary *TupleDictionary::removeAt(ProtoContext *context, ProtoTuple *key) {
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
                ProtoTuple *first = this->next->getFirst(context);
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
    TupleDictionary *newRight = new(context) TupleDictionary(
        context,
        this->key,
        this->previous? this->previous->next : NULL,
        this->next
    );
    return new(context) TupleDictionary(
        context,
        this->previous? this->previous->key : NULL,
        this->previous? this->previous->previous : NULL,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
TupleDictionary *TupleDictionary::leftRotate(ProtoContext *context) {
    TupleDictionary *newLeft = new(context) TupleDictionary(
        context,
        this->key,
        this->previous,
        (this->next? this->next->previous : NULL)
    );
    return new(context) TupleDictionary(
        context,
        this->next? this->next->key : NULL,
        newLeft,
        this->next? this->next->next : NULL
    );
}

TupleDictionary *TupleDictionary::rebalance(ProtoContext *context) {
    TupleDictionary *newNode = this;
    while (TRUE) {
        // If this node becomes unbalanced, then
        // there are 4 cases

        if (!newNode->previous || !newNode->next)
            return newNode;

        int balance = newNode->next->height - newNode->previous->height;

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
                if (balance < 0) {
                    newNode = new(context) TupleDictionary(
                        context,
                        newNode->key,
                        newNode->previous->leftRotate(context),
                        newNode->next
                    );
                    newNode = newNode->rightRotate(context);
                }
                else {
                    // Right Left Case
                    if (balance > 0) {
                        newNode = new(context) TupleDictionary(
                            context,
                            newNode->key,
                            newNode->previous,
                            newNode->next->rightRotate(context)
                        );
                        newNode = newNode->leftRotate(context);
                    }
                    else
                        return newNode;
                }
            }
        }
    }
}

ProtoList *ProtoContext::newList() {
    ProtoList *list = new(this) ProtoList(this);
    return list;
}

ProtoTuple *ProtoContext::newTuple() {
    ProtoTuple *tuple = new(this) ProtoTuple(this);
    return tuple;
}

ProtoSparseList *ProtoContext::newSparseList() {
    ProtoSparseList *sparseList = new(this) ProtoSparseList(this);
    return sparseList;
}

ProtoTuple *ProtoContext::tupleFromList(ProtoList *list) {
    unsigned long size = list->getSize(this);
    ProtoTuple *newTuple = NULL;
    ProtoList *nextLevel, *lastLevel = NULL;
    ProtoTuple *indirectData[TUPLE_SIZE];
    ProtoObject *data[TUPLE_SIZE];

    ProtoList *indirectPointers = new(this) ProtoList(this);
    unsigned long i, j;
    for (i = 0, j = 0; i < size; i++) {
        data[j++] = list->getAt(this, i);

        if (j == TUPLE_SIZE) {
            newTuple = new(this) ProtoTuple(
                this,
                TUPLE_SIZE,
                0,
                data
            );
            indirectPointers = indirectPointers->appendLast(this, (ProtoObject *) newTuple);
            j = 0;
        }
    }

    if (j != 0) {
        unsigned long lastSize = j;
        for (; j < TUPLE_SIZE; j++)
            data[j] = NULL;
        newTuple = new(this) ProtoTuple(
            this,
            lastSize,
            0,
            data
        );
        indirectPointers = indirectPointers->appendLast(this, (ProtoObject *) newTuple);
    }

    if (size >= TUPLE_SIZE) {
        int indirectSize = 0;
        int levelCount = 0;
        while (indirectPointers->getSize(this) > 0) {
            nextLevel = new(this) ProtoList(this);
            levelCount++;
            indirectSize = 0;
            for (i = 0, j = 0; i < indirectPointers->getSize(this); i++) {
                indirectData[j] = (ProtoTuple *) indirectPointers->getAt(this, i);
                indirectSize += indirectData[j]->elementCount;
                j++;
                if (j == TUPLE_SIZE) {
                    newTuple = new(this) ProtoTuple(
                        this,
                        indirectSize,
                        levelCount,
                        NULL,
                        indirectData
                    );
                    indirectSize = 0;
                    j = 0;
                    nextLevel = nextLevel->appendLast(this, (ProtoObject *) newTuple);
                }
            }
            if (j != 0) {
                unsigned long lastCount = indirectSize;
                for (; j < TUPLE_SIZE; j++)
                    indirectData[j] = NULL;
                newTuple = new(this) ProtoTuple(
                    this,
                    indirectSize,
                    levelCount,
                    NULL,
                    indirectData
                );
                nextLevel = nextLevel->appendLast(this, (ProtoObject *) newTuple);
            }

            lastLevel = indirectPointers;
            indirectPointers = nextLevel;

            if (nextLevel->getSize(this) <= TUPLE_SIZE)
                break;
        };

        if (indirectPointers->getSize(this) > 1) {
            for (j = 0; j < TUPLE_SIZE; j++)
                if (j < indirectPointers->getSize(this))
                    indirectData[j] = (ProtoTuple *) lastLevel->getAt(this, j);
                else
                    indirectData[j] = NULL;
            newTuple = new(this) ProtoTuple(
                this,
                list->getSize(this),
                levelCount + 1,
                NULL,
                indirectData
            );
        }
    }

    TupleDictionary *currentRoot, *newRoot;
    currentRoot = this->space->tupleRoot.load();
    do {
        if (currentRoot->has(this, newTuple)) {
            newTuple = currentRoot->getAt(this, newTuple);
            break;
        }
        else
            newRoot = currentRoot->set(this, newTuple);

    } while (this->space->tupleRoot.compare_exchange_strong(
        currentRoot,
        newRoot
    ));

    return newTuple;
}

ProtoString *ProtoContext::fromUTF8String(const char *zeroTerminatedUtf8String) {
    const char *currentChar = zeroTerminatedUtf8String;
    ProtoList *string = new(this) ProtoList(this);

    while (*currentChar) {
        ProtoObject *oneChar = this->fromUTF8Char(currentChar);
        if (( currentChar[0] & 0x80 ) == 0 )
            // 0000 0000-0000 007F | 0xxxxxxx
            currentChar += 1;
        else if (( currentChar[0] & 0xE0 ) == 0xC0 )
            // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
            currentChar += 2;
        else if (( currentChar[0] & 0xF0 ) == 0xE0 ) 
            // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
            currentChar += 3;
        else if (( currentChar[0] & 0xF8 ) == 0xF0 )
            currentChar += 4;

        string = string->appendLast(this, oneChar);    
    }

    return new(this) ProtoString(
        this,
        this->tupleFromList(string)
    );
};

ProtoObject *ProtoContext::fromDate(unsigned year, unsigned month, unsigned day) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.op.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.op.embedded_type = EMBEDED_TYPE_DATE;
    p.date.year = year;
    p.date.month = month;
    p.date.day = day;

    return p.oid.oid; 
};

ProtoObject *ProtoContext::fromTimestamp(unsigned long timestamp) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.op.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.op.embedded_type = EMBEDED_TYPE_TIMESTAMP;
    p.timestampValue.timestamp = timestamp;

    return p.oid.oid; 
};

ProtoObject *ProtoContext::fromTimeDelta(long timedelta) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.sd.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.op.embedded_type = EMBEDED_TYPE_TIMEDELTA;
    p.timedeltaValue.timedelta = timedelta;

    return p.oid.oid; 
};

ProtoMethodCell *ProtoContext::fromMethod(ProtoObject *self, ProtoMethod method) {
    return new(this) ProtoMethodCell(this, self, method);
};

ProtoExternalPointer *ProtoContext::fromExternalPointer(void *pointer) {
    return new(this) ProtoExternalPointer(this, pointer);
};

ProtoByteBuffer *ProtoContext::fromBuffer(unsigned long length, char* buffer) {
    return new(this) ProtoByteBuffer(this, length, buffer);
};

ProtoByteBuffer *ProtoContext::newBuffer(unsigned long length) {
    return new(this) ProtoByteBuffer(this, length);
};

ProtoObject *ProtoContext::fromBoolean(BOOLEAN value) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.booleanValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.booleanValue.embedded_type = EMBEDED_TYPE_BOOLEAN;
    p.booleanValue.booleanValue = value;

    return p.oid.oid;
};

ProtoObject *ProtoContext::fromByte(char c) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.byteValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.byteValue.embedded_type = EMBEDED_TYPE_BYTE;
    p.byteValue.byteData = c;

    return p.oid.oid;
};

};
