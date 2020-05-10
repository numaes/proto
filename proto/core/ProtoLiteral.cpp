/*
 * ProtoLiteral.cpp
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"


LiteralDictionary::LiteralDictionary(
    ProtoContext *context,

    ProtoObject *key = PROTO_NONE,
    ProtoObject *hash = PROTO_NONE,
    TreeCell *previous = NULL,
    TreeCell *next = NULL,
    unsigned long count = 0,
    unsigned long height = 0,
    Cell *nextCell = NULL
) : TreeCell(context, key, hash, previous, next, CELL_TYPE_LITERAL_DICT, count, height, nextCell) {

};

LiteralDictionary::~LiteralDictionary(

) {

};



ProtoObject			*LiteralDictionary::get(ProtoObject *newName) {

};

LiteralDictionary	*LiteralDictionary::set(ProtoContext *context, ProtoObject *newName) {

};
