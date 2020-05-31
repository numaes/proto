/*
 * test_proto.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"

#include <iostream>
using namespace std;
using namespace proto;

int failedTests = 0;


void printString(ProtoContext *context, ProtoObject *string) {
    ProtoList *s = (ProtoList *) string;
    int size = (int) s->getSize(context);

    for (int i = 0; i < size; i++) {
        ProtoObject *unicodeChar = s->getAt(context, i);
        ProtoObjectPointer p;
        p.oid.oid = unicodeChar;
        if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
            p.op.embedded_type == EMBEDED_TYPE_UNICODECHAR) {
            char buffer[8];

            if (p.unicodeChar.unicodeValue <= 0x7f) {
                buffer[0] = (char) p.unicodeChar.unicodeValue;
                buffer[1] = 0;
            }
            else {
                if (p.unicodeChar.unicodeValue >= 0x80 &&
                    p.unicodeChar.unicodeValue <= 0x7ff) {
                    long u = p.unicodeChar.unicodeValue - 0x80;
                    buffer[0] = (char) (u >> 6) & 0x07;
                    buffer[1] = (char) u & 0x3F;
                    buffer[2] = 0;
                }
                else {
                    if (p.unicodeChar.unicodeValue >= 0x800 &&
                        p.unicodeChar.unicodeValue <=0x10ffff) {
                        long u = p.unicodeChar.unicodeValue - 0x800;
                        buffer[0] = (char) (u >> 12) & 0x07;
                        buffer[1] = (char) (u >> 6) & 0x3F;
                        buffer[2] = (char) u & 0x3F;
                        buffer[3] = 0;
                    }
                    else {
                        long u = p.unicodeChar.unicodeValue - 0x10000;
                        buffer[0] = (char) (u >> 18) & 0x07;
                        buffer[1] = (char) (u >> 12) & 0x3F;
                        buffer[2] = (char) (u >> 12) & 0x3F;
                        buffer[3] = (char) u & 0x3F;
                        buffer[4] = 0;
                    }
                }
            }
            cout << buffer;
        }
    }
}

ProtoObject *testMethod(
    ProtoContext *context, 
    ProtoObject *self, 
    ProtoObject *base, 
    ProtoObject *args, 
    ProtoObject *kwargs) {
    return PROTO_NONE;
};

BOOLEAN test_proto_header() {
    cout << "\nTesting headers";

    cout << "\nStep 01 - ProtoObjectPointer";

    if (sizeof(ProtoObjectPointer) != 8) {
        cout << "ProtoObjectPointer size is " 
             << sizeof(ProtoObjectPointer) 
             << "! Please check it";
        return TRUE;
    }

    if (sizeof(BigCell) != 64) {
        cout << "BigCell should be 64 bytes long!";
        return TRUE;
    }

    if (sizeof(BigCell) < sizeof(ProtoThread)) {
        cout << "ProtoThread is bigger than BigCell" 
             << sizeof(ProtoThread) 
             << "! Please check it";
        return TRUE;
    }

    if (sizeof(BigCell) < sizeof(IdentityDict)) {
        cout << "IdentityDict is bigger than BigCell" 
             << sizeof(IdentityDict) 
             << "! Please check it";
        return TRUE;
    }

    return FALSE;
};

BOOLEAN test_cell() {
    cout << "\n\nTesting Cell";


    return FALSE;
};

BOOLEAN test_identityDict() {
    cout << "\n\nTesting IdentitySet";

    ProtoSpace *s = new ProtoSpace();
    ProtoContext *c = new ProtoContext(
        NULL,
        s
    );

    cout << "\nStep 01 Creating";

    IdentityDict *d = new(c) IdentityDict(c);

    cout << "\nStep 02 Adding";

    ProtoObject *s1 = c->fromUTF8String((char *) "First");
    ProtoObject *s2 = c->fromUTF8String((char *) "Second");
    ProtoObject *s3 = c->fromUTF8String((char *) "Third");
    ProtoObject *s4 = c->fromUTF8String((char *) "Fourh");

    d = d->setAt(c, s1, c->fromInteger(1));
    d = d->setAt(c, s2, c->fromInteger(2));
    d = d->setAt(c, s3, c->fromInteger(3));
    d = d->setAt(c, s4, c->fromInteger(4));

    if (d->getSize(c) != 4) {
        cout << "\nError on size";
        return TRUE;
    }

    if (d->getAt(c, s1) != c->fromInteger(1)) {
        cout << "\nMismatch on adding";
        return TRUE;
    }

    cout << "\nStep 03 Replace";

    d = d->setAt(c, s2, c->fromInteger(1));

    if (d->getSize(c) != 4 ||
        d->getAt(c, s2) != c->fromInteger(1)) {
        cout << "\nReplace is not working";
        return TRUE;
    }

    cout << "\nStep 04 Remove";

    d = d->removeAt(c, s2);

    if (d->getSize(c) != 3 ||
        !d->has(c, s1) ||
        !d->has(c, s3) ||
        !d->has(c, s4) ||
        d->has(c, s2)) {
        cout << "\nRemove is not working";
        return TRUE;
    }

    cout << "\nStep 05 isEqual";

    IdentityDict *d2 = new(c) IdentityDict(c);

    d2 = d2->setAt(c, s1, c->fromInteger(1));
    d2 = d2->setAt(c, s3, c->fromInteger(3));

    if (d->isEqual(c, d2)) {
        cout << "\nisEqual with diferent dicts is not working";
        return TRUE;
    }

    d2 = d2->setAt(c, s4, c->fromInteger(4));

    if (!d->isEqual(c, d2)) {
        cout << "\nisEqual with equal dicts is not working";
        return TRUE;
    }

    return FALSE;
};

BOOLEAN test_protoSet() {
    cout << "\n\nTesting ProtoSet";

    ProtoSpace *s = new ProtoSpace();
    ProtoContext *c = new ProtoContext(
        NULL,
        s
    );

    cout << "\nStep 01 Creating";

    ProtoSet *s1 = new(c) ProtoSet(c);
    int i, j;

    cout << "\nStep 01 Add";

    for (i = 0; i < 10; i++)
        s1 = s1->add(c, c->fromInteger(i));

    for (i = 0; i < 10; i++) {
        if (!s1->has(c, c->fromInteger(i))) {
            cout << "\nSomething wrong with add";
            return TRUE;
        }
    }

    if (s1->has(c, c->fromInteger(100))) {
        cout << "\nSomething wrong with has";
        return TRUE;
    }

    cout << "\nStep 02 Remove";

    s1 = s1->removeAt(c, c->fromInteger(5));

    if (s1->has(c, c->fromInteger(5))) {
        cout << "\nSomething wrong with remove";
        return TRUE;
    }

    ProtoSet *s2 = new(c) ProtoSet(c);

    for (i = 0; i < 10; i++)
        s2 = s2->add(c, c->fromInteger(i));

    if (s1->isEqual(c, s2)) {
        cout << "\nSomething wrong with isEqual";
        return TRUE;
    };

    s1 = s1->add(c, c->fromInteger(5));

    if (s1->isEqual(c, s2)) {
        cout << "\nSomething wrong with isEqual";
        return TRUE;
    };

    return FALSE;
};

BOOLEAN test_parentLink() {
    cout << "\n\nTesting ParentLink";


    return FALSE;
};

BOOLEAN test_byteBuffer() {
    cout << "\n\nTesting ByteBuffer";

    return FALSE;
};

int countBlocks(Cell *cell) {
    int count = 0;
    while (cell) {
        count++;
        cell = cell->nextCell;
    }
    return count;
}

BOOLEAN test_protoContext() {
    cout << "\n\nTesting ProtoContext";

    cout << "\nStep 01 - ProtoContext basic";

    ProtoSpace *s = new ProtoSpace();
    s->~ProtoSpace();

    ProtoContext *c = new ProtoContext(
        s->creationContext
    );

    ProtoObjectPointer p;
    ProtoObject *o, *o2, *o3;
    
    cout << "\nStep 02 - Embedded types - Integer";
    o = c->fromInteger(37);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_SMALLINT ||
        p.si.smallInteger != 37) {
        cout << "\nError on Integer representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - Doubles";
    o = c->fromDouble(45.78898387777);

    p.oid.oid = o;
    union {
        unsigned long lv;
        double dv;
    } u;
    u.lv = p.sd.smallDouble << TYPE_SHIFT;

    if (p.op.pointer_tag != POINTER_TAG_SMALLDOUBLE ||
        abs(u.dv - 45.78898387777) > 0.0000000001) {
        cout << "\nError on Double representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - Boolean";
    o = c->fromBoolean(FALSE);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_BOOLEAN ||  
        p.booleanValue.booleanValue != FALSE) {
        cout << "\nError on Boolean representation";
        return TRUE;
    }

    o = c->fromBoolean(TRUE);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_BOOLEAN ||  
        p.booleanValue.booleanValue != TRUE) {
        cout << "\nError on Boolean representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - BYTE";
    o = c->fromByte(38);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_BYTE ||  
        p.byteValue.byteData != 38) {
        cout << "\nError on Byte representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - Date";
    o = c->fromDate(2020, 05, 01);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_DATE ||  
        p.date.year != 2020 ||
        p.date.month != 05 ||
        p.date.day != 01) {
        cout << "\nError on Date representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - Timestamp";
    o = c->fromTimestamp(2993948);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_TIMESTAMP ||  
        p.timestampValue.timestamp != 2993948) {
        cout << "\nError on Timestamp representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - UTF8Char";
    o = c->fromUTF8Char((char *) "Ñ");

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_UNICODECHAR ||  
        p.unicodeChar.unicodeValue != 337) {
        cout << "\nError on UTF8 char representation";
        return TRUE;
    }

    cout << "\nStep 03 - Embedded types - Timedelta";
    o = c->fromTimeDelta(3834848);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != EMBEDED_TYPE_TIMEDELTA ||  
        p.timedeltaValue.timedelta != 3834848) {
        cout << "\nError on Timedelta representation";
        return TRUE;
    }

    cout << "\nStep 04 - Cell types - External Pointers";
    o = c->fromExternalPointer(&o);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_CELL ||
        p.cell.cell->type != CELL_TYPE_EXTERNAL_POINTER ||  
        ((ProtoExternalPointer *) p.cell.cell)->pointer != &o) {
        cout << "\nError on Pointer cell";
        return TRUE;
    }

    cout << "\nStep 04 - Cell types - Byte buffer";
    o = c->newBuffer(10);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_CELL ||
        p.cell.cell->type != CELL_TYPE_BYTE_BUFFER ||  
        ((ProtoByteBuffer *) p.cell.cell)->size != 10 ||
        ((ProtoByteBuffer *) p.cell.cell)->buffer == NULL) {
        cout << "\nError on Byte buffer";
        return TRUE;
    }

    ((ProtoByteBuffer *) p.cell.cell)->~ProtoByteBuffer();

    cout << "\nStep 04 - Cell types - Method call";
    ProtoObject *self = c->fromByte(22);
    o = c->fromMethod(self, &testMethod);

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_CELL ||
        p.cell.cell->type != CELL_TYPE_METHOD ||  
        ((ProtoMethodCell *) p.cell.cell)->self != self ||
        ((ProtoMethodCell *) p.cell.cell)->method != &testMethod) {
        cout << "\nError on Method";
        return TRUE;
    }

    cout << "\nStep 04 - Cell types - UTF8String";
    o = c->fromUTF8String((char *) "Ñoño");

    p.oid.oid = o;
    if (p.op.pointer_tag != POINTER_TAG_CELL ||
        p.cell.cell->type != CELL_TYPE_PROTO_LIST ||  
        ((ProtoList *) p.cell.cell)->count != 4) {
        cout << "\nError on UTF8 String";
        return TRUE;
    }

    if (((ProtoList *) p.cell.cell)->getAt(c, 0) != c->fromUTF8Char((char *) "Ñ") ||
        ((ProtoList *) p.cell.cell)->getAt(c, 1) != c->fromUTF8Char((char *) "o") ||
        ((ProtoList *) p.cell.cell)->getAt(c, 2) != c->fromUTF8Char((char *) "ñ") ||
        ((ProtoList *) p.cell.cell)->getAt(c, 3) != c->fromUTF8Char((char *) "o")) {
        cout << "\nError on UTF8 String content";
        return TRUE;
    }

    cout << "\nStep 05 - Proto.h as* and is* methods";

    if (c->fromBoolean(TRUE)->asBoolean() != TRUE ||
        c->fromBoolean(FALSE)->asBoolean() != FALSE ||
        c->fromBoolean(FALSE)->isBoolean() != TRUE) {
        cout << "\nError on asBoolean or isBoolean";
        return TRUE;
    };

    if (c->fromInteger(64744)->asInteger() != 64744 ||
        c->fromBoolean(FALSE)->asInteger() != 0 ||
        c->fromInteger(3999)->isInteger() != TRUE) {
        cout << "\nError on asInteger or isInteger";
        return TRUE;
    };

    if (c->fromDouble(1.0)->asDouble() != 1.0 ||
        c->fromBoolean(FALSE)->asDouble() != 0.0 ||
        c->fromDouble(3.4848548484)->isDouble() != TRUE) {
        cout << "\nError on asDouble or isDouble";
        return TRUE;
    };

    if (c->fromByte(45)->asByte() != 45 ||
        c->fromBoolean(FALSE)->asByte() != 0 ||
        c->fromByte(38)->isByte() != TRUE) {
        cout << "\nError on asByte or isByte";
        return TRUE;
    };

    cout << "\nStep 06 - Context go and return";

    int currentCount = countBlocks(c->thread->currentWorkingSet);

    ProtoContext *newContext = new ProtoContext(c);
    newContext->~ProtoContext();

    if (currentCount != countBlocks(c->thread->currentWorkingSet)) {
        cout << "\nUnbalanced count on subcontext";
        return TRUE;
    }

    newContext = new ProtoContext(c);
    new(newContext) ProtoSet(newContext);
    if (countBlocks(c->thread->currentWorkingSet) != (currentCount + 1)) {
        cout << "\nWorkingSet incorrect";
        return TRUE;
    }
    newContext->setReturnValue(new(newContext) ProtoSet(newContext));
    newContext->~ProtoContext();

    if ((currentCount + 1) != countBlocks(c->thread->currentWorkingSet)) {
        cout << "\nUnbalanced count on subcontext after return";
        return TRUE;
    }

    newContext->~ProtoContext();

    newContext = NULL;

    return FALSE;
};

BOOLEAN test_externalPointer() {
    cout << "\n\nTesting ExternalPointer";

    return FALSE;
};

BOOLEAN test_protoList() {
    cout << "\n\nTesting ProtoList";

    ProtoSpace *s = new ProtoSpace();
    s->~ProtoSpace();

    ProtoContext *c = new ProtoContext(
        s->creationContext
    );

    cout << "\nStep 01 Creating";

    int i, j;

    ProtoList *l = new(c) ProtoList(c);

    if (l->getSize(c) != 0) {
        cout << "\nWrong size on creation";
        return TRUE;
    }    

    cout << "\nStep 02 Appends";

    for (i = 0; i < 100; i++)
        l = l->appendLast(c, c->fromInteger(i));

    for (i = 0; i < 100; i++) {
        if (l->getAt(c, i) != c->fromInteger(i)) {
            cout << "\nSequence error on appendLast";
            return TRUE;
        }
    }

    for (i = -1; i >= -100; i--) {
        l = l->appendFirst(c, c->fromInteger(i));
    }

    i = 0;

    j = -100;

    while (i < 200) {
        if (l->getAt(c, i) != c->fromInteger(j)) {
            cout << "\nSequence error on appendFirst";
            return TRUE;
        }
        i++;
        j++;
    };

    if (l->getAt(c, -1) != c->fromInteger(99)) {
        cout << "\nGet (negative index) failed";
        return TRUE;
    }

    cout << "\nStep 03 Remove";

    l = l->removeFirst(c);
    if (l->getAt(c, 0) != c->fromInteger(-99)) {
        cout << "\nSomething wrong in removeFirst";
        return TRUE;
    }

    l = l->removeLast(c);
    if (l->getAt(c, 197) != c->fromInteger(98)) {
        cout << "\nSomething wrong in removeLast";
        return TRUE;
    }

    l = l->removeAt(c, 4);
    if (l->getAt(c, 3) != c->fromInteger(-96) && 
        l->getAt(c, 4) != c->fromInteger(-94)) {
        cout << "\nSomething wrong in removeAt";
        return TRUE;
    }

    cout << "\nStep 04 Set";

    l = l->setAt(c, 4, c->fromInteger(201));
    if (l->getAt(c, 4) != c->fromInteger(201)) {
        cout << "\nSomething wrong in setAt";
        return TRUE;
    }

    cout << "\nStep 05 Has";

    if (!l->has(c, c->fromInteger(201))) {
        cout << "\nSomething went wrong in has";
        return TRUE;
    }

    cout << "\nStep 06 InsertAt";

    l = l->insertAt(c, 4, c->fromInteger(202));
    if (l->getAt(c, 3) != c->fromInteger(-97) && 
        l->getAt(c, 4) != c->fromInteger(202) &&
        l->getAt(c, 5) != c->fromInteger(201)) {
        cout << "\nSomething wrong in insertAt";
        return TRUE;
    }

    cout << "\nStep 07 Get";

    if (l->getFirst(c) != c->fromInteger(-99)) {
        cout << "\nSomething wrong in getFirst";
        return TRUE;
    }

    if (l->getLast(c) != c->fromInteger(98)) {
        cout << "\nSomething wrong in getLast";
        return TRUE;
    }

    l = new(c) ProtoList(c);

    for (i = 0; i < 100; i++)
        l = l->appendLast(c, c->fromInteger(i));

    cout << "\nStep 08 getSlice";

    ProtoList *l2 = l->getSlice(c, 30, 40);
    for (i = 30; i < 40; i++) {
        if (l2->getAt(c, i-30) != c->fromInteger(i)) {
            cout << "\nSomething wrong in getSlice";
            return TRUE;
        };
    }

    return FALSE;
};

BOOLEAN test_protoLiteral() {
    cout << "\n\nTesting ProtoLiteral";


    return FALSE;
};

BOOLEAN test_memoryBuffer() {
    cout << "\n\nTesting ByteBuffer";


    return FALSE;
};

BOOLEAN test_methodCall() {
    cout << "\n\nTesting MethodCall";


    return FALSE;
};

BOOLEAN test_protoObject() {
    cout << "\n\nTesting Proto";


    return FALSE;
};

BOOLEAN test_protoSpace() {
    cout << "\n\nTesting ProtoSpace";

    cout << "\nStep 01 - Creating and deleting";
    ProtoSpace *s = new ProtoSpace();
    s->~ProtoSpace();

    cout << "\nStep 02 - Creating with context";
    s = new ProtoSpace();
    ProtoContext *c = new ProtoContext(
        s->creationContext
    );
    c->~ProtoContext();
    s->~ProtoSpace();
    
    cout << "\nStep 03 - Creating with context and return value";
    s = new ProtoSpace();
    *c = new ProtoContext(
        s->creationContext
    );

    ProtoSet *set = new(c) ProtoSet(c);
    c->setReturnValue(set);

    c->~ProtoContext();
    s->~ProtoSpace();
    
    return FALSE;
};

BOOLEAN test_protoThread() {
    cout << "\n\nTesting Thread";


    return FALSE;
};


BOOLEAN main(BOOLEAN argc, char **argv) {

    int phase;
    int error;

    for (phase = 1; phase <= 13; phase++) {
        switch(phase) {
            case 1:
                error = test_proto_header();
                break;
            case 2:
                error = test_protoSpace();
                break;
            case 3:
                error = test_protoContext();
                break;
            case 4:
                error = test_protoThread();
                break;
            case 5:
                error = test_cell();
                break;
            case 6:
                error = test_identityDict();
                break;
            case 7:
                error = test_protoSet();
                break;
            case 8:
                error = test_protoList();
                break;
            case 9:
                error = test_parentLink();
                break;
            case 10:
                error = test_byteBuffer();
                break;
            case 11:
                error = test_memoryBuffer();
                break;
            case 12:
                error = test_methodCall();
                break;
            case 13:
                error = test_protoObject();
                break;
        }
        if (error) {
            cout << "\nError running test " << phase;
            failedTests++;
        }
    }

    if (failedTests) {
        cout << "\nSome test failed (" << failedTests << " with errors)\n";
        return 1;
    }
    else {
        cout << "\nEverthing OK!\n";
        return 0;
    }
}