/*
 * test_proto.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"
#include "../headers/proto_internal.h"

#include <iostream>
#include <string>
#include <cmath> // For std::abs
#include <thread> // For std::thread, std::jthread
#include <utility> // For std::forward, std::move

// Removed using namespace std; and using namespace proto; to avoid conflicts

int failedTests = 0;


void printString(proto::ProtoContext *context, proto::ProtoString *string) {
    proto::ProtoList<proto::ProtoObject> *s = (proto::ProtoList<proto::ProtoObject> *) string;
    int size = (int) s->getSize(context);

    for (int i = 0; i < size; i++) {
        proto::ProtoObject *unicodeChar = s->getAt(context, i);
        proto::ProtoObjectPointer p;
    p.oid.oid = unicodeChar;
    if (p.op.pointer_tag == proto::POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == proto::EMBEDED_TYPE_UNICODECHAR) {
            char buffer[8];

            if (p.unicodeChar.unicodeValue <= 0x7f) {
                buffer[0] = (char) p.unicodeChar.unicodeValue;
                buffer[1] = 0;
            }
            else {
                if (p.unicodeChar.unicodeValue >= 0x80 &&
                    p.unicode.unicodeValue <= 0x7ff) {
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
            std::cout << buffer;
        }
    }
}

proto::ProtoObject *testMethod(
    proto::ProtoContext *context, 
    proto::ProtoObject *self, 
    proto::ParentLink *base, 
    proto::ProtoList<proto::ProtoObject> *args, 
    proto::ProtoSparseList<proto::ProtoObject> *kwargs) {
    return nullptr;
}

bool test_proto_header() {
    std::cout << "\nTesting headers";

    std::cout << "\nStep 01 - ProtoObjectPointer";

    if (sizeof(proto::ProtoObjectPointer) != 8) {
        std::cout << "ProtoObjectPointer size is " 
             << sizeof(proto::ProtoObjectPointer) 
             << "! Please check it\n";
        return true;
    }

    if (sizeof(proto::BigCell) != 64) {
        std::cout << "BigCell should be 64 bytes long!\n";
        return true;
    }

    if (sizeof(proto::BigCell) < sizeof(proto::ProtoThread)) {
        std::cout << "ProtoThread is bigger than BigCell" 
             << sizeof(proto::ProtoThread) 
             << "! Please check it\n";
        return true;
    }

    if (sizeof(proto::BigCell) < sizeof(proto::ProtoSparseList<proto::ProtoObject>)) {
        std::cout << "Sparse is bigger than BigCell" 
             << sizeof(proto::ProtoSparseList<proto::ProtoObject>) 
             << "! Please check it\n";
        return true;
    }

    std::cout << "\nStep 02 - Immutability of ProtoObject";
    proto::ProtoContext c;
    proto::ProtoObject *obj1 = c.fromInteger(10);
    proto::ProtoObject *obj2 = c.fromInteger(20);

    // Attempt to \"modify\" obj1 - this should create a new object
    proto::ProtoObject *obj1_modified = c.fromInteger(10);
    if (obj1 != obj1_modified) {
        std::cout << "\nError: Modifying an immutable object should return the same object";
        return true;
    }

    // Verify that the original object remains unchanged
    if (obj1->asInteger() != 10) {
        std::cout << "\nError: Original immutable object was modified";
        return true;
    }

    // Test with a string, which is also immutable
    proto::ProtoString *str1 = c.fromUTF8String("hello");
    proto::ProtoString *str2 = c.fromUTF8String("hello");
    if (str1 != str2) {
        std::cout << "\nError: Identical immutable strings should be the same object";
        return true;
    }

    proto::ProtoString *str_modified = str1->appendLast(&c, c.fromUTF8String(" world"));
    if (str1 == str_modified) {
        std::cout << "\nError: Appending to a string should create a new string";
        return true;
    }

    if (str1->getSize(&c) != 5) {
        std::cout << "\nError: Original string was modified";
        return true;
    }

    if (str_modified->getSize(&c) != 11) {
        std::cout << "\nError: Appended string has incorrect size";
        return true;
    }

    return false;
}

bool test_cell(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting Cell";


    return false;
}

bool test_sparseList(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting Sparse List";

    proto::ProtoContext c(previousContext);

    std::cout << "\nStep 01 Creating";

    proto::ProtoSparseList<proto::ProtoObject> *d = new(&c) proto::ProtoSparseListImplementation<proto::ProtoObject>(&c);

    std::cout << "\nStep 02 Adding";

    proto::ProtoString *s1 = c.fromUTF8String("First");
    proto::ProtoString *s2 = c.fromUTF8String("Second");
    proto::ProtoString *s3 = c.fromUTF8String("Third");
    proto::ProtoString *s4 = c.fromUTF8String("Fourh");

    d = d->setAt(&c, s1->getHash(&c), c.fromInteger(1));
    d = d->setAt(&c, s2->getHash(&c), c.fromInteger(2));
    d = d->setAt(&c, s3->getHash(&c), c.fromInteger(3));
    d = d->setAt(&c, s4->getHash(&c), c.fromInteger(4));

    if (d->getSize(&c) != 4) {
        std::cout << "\nError on size";
        return true;
    }

    if (d->getAt(&c, s1->getHash(&c)) != c.fromInteger(1)) {
        std::cout << "\nMismatch on adding";
        return true;
    }

    std::cout << "\nStep 03 Replace";

    d = d->setAt(&c, s2->getHash(&c), c.fromInteger(1));

    if (d->getSize(&c) != 4 ||
        d->getAt(&c, s2->getHash(&c)) != c.fromInteger(1)) {
        std::cout << "\nReplace is not working";
        return true;
    }

    std::cout << "\nStep 04 Remove";

    d = d->removeAt(&c, s2->getHash(&c));

    if (d->getSize(&c) != 3 ||
        !d->has(&c, s1->getHash(&c)) ||
        !d->has(&c, s3->getHash(&c)) ||
        !d->has(&c, s4->getHash(&c)) ||
        d->has(&c, s2->getHash(&c))) {
        std::cout << "\nRemove is not working";
        return true;
    }

    std::cout << "\nStep 05 isEqual";

    proto::ProtoSparseList<proto::ProtoObject> *d2 = new(&c) proto::ProtoSparseListImplementation<proto::ProtoObject>(&c);

    d2 = d2->setAt(&c, s1->getHash(&c), c.fromInteger(1));
    d2 = d2->setAt(&c, s3->getHash(&c), c.fromInteger(3));

    if (d->isEqual(&c, d2)) {
        std::cout << "\nisEqual with diferent dicts is not working";
        return true;
    }

    d2 = d2->setAt(&c, s4->getHash(&c), c.fromInteger(4));

    if (!d->isEqual(&c, d2)) {
        std::cout << "\nisEqual with equal dicts is not working";
        return true;
    }

    return false;
}

bool test_parentLink(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ParentLink";


    return false;
}

bool test_byteBuffer(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ByteBuffer";

    return false;
}

int countBlocks(proto::Cell *cell) {
    int count = 0;
    while (cell) {
        count++;
        cell = cell->nextCell;
    }
    return count;
}

bool test_protoContext(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ProtoContext";

    std::cout << "\nStep 01 - ProtoContext basic";

    proto::ProtoContext *c = new proto::ProtoContext(previousContext);

    proto::ProtoObjectPointer p;
    proto::ProtoObject *o, *o2, *o3;
    
    std::cout << "\nStep 02 - Embedded types - Integer";
    o = c->fromInteger(37);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE || 
        p.op.embedded_type != proto::EMBEDED_TYPE_SMALLINT ||
        p.si.smallInteger != 37) {
        std::cout << "\nError on Integer representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - Doubles";
    o = c->fromDouble(45.78898387777);

    p.oid.oid = o;
    union {
        unsigned long lv;
        double dv;
    } u;
    u.lv = p.sd.smallDouble << proto::TYPE_SHIFT;

    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_SMALLDOUBLE ||
        std::abs(u.dv - 45.78898387777) > 0.0000000001) {
        std::cout << "\nError on Double representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - Boolean";
    o = c->fromBoolean(false);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_BOOLEAN ||  
        p.booleanValue.booleanValue != false) {
        std::cout << "\nError on Boolean representation";
        return true;
    }

    o = c->fromBoolean(true);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_BOOLEAN ||  
        p.booleanValue.booleanValue != true) {
        std::cout << "\nError on Boolean representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - BYTE";
    o = c->fromByte(38);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_BYTE ||  
        p.byteValue.byteData != 38) {
        std::cout << "\nError on Byte representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - Date";
    o = c->fromDate(2020, 05, 01);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_DATE ||  
        p.date.year != 2020 ||
        p.date.month != 05 ||
        p.date.day != 01) {
        std::cout << "\nError on Date representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - Timestamp";
    o = c->fromTimestamp(2993948);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_TIMESTAMP ||  
        p.timestampValue.timestamp != 2993948) {
        std::cout << "\nError on Timestamp representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - UTF8Char";
    o = c->fromUTF8Char((char *) "Ñ");

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_UNICODECHAR ||  
        p.unicodeChar.unicodeValue != 337) {
        std::cout << "\nError on UTF8 char representation";
        return true;
    }

    std::cout << "\nStep 03 - Embedded types - Timedelta";
    o = c->fromTimeDelta(3834848);

    p.oid.oid = o;
    if (p.op.pointer_tag != proto::POINTER_TAG_EMBEDEDVALUE ||
        p.op.embedded_type != proto::EMBEDED_TYPE_TIMEDELTA ||  
        p.timedeltaValue.timedelta != 3834848) {
        std::cout << "\nError on Timedelta representation";
        return true;
    }

    std::cout << "\nStep 04 - Cell types - External Pointers";
    proto::ProtoExternalPointer *ep = c.fromExternalPointer(&o);

    p.oid.oid = ep->asObject(&c);
    if (p.op.pointer_tag != proto::POINTER_TAG_EXTERNAL_POINTER ||
        (ep->getPointer(&c) != &o)) {
        std::cout << "\nError on Pointer cell";
        return true;
    }

    std::cout << "\nStep 04 - Cell types - Byte buffer";
    proto::ProtoByteBuffer *bb = c.newBuffer(10);

    p.oid.oid = bb->asObject(&c);
    if (p.op.pointer_tag != proto::POINTER_TAG_BYTE_BUFFER ||
        bb->getSize(&c) != 10 ||
        bb->getBuffer(&c) == NULL) {
        std::cout << "\nError on Byte buffer";
        return true;
    }

    // ((proto::ProtoByteBuffer *) p.cell.cell)->~ProtoByteBuffer();

    std::cout << "\nStep 04 - Cell types - Method call";
    proto::ProtoObject *self = c.fromByte(22);
    proto::ProtoMethodCell *mc = c.fromMethod(self, &testMethod);

    p.oid.oid = mc->asObject(&c);
    if (p.op.pointer_tag != proto::POINTER_TAG_METHOD_CELL ||
        mc->getSelf(&c) != self ||
        mc->getMethod(&c) != testMethod) {
        std::cout << "\nError on Method";
        return true;
    }

    std::cout << "\nStep 04 - Cell types - UTF8String";
    proto::ProtoString *sp = c.fromUTF8String((char *) "Ñoño");

    p.oid.oid = sp->asObject(&c);
    if (p.op.pointer_tag != proto::POINTER_TAG_STRING ||
        sp->getSize(&c) != 4) {
        std::cout << "\nError on UTF8 String";
        return true;
    }

    if (sp->getAt(&c, 0) != c.fromUTF8Char((char *) "Ñ") ||
        sp->getAt(&c, 1) != c.fromUTF8Char((char *) "o") ||
        sp->getAt(&c, 2) != c.fromUTF8Char((char *) "ñ") ||
        sp->getAt(&c, 3) != c.fromUTF8Char((char *) "o")) {
        std::cout << "\nError on UTF8 String content";
        return true;
    }

    std::cout << "\nStep 05 - Proto.h as* and is* methods";

    if (c.fromBoolean(true)->asBoolean() != true ||
        c.fromBoolean(false)->asBoolean() != false ||
        c.fromBoolean(false)->isBoolean() != true) {
        std::cout << "\nError on asBoolean or isBoolean";
        return true;
    }

    if (c.fromInteger(64744)->asInteger() != 64744 ||
        c.fromBoolean(false)->asInteger() != 0 ||
        c.fromInteger(3999)->isInteger() != true) {
        std::cout << "\nError on asInteger or isInteger";
        return true;
    }

    if (c.fromDouble(1.0)->asDouble() != 1.0 ||
        c.fromBoolean(false)->asDouble() != 0.0 ||
        c.fromDouble(3.4848548484)->isDouble() != true) {
        std::cout << "\nError on asDouble or isDouble";
        return true;
    }

    if (c.fromByte(45)->asByte() != 45 ||
        c.fromBoolean(false)->asByte() != 0 ||
        c.fromByte(38)->isByte() != true) {
        std::cout << "\nError on asByte or isByte";
        return true;
    }

//    std::cout << "\nStep 06 - Context go and return";


//    int currentCount = countBlocks(c.thread->currentWorkingSet);

//    proto::ProtoContext *newContext = new proto::ProtoContext(c);

//    newContext->~ProtoContext();

//    if (currentCount != countBlocks(c.thread->currentWorkingSet)) {

//        std::cout << "\nUnbalanced count on subcontext";

//        return true;

//    }

//    newContext = new proto::ProtoContext(c);

//    new(newContext) proto::ProtoSet(newContext);

//    if (countBlocks(c.thread->currentWorkingSet) != (currentCount + 1)) {

//        std::cout << "\nWorkingSet incorrect";

//        return true;

//    }

//    newContext->setReturnValue(new(newContext) proto::ProtoSet(newContext));

//    newContext->~ProtoContext();

//    if ((currentCount + 1) != countBlocks(c.thread->currentWorkingSet)) {

//        std::cout << "\nUnbalanced count on subcontext after return";

//        return true;

//    }

//    newContext->~ProtoContext();

//    newContext = NULL;

    return false;
}

bool test_externalPointer(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ExternalPointer";

    return false;
}

bool test_byteBuffer(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ByteBuffer";


    return false;
}

int countBlocks(proto::Cell *cell) {
    int count = 0;
    while (cell) {
        count++;
        cell = cell->nextCell;
    }
    return count;
}

bool test_protoList(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ProtoList";

    proto::ProtoContext c(previousContext);

    std::cout << "\nStep 01 Creating";

    int i, j;

    proto::ProtoList<proto::ProtoObject> *l = new(&c) proto::ProtoListImplementation<proto::ProtoObject>(&c);

    if (l->getSize(&c) != 0) {
        std::cout << "\nWrong size on creation";
        return true;
    }    

    std::cout << "\nStep 02 Appends";

    for (i = 0; i < 100; i++)
        l = l->appendLast(&c, c.fromInteger(i));

    for (i = 0; i < 100; i++) {
        if (l->getAt(&c, i) != c.fromInteger(i)) {
            std::cout << "\nSequence error on appendLast";
            return true;
        }
    }

    for (i = -1; i >= -100; i--) {
        l = l->appendFirst(&c, c.fromInteger(i));
    }

    i = 0;

    j = -100;

    while (i < 200) {
        if (l->getAt(&c, i) != c.fromInteger(j)) {
            std::cout << "\nSequence error on appendFirst";
            return true;
        }
        i++;
        j++;
    }

    if (l->getAt(&c, -1) != c.fromInteger(99)) {
        std::cout << "\nGet (negative index) failed";
        return true;
    }

    std::cout << "\nStep 03 Remove";

    l = l->removeFirst(&c);
    if (l->getAt(&c, 0) != c.fromInteger(-99)) {
        std::cout << "\nSomething wrong in removeFirst";
        return true;
    }

    l = l->removeLast(&c);
    if (l->getAt(&c, 197) != c.fromInteger(98)) {
        std::cout << "\nSomething wrong in removeLast";
        return true;
    }

    l = l->removeAt(&c, 4);
    if (l->getAt(&c, 3) != c.fromInteger(-96) && 
        l->getAt(&c, 4) != c.fromInteger(-94)) {
        std::cout << "\nSomething wrong in removeAt";
        return true;
    }

    std::cout << "\nStep 04 Set";

    l = l->setAt(&c, 4, c.fromInteger(201));
    if (l->getAt(&c, 4) != c.fromInteger(201)) {
        std::cout << "\nSomething wrong in setAt";
        return true;
    }

    std::cout << "\nStep 05 Has";

    if (!l->has(&c, c.fromInteger(201))) {
        std::cout << "\nSomething went wrong in has";
        return true;
    }

    std::cout << "\nStep 06 InsertAt";

    l = l->insertAt(&c, 4, c.fromInteger(202));
    if (l->getAt(&c, 3) != c.fromInteger(-97) && 
        l->getAt(&c, 4) != c.fromInteger(202) &&
        l->getAt(&c, 5) != c.fromInteger(201)) {
        std::cout << "\nSomething wrong in insertAt";
        return true;
    }

    std::cout << "\nStep 07 Get";

    if (l->getFirst(&c) != c.fromInteger(-99)) {
        std::cout << "\nSomething wrong in getFirst";
        return true;
    }

    if (l->getLast(&c) != c.fromInteger(98)) {
        std::cout << "\nSomething wrong in getLast";
        return true;
    }

    proto::ProtoList<proto::ProtoObject> *l_new = new(&c) proto::ProtoListImplementation<proto::ProtoObject>(&c);

    for (i = 0; i < 100; i++)
        l_new = l_new->appendLast(&c, c.fromInteger(i));

    std::cout << "\nStep 08 getSlice";

    proto::ProtoList<proto::ProtoObject> *l2 = l_new->getSlice(&c, 30, 40);
    for (i = 30; i < 40; i++) {
        if (l2->getAt(&c, i-30) != c.fromInteger(i)) {
            std::cout << "\nSomething wrong in getSlice";
            return true;
        }
    }

    return false;
}

bool test_protoLiteral(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ProtoLiteral";


    return false;
}

bool test_memoryBuffer(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting ByteBuffer";


    return false;
}

bool test_methodCall(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting MethodCall";


    return false;
}

bool test_protoObject(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting Proto";


    return false;
}


void test_protomethod(proto::ProtoContext *previousContext, int par1) {
    unsigned int localsCount = 4;
    struct {
        proto::ProtoObject * p1;
        proto::ProtoObject * p2;
        proto::ProtoObject * p3;
        proto::ProtoObject * p4;
    } locals;
    proto::ProtoContext localContext(previousContext, &locals.p1, sizeof(locals) / sizeof(proto::ProtoObject *));
}

bool test_protoSpace() {
    std::cout << "\n\nTesting ProtoSpace";

    std::cout << "\nStep 01 - Creating and deleting";
    proto::ProtoSpace *s = new proto::ProtoSpace(nullptr, 0, nullptr);
    delete s; // Clean up allocated space

    std::cout << "\nStep 02 - Creating with context";
    s = new proto::ProtoSpace(nullptr, 0, nullptr);
    proto::ProtoContext c;

    test_protomethod(&c, 0); // Call with an argument

    delete s; // Clean up allocated space

    return false;
}

bool test_protoThread(proto::ProtoContext *previousContext) {
    std::cout << "\n\nTesting Thread";


    return false;
}


int main(int argc, char **argv) {

    int phase;
    int error;

    proto::ProtoSpace *s = new proto::ProtoSpace(nullptr, 0, nullptr);
    proto::ProtoContext c(nullptr, nullptr, 0, nullptr, s);

    for (phase = 1; phase <= 13; phase++) {
        switch(phase) {
            case 1:
                error = test_proto_header();
                break;
            case 2:
                error = test_protoSpace();
                break;
            case 3:
                error = test_protoContext(&c);
                break;
            case 4:
                error = test_protoThread(&c);
                break;
            case 5:
                error = test_cell(&c);
                break;
            case 6:
                error = test_sparseList(&c);
                break;
            case 7:
                error = test_protoList(&c);
                break;
            case 8:
                error = test_parentLink(&c);
                break;
            case 9:
                error = test_byteBuffer(&c);
                break;
            case 10:
                error = test_memoryBuffer(&c);
                break;
            case 11:
                error = test_methodCall(&c);
                break;
            case 12:
                error = test_protoObject(&c);
                break;
        }
        if (error) {
            std::cout << "\nError running test " << phase;
            failedTests++;
        }
    }

    if (failedTests) {
        std::cout << "\nSome test failed (" << failedTests << " with errors)\n";
        return 1;
    }
    else {
        std::cout << "\nEverthing OK!\n";
        return 0;
    }
}
