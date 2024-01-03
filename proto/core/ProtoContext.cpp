/*
 * ProtoContext.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"

#include <malloc.h>
#include <random>
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

#define BLOCKS_PER_MALLOC_REQUEST_FOR_LITERAL 1024U

ProtoContext::ProtoContext(
		ProtoContext *previous = NULL,
		void *localsBase = NULL,
		unsigned int localsCount = 0, 
		ProtoThread *thread = NULL,
		ProtoSpace *space = NULL
) {
    this->previous = previous;
    if (previous) {
        this->space = this->previous->space;
        this->thread = this->previous->thread;
    }
    else {
        this->space = space;
        this->thread = thread;
    }

    this->returnValue = NULL;
    this->lastAllocatedCell = NULL;
    this->localsBase = (ProtoObjectPointer *) localsBase;
    this->localsCount = localsCount;
    this->allocatedCellsCount = 0;
 
    this->thread->currentContext = this;
 };

ProtoContext::~ProtoContext() {
    if (this->previous && this->space && this->lastAllocatedCell) {
        Cell *firstCell = this->lastAllocatedCell;
        Cell *c = firstCell;
        ProtoObjectPointer p;
        p.oid.oid = this->returnValue;

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
    if (this->thread) {
        Cell *newCell = this->thread->allocCell();
        this->allocatedCellsCount += 1;
        return newCell;
    }
    else {
        // Assume the only context without thread is the Literal creation,
        // so use the literal global pool for cells

        Cell *newCell;
        BOOLEAN currentLock = FALSE;

        while (literalMutex.compare_exchange_strong(
            currentLock,
            TRUE
        )) {
            currentLock = FALSE;
            std::this_thread::yield();
        };

        if (!literalFreeCells) {
            literalFreeCells = (BigCell *) malloc(sizeof(BigCell) * BLOCKS_PER_MALLOC_REQUEST_FOR_LITERAL);
            if (!literalFreeCells) {
                printf("\nPANIC ERROR: Not enough MEMORY! Exiting ...\n");
                exit(1);
            }
            literalFreeCellsIndex = 0;

            // Clear new allocated blocks
            void **p =(void **) literalFreeCells;
            unsigned n = 0;
            while (n++ < (BLOCKS_PER_MALLOC_REQUEST_FOR_LITERAL * sizeof(BigCell) / sizeof(void *)))
                *p++ = NULL;
        }

        // Dealloc first free cell
        newCell = (Cell *) &literalFreeCells[literalFreeCellsIndex];

        literalFreeCellsIndex++;
        if (literalFreeCellsIndex >= BLOCKS_PER_MALLOC_REQUEST_FOR_LITERAL)
            literalFreeCells = NULL;

        literalMutex.store(FALSE);

        return newCell;
    }
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

ProtoObject *ProtoContext::fromUTF8Char(char *utf8OneCharString) {
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

class TupleDictionary: Cell {
private:
    TupleDictionary *next;
    TupleDictionary *previous;
    ProtoTuple *key;
    int count;
    int height;

    int compareTuple(ProtoContext *context, ProtoTuple *tuple) {
        int thisSize = this->key->getSize(context);
        int tupleSize = tuple->getSize(context);

        int cmpSize = (thisSize < tupleSize)? thisSize : tupleSize;
        int i;
        for (i = 0; i <= cmpSize; i++) {
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
    TupleDictionary *rightRotate(ProtoContext *context, TupleDictionary *n)
    {
        TupleDictionary *newRight = new(context) TupleDictionary(
            context,
            n->key,
            n->previous->next,
            n->next
        );
        return new(context) TupleDictionary(
            context,
            n->previous->key,
            n->previous->previous,
            newRight
        );
    }

    // A utility function to left rotate subtree rooted with x
    // See the diagram given above.
    TupleDictionary *leftRotate(ProtoContext *context, TupleDictionary *n) {
        TupleDictionary *newLeft = new(context) TupleDictionary(
            context,
            n->key,
            n->previous,
            n->next->previous
        );
        return new(context) TupleDictionary(
            context,
            n->next->key,
            newLeft,
            n->next->next
        );
    }

    TupleDictionary *rebalance(ProtoContext *context, TupleDictionary *newNode) {
        while (TRUE) {
            int balance = newNode->height;

            // If this node becomes unbalanced, then
            // there are 4 cases

            // Left Left Case
            if (balance < -1 && newNode->previous->height < 0) {
                newNode = rightRotate(context, newNode);
            }
            else {
                // Right Right Case
                if (balance > 1 && newNode->previous->height > 0) {
                    newNode = leftRotate(context, newNode);
                }
                // Left Right Case
                else {
                    if (balance < 0 && newNode->previous->height > 0) {
                        newNode = new(context) TupleDictionary(
                            context,
                            newNode->key,
                            leftRotate(context, newNode->previous),
                            newNode->next
                        );
                        newNode = rightRotate(context, newNode);
                    }
                    else {
                        // Right Left Case
                        if (balance > 0 && newNode->next->height < 0) {
                            newNode = new(context) TupleDictionary(
                                context,
                                newNode->key,
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

public:
    TupleDictionary(
        ProtoContext *context,
        ProtoTuple *key,
        TupleDictionary *next = NULL,
        TupleDictionary *previous = NULL
    ): Cell(context) {
        this->key = key;
        this->next = next;
        this->previous = previous;
        this->height = 1 + max(previous? previous->height : 0, next? next->height : 0),
        this->count = (previous? previous->count : 0) + (key? 1 : 0) + (next? next->count : 0);
    }

	virtual void finalize() {}

	virtual void processReferences(
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
    }

    int compareList(ProtoContext *context, ProtoList *list) {
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
    }

    ProtoTuple *hasList(ProtoContext *context, ProtoList *list) {
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
    }

    int has(ProtoContext *context, ProtoTuple *tuple) {
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
    }

    TupleDictionary *set(ProtoContext *context, ProtoTuple *tuple) {
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

        return rebalance(context, newNode);
    }
};

ProtoList *ProtoContext::newList() {
    ProtoList *list = new(this) ProtoList(this);
    return list;
}

ProtoTuple *ProtoContext::newTuple() {
    ProtoTuple *tuple = new(this) ProtoTuple(this);
    return tuple;
}

ProtoTuple *ProtoContext::tupleFromList(ProtoList *list) {
    unsigned long size = list->getSize(this);
    ProtoTuple *newTuple;
    ProtoList *nextLevel;
    ProtoTuple *indirectData[TUPLE_SIZE];
    ProtoObject *data[TUPLE_SIZE];

    ProtoList *indirectPointers = new(this) ProtoList(this);
    int i, j;
    for (i = 0, j = 0; i < size; i++, j++) {
        if (j == TUPLE_SIZE) {
            newTuple = new(this) ProtoTuple(
                this,
                TUPLE_SIZE,
                data
            );
            indirectPointers = indirectPointers->appendLast(this, (ProtoObject *) newTuple);
            j = 0;
        }
        data[j] = list->getAt(this, i);
    }

    newTuple = new(this) ProtoTuple(
        this,
        j,
        data
    );
    indirectPointers = indirectPointers->appendLast(this, (ProtoObject *) newTuple);
    
    int indirectSize = 0;
    while (indirectPointers->getSize(this) > 0) {
        nextLevel = new(this) ProtoList(this);
        indirectSize = 0;
        for (i = 0, j = 0; i < indirectPointers->getSize(this); i++, j++) {
            if (j == TUPLE_SIZE) {
                newTuple = new(this) ProtoTuple(
                    this,
                    indirectSize,
                    NULL,
                    indirectData
                );
                indirectSize = 0;
                j = 0;
                nextLevel = nextLevel->appendLast(this, (ProtoObject *) newTuple);
            }
            indirectData[j] = (ProtoTuple *) indirectPointers->getAt(this, i);
            indirectSize += indirectData[j]->elementCount;
        }
        newTuple = new(this) ProtoTuple(
            this,
            indirectSize,
            NULL,
            indirectData
        );
        nextLevel = nextLevel->appendLast(this, (ProtoObject *) newTuple);

        indirectPointers = nextLevel;
    }

    newTuple = new(this) ProtoTuple(
        this,
        indirectSize,
        NULL,
        indirectData
    );

    return newTuple;
}

ProtoString *ProtoContext::fromUTF8String(char *zeroTerminatedUtf8String) {
    char *currentChar = zeroTerminatedUtf8String;
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

ProtoByteBuffer *ProtoContext::newBuffer(unsigned long length) {
    return new(this) ProtoByteBuffer(this, length, (char *) malloc((size_t) length));
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

ProtoObject *ProtoContext::newInmutable() {
    ParentLink *newParent = new(this) ParentLink(
        this,
        NULL,
        (ProtoObjectCell *) this->space->objectPrototype
    );

    ProtoObjectCell *object = new(this) ProtoObjectCell(
        this,
        newParent
    );

    return object->asObject(this);
}

ProtoObject *ProtoContext::newMutable() {
    ProtoSparseList *currentRoot, *oldRoot;
    ProtoObject *randomIdObject;
    int randomId;

    do {
        currentRoot = this->space->mutableRoot.load();
        oldRoot = currentRoot;

        randomId = rand();

        BOOLEAN existingMutable = currentRoot->has(this, (unsigned long) randomIdObject);
        if (existingMutable)
            continue;

    } while (!this->space->mutableRoot.compare_exchange_strong(
        oldRoot,
        currentRoot->setAt(
            this,
            randomId,
            (new(this) ProtoSparseList(this))->asObject(this)
        )
    ));

    
};

ProtoThread *ProtoContext::getCurrentThread() {
    return this->thread;
}

};
