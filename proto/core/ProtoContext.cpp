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

// Global literal dictionary
class LiteralDictionary: public Cell {
public:
	LiteralDictionary(
		ProtoContext *context,

		ProtoList *key,
		LiteralDictionary *previous,
		LiteralDictionary *next
	);
	~LiteralDictionary();

    ProtoList           *key;
    ProtoObject         *hash;
    LiteralDictionary   *previous;
    LiteralDictionary   *next;
    unsigned long        count:52;
    unsigned long        balance:4;
    unsigned long        height:8;

    ProtoList           *getFromString(ProtoList *string);
	ProtoList			*get(char *zeroTerminatedUTF8CharString);
	LiteralDictionary	*set(ProtoList *string);
};

std::atomic<BOOLEAN> literalMutex(FALSE);

BigCell *literalFreeCells = NULL;
unsigned   literalFreeCellsIndex = 0;
ProtoContext literalContext;
std::atomic<LiteralDictionary *> literalRoot(
    new(&literalContext) LiteralDictionary(
            &literalContext, (ProtoList *) NULL, 
            (LiteralDictionary *) NULL, (LiteralDictionary *) NULL));

#define BLOCKS_PER_MALLOC_REQUEST_FOR_LITERAL 1024U

ProtoContext::ProtoContext(
		ProtoContext *previous,
		ProtoSpace *space,
		ProtoThread *thread
) {
    if (previous) {
        this->previous = previous;
        this->space = this->previous->space;
        this->thread = this->previous->thread;
    }
    else {
        this->previous = NULL;
        this->space = space;
        this->thread = thread;
    }

    this->returnValue = NULL;

    if (this->thread)
        this->lastCellPreviousContext = this->thread->currentWorkingSet;
};

ProtoContext::~ProtoContext() {
    Cell *freeCells = NULL;

    // Generate the chain of cells for the return value if needed

    if (this->thread) {
        freeCells = this->thread->currentWorkingSet;
        Cell *currentCell = this->thread->currentWorkingSet;

        Cell *previousCell = NULL;
        while (currentCell && currentCell != this->lastCellPreviousContext) {
            if (currentCell->context == this &&
                currentCell == (Cell *) this->returnValue) {
                if (previousCell)
                    previousCell->nextCell = currentCell->nextCell;
                else
                    freeCells = currentCell->nextCell;

                currentCell->nextCell = this->lastCellPreviousContext;
                this->lastCellPreviousContext = currentCell;

                currentCell->context = this->previous;
            }
            else
                previousCell = currentCell;

            currentCell = currentCell->nextCell;
        }

        if (previousCell)
            previousCell->nextCell = NULL;

        if (this->thread)
            this->thread->currentWorkingSet = this->lastCellPreviousContext;
        
        // Return all dirty blocks to space

        if (freeCells && this->space)
            this->space->analyzeUsedCells(freeCells);
    }
};

