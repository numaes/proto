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
void test_primitives(ProtoContext& c);
void test_string(ProtoContext& c);
void test_tuple(ProtoContext& c);
void test_list(ProtoContext& c);
void test_sparse_list(ProtoContext& c);
void test_objects_and_prototypes(ProtoContext& c);
void test_method_call(ProtoContext& c);
void test_gc_stress(ProtoContext& c);

// Un método nativo para propósitos de prueba
ProtoObject* native_method_for_test(
        ProtoContext& c,
        ProtoObject* self,
        ProtoObject* slot,
        ProtoList* args
) {
    std::cout << "llamada a método nativo" << std::endl;
    return PROTO_NONE;
}

int main(int argc, char **argv) {
    std::cout << "Suite de pruebas de Proto" << std::endl;

    ProtoContext c;

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

void test_primitives(ProtoContext& c) {
    // Implementación para pruebas de primitivas
}

void test_string(ProtoContext& c) {
    // Implementación para pruebas de strings
}

void test_tuple(ProtoContext& c) {
    // Implementación para pruebas de tuplas
}

void test_list(ProtoContext& c) {
    // Implementación para pruebas de listas
}

void test_sparse_list(ProtoContext& c) {
    // Implementación para pruebas de listas dispersas
}

void test_objects_and_prototypes(ProtoContext& c) {
    // Implementación para pruebas de objetos y prototipos
}

void test_method_call(ProtoContext& c) {
    // Implementación para pruebas de llamadas a métodos
}

void test_gc_stress(ProtoContext& c) {
    // Implementación para pruebas de estrés del GC
}

} // Cierra el 'namespace proto' abierto en 'proto.h'