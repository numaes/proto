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

ProtoContextImplementation::ProtoContextImplementation(
		ProtoContextImplementation *previous = NULL,
		ProtoSpace *space = NULL,
		ProtoThread *thread = NULL
) : ProtoContext() {

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

ProtoContextImplementation::~ProtoContextImplementation() {
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

void *ProtoContext::operator new(size_t size) {
    return malloc(sizeof(ProtoContextImplementation));
};

Cell *ProtoContext::allocCell(){
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    return self->thread->allocCell();
};

void collectCells(ProtoContext *context, void *self, ProtoObject *value) {
    ProtoContextImplementation *target = (ProtoContextImplementation *) self;

    ProtoObjectPointer p;
    p.oid = value;

    if (p.op.pointer_tag == POINTER_TAG_CELL) {
        // It is an object pointer with references
        target->returnSet->add(context, p.oid);
    }
}

void ProtoContext::returnValue(ProtoObject *value=PROTO_NONE){
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    if (value != NULL) {
        self->returnSet = new(self) ProtoSet();

        value->processReferences(self, collectCells);
    }
};

ProtoObject *ProtoContext::fromInteger(int value) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    ProtoObjectPointer p;
    p.si.pointer_tag = POINTER_TAG_SMALLINT;
    p.si.smallInteger = value;

    return p.oid; 
};

ProtoObject *ProtoContext::fromDouble(double value) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    ProtoObjectPointer p;
    p.sd.pointer_tag = POINTER_TAG_SMALLDOUBLE;
    p.si.smallInteger = ((unsigned long) value) >> TYPE_SHIFT;

    return p.oid; 
};

ProtoObject *ProtoContext::fromUTF8Char(char *utf8OneCharString) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

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
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    char *currentChar = zeroTerminatedUtf8String;
    ProtoList *string = new(self) ProtoList();

    while (*currentChar) {
        ProtoObject *oneChar = self->fromUTF8Char(currentChar);
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

ProtoObject *ProtoContext::fromMethod(ProtoMethod method) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    ProtoObjectPointer p;
    p.methodPointer = method;
    p.op.pointer_tag = POINTER_TAG_METHOD;

    return p.oid;
};

ProtoObject *ProtoContext::fromBuffer(char *pointer, unsigned long length) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    ProtoByteBuffer *buffer = new(self) ProtoByteBuffer(pointer, length);

    ProtoObjectPointer p;
    p.cell = buffer;
    p.op.pointer_tag = POINTER_TAG_BYTE_BUFFER;

    return p.oid;
};

ProtoObject *ProtoContext::fromBoolean(BOOLEAN value) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    ProtoObjectPointer p;
    p.booleanValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.booleanValue.embedded_type = EMBEDED_TYPE_BOOLEAN;
    p.booleanValue.booleanValue = value;

    return p.oid;
};

ProtoObject *ProtoContext::fromByte(char c) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    ProtoObjectPointer p;
    p.byteValue.pointer_tag = POINTER_TAG_EMBEDEDVALUE;
    p.byteValue.embedded_type = EMBEDED_TYPE_BYTE;
    p.byteValue.byteData = c;

    return p.oid;
};

ProtoObject *ProtoContext::literalFromString(char *zeroTerminatedUtf8String) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    return literalRoot.load()->getFromZeroTerminatedString(
        self, 
        zeroTerminatedUtf8String
    );
};

ProtoObject *ProtoContext::newMutable(ProtoObject *value=PROTO_NONE) {
    ProtoContextImplementation *self = (ProtoContextImplementation *) this;

    int randomId;
    IdentityDict *newRoot, *currentRoot;

    do {
        randomId = rand();
        currentRoot = self->space->mutableRoot;
        newRoot = currentRoot->setAt(
            self,
            self->fromInteger(randomId), 
            value);
    } while (!self->space->mutableRoot.compare_exchange_strong(
        currentRoot, 
        newRoot
    ));

    ProtoObjectPointer p;
    p.mutableObject.pointer_tag = POINTER_TAG_MUTABLEOBJECT;
    p.mutableObject.mutableID = randomId;

    return p.oid;

};