Cell *ProtoContext::allocCell(){
    if (this->thread)
        return this->thread->allocCell();
    else {
        // Assume the only context without thread is the Literal creation,
        // so use global pool for cells

        // Get literalLock
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

void ProtoContext::setReturnValue(ProtoObject *value){
    this->returnValue = value;
};

ProtoObject *ProtoContext::fromInteger(int value) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.si.pointer_tag = POINTER_TAG_SMALLINT;
    p.si.smallInteger = value;

    return p.oid.oid; 
};

ProtoObject *ProtoContext::fromDouble(double value) {
    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.op.pointer_tag = POINTER_TAG_SMALLDOUBLE;

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

ProtoObject *ProtoContext::fromUTF8String(char *zeroTerminatedUtf8String) {
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

    return string;
};

ProtoObject *ProtoContext::fromMethod(ProtoObject *self, ProtoMethod method) {
    return new(this) ProtoMethodCell(this, self, method);
};

ProtoObject *ProtoContext::fromExternalPointer(void *pointer) {
    return new(this) ProtoExternalPointer(this, pointer);
};

ProtoObject *ProtoContext::newBuffer(unsigned long length) {
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

ProtoObject *ProtoContext::literalFromUTF8String(char *zeroTerminatedUtf8String) {
    LiteralDictionary *newRoot, *currentRoot;
    ProtoList *literalString;

    do {
        currentRoot = literalRoot.load();

        ProtoObject *currentLiteral = currentRoot->get(zeroTerminatedUtf8String);
        if (currentLiteral != PROTO_NONE)
            return currentLiteral;

        literalString = new(&literalContext) ProtoList(&literalContext);
        char *currentChar = zeroTerminatedUtf8String;
        while (*currentChar) {
            ProtoObject *protoChar = this->fromUTF8Char(currentChar);
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
            literalString = literalString->appendLast(&literalContext, protoChar);
        }

        newRoot = currentRoot->set(literalString);
    } while (!literalRoot.compare_exchange_strong(
        currentRoot, 
        newRoot
    ));

    return literalString;
};

ProtoObject *ProtoContext::literalFromString(ProtoList *string) {
    LiteralDictionary *newRoot, *currentRoot;

    do {
        currentRoot = literalRoot.load();

        ProtoList *currentLiteral = currentRoot->getFromString(string);
        if (currentLiteral)
            return currentLiteral;

        newRoot = currentRoot->set(string);
    } while (!literalRoot.compare_exchange_strong(
        currentRoot, 
        newRoot
    ));

    return PROTO_NULL;
};

ProtoObject *ProtoContext::newMutable(ProtoObject *value) {
    IdentityDict *currentRoot;
    Cell *oldRoot;
    ProtoObject *randomIdObject;
    int randomId;

    do {
        currentRoot = (IdentityDict *) this->space->mutableRoot.load();
        oldRoot = currentRoot;

        randomId = rand();
        randomIdObject = this->fromInteger(randomId);

        BOOLEAN existingMutable = currentRoot->has(this, randomIdObject);
        if (existingMutable)
            continue;

    } while (!this->space->mutableRoot.compare_exchange_strong(
        oldRoot,
        currentRoot->setAt(
            this,
            randomIdObject,
            value
        )
    ));

    ProtoObjectPointer p;
    p.oid.oid = NULL;
    p.mutableObject.pointer_tag = POINTER_TAG_MUTABLEOBJECT;
    p.mutableObject.mutableID = randomId;

    return p.oid.oid;
};

ProtoThread *ProtoContext::getCurrentThread() {
    return this->thread;
}

int compareStrings(ProtoList *string1, ProtoList *string2) {
    int string1Size = string1->getSize(&literalContext);
    int string2Size = string2->getSize(&literalContext);

    int i;
    for (i = 0; i <= string1Size && i <= string2Size; i++) {
        ProtoObjectPointer string1Char;
        string1Char.oid.oid = string1->getAt(&literalContext, i);
        ProtoObjectPointer string2Char;
        string2Char.oid.oid = string2->getAt(&literalContext, i);

        if (string1Char.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE || 
            string1Char.op.embedded_type != EMBEDED_TYPE_UNICODECHAR)
            return -1;
        
        if (string2Char.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE || 
            string2Char.op.embedded_type != EMBEDED_TYPE_UNICODECHAR)
            return 1;
        
        if (string1Char.unicodeChar.unicodeValue < string2Char.unicodeChar.unicodeValue)
            return -1;
        else if (string1Char.unicodeChar.unicodeValue > string2Char.unicodeChar.unicodeValue)
            return 1;
    }
    if (i > string1Size)
        return -1;
    else if (i > string2Size)
        return 1;
    return 0;
}

ProtoList *LiteralDictionary::getFromString(
    ProtoList *string
) {
	if (!this->key)
		return NULL;

    LiteralDictionary *node = this;
	while (node) {
		if (node->key == key)
			return node->key;
        int cmp = compareStrings(string, this->key);
        if (cmp < 0)
            node = node->previous;
        else if (cmp > 1)
            node = node->next;
	}

    if (node)
        return node->key;
    else
        return NULL;
};

ProtoList *LiteralDictionary::get(char *zeroTerminatedUTF8CharString) {
	if (!this->key)
		return NULL;

    LiteralDictionary *node = this;
	while (node) {
		if (node->key == key)
			return node->key;

        char *currentChar = zeroTerminatedUTF8CharString;
        int keySize = node->key->getSize(context);
        int cmp = 0;
        int i;
        for (i = 0; i <= keySize && *currentChar; i++) {
            ProtoObjectPointer keyChar;
            keyChar.oid.oid = node->key->getAt(context, i);
            ProtoObjectPointer stringChar;
            stringChar.oid.oid = context->fromUTF8Char(currentChar);
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

            if (keyChar.unicodeChar.unicodeValue > stringChar.unicodeChar.unicodeValue) {
                cmp = -1;
                node = node->previous;
                break;
            }
            else if (keyChar.unicodeChar.unicodeValue < stringChar.unicodeChar.unicodeValue) {
                cmp = 1;
                node = node->next;
                break;
            }
        }

        if (cmp == 0 && i > keySize)
            node = node->previous;
        else if (cmp == 0 && *currentChar)
            node = node->next;
        else if (cmp == 0 && i == keySize && *currentChar)
            break;
	}

    if (node)
        return node->key;
    else
        return NULL;

};

LiteralDictionary::LiteralDictionary(
    ProtoContext *context,

    ProtoList *key = NULL,
    LiteralDictionary *previous = NULL,
    LiteralDictionary *next = NULL
):Cell(
    context, 
    type = CELL_TYPE_LITERAL_DICT,
    height = 1 + max(previous? previous->height : 0, next? next->height : 0),
    count = (key? 1: 0) + (previous? previous->count : 0) + (next? next->count : 0)
) {
    this->key = key;
    this->previous = previous;
    this->next = next;
};

LiteralDictionary::~LiteralDictionary() {

};

int getBalance(LiteralDictionary *self) {
	if (self->next && self->previous)
		return self->next->height - self->previous->height;
	else if (self->previous)
		return -self->previous->height;
	else if (self->next)
		return self->next->height;
	else
		return 0;
}

// A utility function to right rotate subtree rooted with y
// See the diagram given above.
LiteralDictionary *rightRotate(LiteralDictionary *n)
{
    LiteralDictionary *newRight = new(&literalContext) LiteralDictionary(
        &literalContext,
        n->key,
        n->previous->next,
        n->next
    );
    return new(&literalContext) LiteralDictionary(
        &literalContext,
        n->previous->key,
        n->previous->previous,
        newRight
    );
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
LiteralDictionary *leftRotate(LiteralDictionary *n) {
    LiteralDictionary *newLeft = new(&literalContext) LiteralDictionary(
        &literalContext,
        n->key,
        n->previous,
        n->next->previous
    );
    return new(&literalContext) LiteralDictionary(
        &literalContext,
        n->next->key,
        newLeft,
        n->next->next
    );
}

LiteralDictionary *rebalance(LiteralDictionary *newNode) {
    while (TRUE) {
        int balance = getBalance(newNode);

        // If this node becomes unbalanced, then
        // there are 4 cases

        // Left Left Case
        if (balance < -1) {
            newNode = rightRotate(newNode);
        }
        else {
            // Right Right Case
            if (balance > 1) {
                newNode = leftRotate(newNode);
            }
            // Left Right Case
            else {
                if (balance < 0 && getBalance(newNode->previous) > 0) {
                    newNode = new(&literalContext) LiteralDictionary(
                        &literalContext,
                        newNode->key,
                        leftRotate(newNode->previous),
                        newNode->next
                    );
                    newNode = rightRotate(newNode);
                }
                else {
                    // Right Left Case
                    if (balance > 0 && getBalance(newNode->next) < 0) {
                        newNode = new(&literalContext) LiteralDictionary(
                            &literalContext,
                            newNode->key,
                            newNode->previous,
                            rightRotate(newNode->next)
                        );
                        newNode = leftRotate(newNode);
                    }
                    else
                        return newNode;
                }
            }
        }
    }
}

LiteralDictionary *LiteralDictionary::set(ProtoList *string) {
	LiteralDictionary *newNode;
	int cmp;

	// Empty tree case
	if (!this->key)
        return new(&literalContext) LiteralDictionary(
            &literalContext,
            key = string
        );

    cmp = compareStrings(string, this->key);
    if (cmp > 0) {
        if (this->next) {
            newNode = new(&literalContext) LiteralDictionary(
                &literalContext,
                key = this->key,
                previous = this->previous,
                next = this->next->set(string)
            );
        }
        else {
            newNode = new(&literalContext) LiteralDictionary(
                &literalContext,
                key = this->key,
                previous = this->previous,
                next = new(&literalContext) LiteralDictionary(
                    &literalContext,
                    key = this->key
                )
            );
        }
    }
    else if (cmp < 0) {
        if (this->previous) {
            newNode = new(&literalContext) LiteralDictionary(
                &literalContext,
                key = this->key,
                previous = this->previous->set(string),
                next = this->next
            );
        }
        else {
            newNode = new(&literalContext) LiteralDictionary(
                &literalContext,
                key = this->key,
                previous = new(&literalContext) LiteralDictionary(
                    &literalContext,
                    key = this->key
                ),
                next = this->next
            );
        }
    }
    else 
        return this;

    return rebalance(newNode);
};

};
