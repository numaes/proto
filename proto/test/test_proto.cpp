/*
 * test_proto.cpp
 *
 *  Created on: 2 de ago. de 2017
 *      Author: gamarino
 */
#include <iostream>
#include "./headers/proto.h"
#include "./headers/proto_internal.h"

// Todo el código a continuación está dentro del 'namespace proto'
// que se abre en 'proto.h'.

// Declaraciones anticipadas de las funciones de prueba
void test_primitives(proto::ProtoContext& c);
void test_string(proto::ProtoContext& c);
void test_tuple(proto::ProtoContext& c);
void test_list(proto::ProtoContext& c);
void test_sparse_list(proto::ProtoContext& c);
void test_objects_and_prototypes(proto::ProtoContext& c);
void test_method_call(proto::ProtoContext& c);
void test_gc_stress(proto::ProtoContext& c);

// Un método nativo para propósitos de prueba
proto::ProtoObject* native_method_for_test(
        proto::ProtoContext& c,
        proto::ProtoObject* self,
        proto::ProtoObject* slot,
        proto::ProtoList* args
) {
    std::cout << "llamada a método nativo" << std::endl;
    return PROTO_NONE;
}

int main(int argc, char **argv) {
    std::cout << "Suite de pruebas de Proto" << std::endl;

    proto::ProtoContext c;

    // Ejecutar todas las pruebas
    test_primitives(c);
    test_string(c);
    test_tuple(c);
    test_list(c);
    test_sparse_list(c);
    test_objects_and_prototypes(c);
    test_method_call(c);
    test_gc_stress(c);

    std::cout << "Todas las pruebas completadas" << std::endl;
    return 0;
}

// --- Implementaciones de las Pruebas ---

void test_primitives(proto::ProtoContext& c) {
    // Implementación para pruebas de primitivas
}

void test_string(proto::ProtoContext& c) {
    // Implementación para pruebas de strings
}

void test_tuple(proto::ProtoContext& c) {
    // Implementación para pruebas de tuplas
}

void test_list(proto::ProtoContext& c) {
    // Implementación para pruebas de listas
}

void test_sparse_list(proto::ProtoContext& c) {
    // Implementación para pruebas de listas dispersas
}

void test_objects_and_prototypes(proto::ProtoContext& c) {
    // Implementación para pruebas de objetos y prototipos
}

void test_method_call(proto::ProtoContext& c) {
    // Implementación para pruebas de llamadas a métodos
}

void test_gc_stress(proto::ProtoContext& c) {
    // Implementación para pruebas de estrés del GC
}

