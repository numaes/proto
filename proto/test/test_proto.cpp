/*
 * test_proto.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"

#include <stdio.h>

int failedTests = 0;


BOOLEAN test_proto_header() {
    printf("\nTesting headers");


    return FALSE;
};

BOOLEAN test_cell() {
    printf("\nTesting Cell");


    return FALSE;
};

BOOLEAN test_identityDict() {
    printf("\nTesting IdentitySet");

    return FALSE;
};

BOOLEAN test_protoSet() {
    printf("\nTesting ProtoSet");


    return FALSE;
};

BOOLEAN test_parentLink() {
    printf("\nTesting ParentLink");


    return FALSE;
};

BOOLEAN test_byteBuffer() {
    printf("\nTesting ByteBuffer");

    return FALSE;
};

BOOLEAN test_protoContext() {
    printf("\nTesting ProtoContext");


    return FALSE;
};

BOOLEAN test_externalPointer() {
    printf("\nTesting ExternalPointer");

    return FALSE;
};

BOOLEAN test_protoList() {
    printf("\nTesting ProtoList");


    return FALSE;
};

BOOLEAN test_protoLiteral() {
    printf("\nTesting ProtoLiteral");


    return FALSE;
};

BOOLEAN test_memoryBuffer() {
    printf("\nTesting ByteBuffer");


    return FALSE;
};

BOOLEAN test_methodCall() {
    printf("\nTesting MethodCall");


    return FALSE;
};

BOOLEAN test_protoObject() {
    printf("\nTesting Proto");


    return FALSE;
};

BOOLEAN test_protoSpace() {
    printf("\nTesting ProtoSpace");

    printf("\nStep 01 - Creating and deleting");
    ProtoSpace *s = new ProtoSpace();
    s->~ProtoSpace();

    printf("\nStep 02 - Creating with context");
    s = new ProtoSpace();
    ProtoContext *c = new ProtoContext(
        s->creationContext
    );
    c->~ProtoContext();
    s->~ProtoSpace();
    
    return FALSE;
};

BOOLEAN test_protoThread() {
    printf("\nTesting Thread");


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
            printf("Error running test %d", phase);
            failedTests++;
        }
    }

    if (failedTests) {
        printf("Some test failed (%d with errors)\n", failedTests);
        return 1;
    }
    else {
        printf("Everthing OK!\n");
        return 0;
    }
}