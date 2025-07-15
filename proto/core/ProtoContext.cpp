/*
 * ProtoContext.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto_internal.h"

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
            ((ProtoThreadImplementation *) this->thread)->currentContext = this;
    }
    else {
        this->space = space;
        this->thread = thread;
        if (thread)
            ((ProtoThreadImplementation *) this->thread)->currentContext = this;
    }

    this->lastAllocatedCell = NULL;
    this->localsBase = localsBase;
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

void ProtoContext::setReturnValue(ProtoContext *context, ProtoObject *returnValue) {
    if (this->previous) {
        // Ensure a reference to the last return value during context destruction
        this->previous->lastReturnValue = returnValue;
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
        newCell = ((ProtoThreadImplementation *) this->thread)->allocCell();
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

ProtoList<ProtoObject> *ProtoContext::newList() {
    ProtoList<ProtoObject> *list = new(this) ProtoListImplementation<ProtoObject>(this);
    return list;
}

ProtoTuple *ProtoContext::newTuple() {
    ProtoTuple *tuple = new(this) ProtoTupleImplementation(this);
    return tuple;
}

ProtoSparseList<ProtoObject> *ProtoContext::newSparseList() {
    ProtoSparseList<ProtoObject> *sparseList = new(this) ProtoSparseListImplementation<ProtoObject>(this);
    return sparseList;
}

ProtoString *ProtoContext::fromUTF8String(const char *zeroTerminatedUtf8String) {
    const char *currentChar = zeroTerminatedUtf8String;
    ProtoList<ProtoObject> *string = this->newList();

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

    return new(this) ProtoStringImplementation(
        this,
        ProtoTupleImplementation::tupleFromList(this, string)
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
    return new(this) ProtoMethodCellImplementation(this, self, method);
};

ProtoExternalPointer *ProtoContext::fromExternalPointer(void *pointer) {
    return new(this) ProtoExternalPointerImplementation(this, pointer);
};

ProtoByteBuffer *ProtoContext::fromBuffer(unsigned long length, char* buffer) {
    return new(this) ProtoByteBufferImplementation(this, length, buffer);
};

ProtoByteBuffer *ProtoContext::newBuffer(unsigned long length) {
    return new(this) ProtoByteBufferImplementation(this, length);
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

void ProtoContext::addCell2Context(Cell *newCell) {
    newCell->nextCell = this->lastAllocatedCell;
    this->lastAllocatedCell = newCell;
}

};
