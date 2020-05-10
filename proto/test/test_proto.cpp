/*
 * test_proto.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */


#include "../headers/proto.h"
#include "../headers/proto_internal.h"

int failedTests = 0;


void test_proto_header() {

};

void test_proto_internal_header() {

};

void test_cell() {

};

void test_identityDict() {

};

void test_protoSet() {

};

void test_parentLink() {

};

void test_byteBuffer() {

};

void test_protoContext() {

};

void test_externalPointer() {

};

void test_protoList() {

};

void test_protoLiteral() {

};

void test_memoryBuffer() {

};

void test_methodCall() {

};

void test_protoObject() {

};

void test_protoSpace() {

};

void test_protoThread() {

};


main(int argc, char *argv) {
 
    test_proto_header();
    test_proto_internal_header();
    test_protoSpace();
    test_protoContext();
    test_protoThread();
    test_cell();
    test_identityDict();
    test_protoSet();
    test_protoList();
    test_parentLink();
    test_byteBuffer();
    test_memoryBuffer();
    test_methodCall();
    test_protoObject();

    if (failedTests)
        return 1;
    else
        return 0;
}