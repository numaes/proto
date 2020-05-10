/*
 * ProtoContext.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"
#include "../headers/proto_internal.h"
#include "malloc.h"
#include "random"

std::atomic<LiteralDictionary *> *literalRoot;

ProtoContext::ProtoContext(
		ProtoContext *previous = NULL,
		ProtoSpace *space = NULL,
		ProtoThread *thread = NULL
) {

    if (previous) {
        this->previous = previous;
        this->space = this->previous->space;
        this->thread = this->previous->thread;
    }

    this->returnSet = NULL;
    this->returnChain = NULL;

    if (this->thread)
        this->lastCellPreviousContext = this->thread->nextCell;
};

ProtoContext::~ProtoContext() {
    Cell *freeCells = NULL;

    // Generate the chain of cells for the return value if needed

    if (this->thread && this->returnSet) {
        Cell *returnChain = this->lastCellPreviousContext;
        Cell *currentCell = this->thread->nextCell;

        while (currentCell && currentCell != this->lastCellPreviousContext) {
            if (this->returnSet->has(this, (ProtoObject *) currentCell) != PROTO_TRUE) {
                currentCell->nextCell = freeCells;
                freeCells = currentCell;
            }
            else {
                currentCell->nextCell = returnChain;
                returnChain = currentCell;
            }
        }

        this->thread->nextCell = returnChain;
    }

    // Return all dirty blocks to space

    if (freeCells && this->space)
        this->space->analyzeUsedCells(freeCells);

};

Cell *ProtoContext::allocCell(){
    return this->thread->allocCell();
};

void collectCells(ProtoContext *context, void *self, Cell *value) {
    ProtoObjectPointer p;
    p.oid = (ProtoObject *) value;

    if (p.op.pointer_tag == POINTER_TAG_CELL) {
        // It is an object pointer with references
        context->returnSet->add(context, p.oid);
    }
}

void ProtoContext::returnValue(ProtoObject *value=PROTO_NONE){
    if (value != NULL && value != PROTO_NONE) {
        this->returnSet = new(this) ProtoSet(this);

        ProtoObjectPointer p;
        p.oid = value;

        if (p.op.pointer_tag == POINTER_TAG_CELL) {
            p.cell->processReferences(this, this, collectCells);

        }
    }
};

ProtoObject *ProtoContext::fromInteger(int value) {
    ProtoObjectPointer p;
    p.si.pointer_tag = POINTER_TAG_SMALLINT;
    p.si.smallInteger = value;

    return p.oid; 
};

ProtoObject *ProtoContext::fromDouble(double value) {
    ProtoObjectPointer p;
    p.sd.pointer_tag = POINTER_TAG_SMALLDOUBLE;
    p.si.smallInteger = ((unsigned long) value) >> TYPE_SHIFT;

    return p.oid; 
};

ProtoObject *ProtoContext::fromUTF8Char(char *utf8OneCharString) {
    ProtoObjectPointer p;
    p.unicodeChar.pointer_tag = POINTER_TAG_SMALLDOUBLE;
    p.unicodeChar.embedded_type = EMBEDED_TYPE_UNICODECHAR;

    unsigned unicodeValue;
    if (( utf8OneCharString[0] & 0x80 ) == 0 )
        // 0000 0000-0000 007F | 0xxxxxxx
        unicodeValue = utf8OneCharString[0] & 0x7F;
    else if (( utf8OneCharString[0] & 0xE0 ) == 0xC0 )
        // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
        unicodeValue = 0x80 + 
                       (utf8OneCharString[0] & 0x1F) << 6 + 
                       (utf8OneCharString[1] & 0x3F);
    else if (( utf8OneCharString[0] & 0xF0 ) == 0xE0 ) 
        // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
        unicodeValue = 0x800 + 
                       (utf8OneCharString[0] & 0x0F) << 12 + 
                       (utf8OneCharString[1] & 0x3F) << 6 + 
                       (utf8OneCharString[2] & 0x3F);
    else if (( utf8OneCharString[0] & 0xF8 ) == 0xF0 )
        // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        unicodeValue = 0x10000 + 
                       (utf8OneCharString[0] & 0x07) << 18 + 
                       (utf8OneCharString[1] & 0x3F) << 12 + 
                       (utf8OneCharString[1] & 0x3F) << 6 + 
                       (utf8OneCharString[2] & 0x3F);

    p.unicodeChar.unicodeValue = unicodeValue;

    return p.oid; 
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
    }

    return string;
};

ProtoObject *ProtoContext::fromMethod(ProtoObject *self, ProtoMethod *method) {
    return new(this) ProtoMethodCell(this, self, method);
};

ProtoObject *ProtoContext::fromBuffer(char *pointer, unsigned long length) {
    return new(this) ProtoByteBuffer(this, pointer, length);
};

ProtoObject *ProtoContext::fromBoolean(BOOLEAN value) {
    ProtoObjectPointer p;
    p.booleanValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.booleanValue.embedded_type = EMBEDED_TYPE_BOOLEAN;
    p.booleanValue.booleanValue = value;

    return p.oid;
};

ProtoObject *ProtoContext::fromByte(char c) {
    ProtoObjectPointer p;
    p.byteValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.byteValue.embedded_type = EMBEDED_TYPE_BYTE;
    p.byteValue.byteData = c;

    return p.oid;
};

ProtoObject *ProtoContext::literalFromString(char *zeroTerminatedUtf8String) {
    LiteralDictionary *newRoot, *currentRoot;

    ProtoObject *literalString = this->fromUTF8String(zeroTerminatedUtf8String);

    do {
        currentRoot = literalRoot->load();

        ProtoObject *currentLiteral = currentRoot->get(literalString);
        if (currentLiteral != PROTO_NONE)
            return currentLiteral;

        newRoot = currentRoot->set(
            this,
            literalString);
    } while (!literalRoot->compare_exchange_strong(
        currentRoot, 
        newRoot
    ));

    return literalString;
};

ProtoObject *ProtoContext::newMutable(ProtoObject *value=PROTO_NONE) {
    IdentityDict *newRoot, *currentRoot;
    int randomId;

    do {
        currentRoot = (IdentityDict *) this->space->mutableRoot.load();
        randomId = rand();
        ProtoObject *randomIdObject = this->fromInteger(randomId);

        BOOLEAN existingMutable = currentRoot->has(this, randomIdObject);
        if (existingMutable)
            continue;

        newRoot = currentRoot->setAt(
            this,
            randomIdObject,
            value
        );

    } while (!this->space->mutableRoot.compare_exchange_strong(
        (Cell *&) currentRoot,
        (Cell *&) newRoot
    ));

    ProtoObjectPointer p;
    p.mutableObject.pointer_tag = POINTER_TAG_MUTABLEOBJECT;
    p.mutableObject.mutableID = randomId;

    return p.oid;
};

ProtoThread *ProtoContext::getCurrentThread() {
    return this->thread;
}
