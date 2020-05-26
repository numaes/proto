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

BOOLEAN test_proto_header() {
    cout << "\nTesting headers";


    return FALSE;
};

BOOLEAN test_cell() {
    cout << "\nTesting Cell";


    return FALSE;
};

BOOLEAN test_identityDict() {
    cout << "\nTesting IdentitySet";

    return FALSE;
};

BOOLEAN test_protoSet() {
    cout << "\nTesting ProtoSet";


    return FALSE;
};

BOOLEAN test_parentLink() {
    cout << "\nTesting ParentLink";


    return FALSE;
};

BOOLEAN test_byteBuffer() {
    cout << "\nTesting ByteBuffer";

    return FALSE;
};

BOOLEAN test_protoContext() {
    cout << "\nTesting ProtoContext";

    cout << "\nStep 01 - ProtoContext basic";

    ProtoSpace *s = new ProtoSpace();
    s->~ProtoSpace();

    ProtoContext *c = new ProtoContext(
        s->creationContext
    );

    cout << "\nStep 02 - UTF8 String";

    cout << "\nEsta es una prueba --- ";
    printString(c, c->fromUTF8String((char*) "Esta es una prueba"));

    cout << "\nCoño, äáíñ --- ";
    printString(c, c->fromUTF8String((char *)"Coño, äáíñ"));

    return FALSE;
};

BOOLEAN test_externalPointer() {
    cout << "\nTesting ExternalPointer";

    return FALSE;
};

BOOLEAN test_protoList() {
    cout << "\nTesting ProtoList";


    return FALSE;
};

BOOLEAN test_protoLiteral() {
    cout << "\nTesting ProtoLiteral";


    return FALSE;
};

BOOLEAN test_memoryBuffer() {
    cout << "\nTesting ByteBuffer";


    return FALSE;
};

BOOLEAN test_methodCall() {
    cout << "\nTesting MethodCall";


    return FALSE;
};

BOOLEAN test_protoObject() {
    cout << "\nTesting Proto";


    return FALSE;
};

BOOLEAN test_protoSpace() {
    cout << "\nTesting ProtoSpace";

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
    cout << "\nTesting Thread";


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