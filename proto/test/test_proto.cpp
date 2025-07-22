/*
 * test_proto.cpp
 *
 *  Created on: 2024
 *      Author: Gemini Code Assist
 *
 *  Comprehensive test suite for the Proto runtime.
 */

#include <cstdio>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include "proto.h"
#include "proto_internal.h" // Necesario para acceder a las clases de implementación

// --- Macro de Aserción Simple ---
// Una macro simple para hacer las pruebas más legibles y detener la ejecución en caso de fallo.
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(">> FAILED: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            assert(condition); \
        } else { \
            printf("   OK: %s\n", message); \
        } \
    } while (false)

// --- Funciones de Prueba por Característica ---

void test_primitives(proto::ProtoContext& c) {
    printf("\n--- Probando Tipos Primitivos ---\n");

    proto::ProtoObject* i1 = c.fromInteger(123);
    proto::ProtoObject* i2 = c.fromInteger(-456);
    proto::ProtoObject* b_true = c.fromBoolean(true);
    proto::ProtoObject* b_false = c.fromBoolean(false);
    proto::ProtoObject* byte = c.fromByte('A');

    ASSERT(i1->isInteger(), "isInteger para entero positivo");
    ASSERT(i1->asInteger() == 123, "asInteger para entero positivo");

    ASSERT(i2->isInteger(), "isInteger para entero negativo");
    ASSERT(i2->asInteger() == -456, "asInteger para entero negativo");

    ASSERT(b_true->isBoolean(), "isBoolean para true");
    ASSERT(b_true->asBoolean() == true, "asBoolean para true");

    ASSERT(b_false->isBoolean(), "isBoolean para false");
    ASSERT(b_false->asBoolean() == false, "asBoolean para false");

    ASSERT(i1->asBoolean() == true, "Conversión de entero a booleano (true)");
    ASSERT(c.fromInteger(0)->asBoolean() == false, "Conversión de entero a booleano (false)");

    ASSERT(byte->isByte(), "isByte para char");
    ASSERT(byte->asByte() == 'A', "asByte para char");
}

void test_list(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoList ---\n");

    proto::ProtoList* list = c.newList();
    ASSERT(list->getSize(&c) == 0, "Una lista nueva debe estar vacía");

    list = list->appendLast(&c, c.fromInteger(10));
    list = list->appendLast(&c, c.fromInteger(20));
    list = list->appendLast(&c, c.fromInteger(30));

    ASSERT(list->getSize(&c) == 3, "Tamaño de la lista después de 3 inserciones");
    ASSERT(list->getAt(&c, 0)->asInteger() == 10, "getAt(0)");
    ASSERT(list->getAt(&c, 1)->asInteger() == 20, "getAt(1)");
    ASSERT(list->getAt(&c, 2)->asInteger() == 30, "getAt(2)");
    ASSERT(list->getAt(&c, -1)->asInteger() == 30, "getAt(-1) para el último elemento");

    ASSERT(list->has(&c, c.fromInteger(20)), "has() para un elemento existente");
    ASSERT(!list->has(&c, c.fromInteger(99)), "has() para un elemento no existente");

    proto::ProtoList* list2 = list->setAt(&c, 1, c.fromInteger(25));
    ASSERT(list->getAt(&c, 1)->asInteger() == 20, "La lista original es inmutable después de setAt");
    ASSERT(list2->getAt(&c, 1)->asInteger() == 25, "La nueva lista tiene el valor actualizado después de setAt");
    ASSERT(list2->getSize(&c) == 3, "El tamaño se preserva después de setAt");

    proto::ProtoList* list3 = list2->insertAt(&c, 0, c.fromInteger(5));
    ASSERT(list3->getSize(&c) == 4, "Tamaño después de insertAt al principio");
    ASSERT(list3->getAt(&c, 0)->asInteger() == 5, "Valor después de insertAt al principio");
    ASSERT(list3->getAt(&c, 1)->asInteger() == 10, "Valor desplazado después de insertAt");

    proto::ProtoList* list4 = list3->removeAt(&c, 2);
    ASSERT(list4->getSize(&c) == 3, "Tamaño después de removeAt");
    ASSERT(list4->getAt(&c, 1)->asInteger() == 10, "Elemento antes del eliminado");
    ASSERT(list4->getAt(&c, 2)->asInteger() == 30, "Elemento después del eliminado");

    proto::ProtoList* slice = list->getSlice(&c, 1, 3);
    ASSERT(slice->getSize(&c) == 2, "Tamaño del slice");
    ASSERT(slice->getAt(&c, 0)->asInteger() == 20, "Elemento 0 del slice");
    ASSERT(slice->getAt(&c, 1)->asInteger() == 30, "Elemento 1 del slice");

    // Probar el iterador
    proto::ProtoListIterator* iter = list->getIterator(&c);
    ASSERT(iter->hasNext(&c), "El iterador tiene hasNext en una lista no vacía");
    ASSERT(iter->next(&c)->asInteger() == 10, "El iterador next() devuelve el primer elemento");
    iter = iter->advance(&c);
    ASSERT(iter->next(&c)->asInteger() == 20, "El iterador next() devuelve el segundo elemento");
    iter = iter->advance(&c);
    ASSERT(iter->next(&c)->asInteger() == 30, "El iterador next() devuelve el tercer elemento");
    iter = iter->advance(&c);
    ASSERT(!iter->hasNext(&c), "El iterador hasNext al final es falso");
}

void test_tuple(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoTuple ---\n");

    proto::ProtoList* list1 = c.newList();
    list1 = list1->appendLast(&c, c.fromInteger(1));
    list1 = list1->appendLast(&c, c.fromInteger(2));

    proto::ProtoList* list2 = c.newList();
    list2 = list2->appendLast(&c, c.fromInteger(1));
    list2 = list2->appendLast(&c, c.fromInteger(2));

    proto::ProtoTuple* tuple1 = c.tupleFromList(list1);
    proto::ProtoTuple* tuple2 = c.tupleFromList(list2);

    ASSERT(tuple1->getSize(&c) == 2, "Tamaño de la tupla");
    ASSERT(tuple1->getAt(&c, 1)->asInteger() == 2, "getAt de la tupla");

    // Característica clave: internalización de tuplas
    ASSERT(tuple1 == tuple2, "Internalización: tuplas idénticas deben ser el mismo objeto");

    proto::ProtoList* list3 = c.newList();
    list3 = list3->appendLast(&c, c.fromInteger(1));
    list3 = list3->appendLast(&c, c.fromInteger(3));
    proto::ProtoTuple* tuple3 = c.tupleFromList(list3);

    ASSERT(tuple1 != tuple3, "Internalización: tuplas diferentes deben ser objetos diferentes");
}

void test_string(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoString ---\n");

    proto::ProtoString* s1 = c.fromUTF8String("hola");
    proto::ProtoString* s2 = c.fromUTF8String(" mundo");

    ASSERT(s1->getSize(&c) == 4, "Tamaño del string");
    ASSERT(s1->getAt(&c, 1)->asInteger() == 'o', "getAt del string (un char es un entero)");

    proto::ProtoString* s3 = s1->appendLast(&c, s2);
    ASSERT(s3->getSize(&c) == 10, "Tamaño del string concatenado");

    // Para verificar el contenido, podemos convertir a lista y comprobar los elementos
    proto::ProtoList* s3_list = s3->asList(&c);
    ASSERT(s3_list->getAt(&c, 4)->asInteger() == ' ', "Verificación del contenido del string concatenado");
    ASSERT(s3_list->getAt(&c, 9)->asInteger() == 'o', "Verificación del contenido al final");

    proto::ProtoString* slice = s3->getSlice(&c, 5, 10);
    ASSERT(slice->getSize(&c) == 5, "Tamaño del slice de string");
    ASSERT(slice->getAt(&c, 0)->asInteger() == 'm', "Contenido del slice de string");
}

void test_sparse_list(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoSparseList (Diccionario) ---\n");

    proto::ProtoSparseList* dict = c.newSparseList();
    ASSERT(dict->getSize(&c) == 0, "Una lista dispersa nueva está vacía");

    proto::ProtoString* key1 = c.fromUTF8String("nombre");
    proto::ProtoString* key2 = c.fromUTF8String("edad");
    unsigned long key1_hash = key1->getHash(&c);
    unsigned long key2_hash = key2->getHash(&c);

    dict = dict->setAt(&c, key1_hash, c.fromUTF8String("proto"));
    dict = dict->setAt(&c, key2_hash, c.fromInteger(7));

    ASSERT(dict->getSize(&c) == 2, "Tamaño del diccionario después de dos inserciones");
    ASSERT(dict->has(&c, key1_hash), "has() del diccionario para una clave existente");
    ASSERT(!dict->has(&c, c.fromUTF8String("x")->getHash(&c)), "has() del diccionario para una clave no existente");

    proto::ProtoString* name_val = static_cast<proto::ProtoString*>(dict->getAt(&c, key1_hash));
    ASSERT(name_val->cmp_to_string(&c, c.fromUTF8String("proto")) == 0, "getAt() del diccionario para un valor string");
    ASSERT(dict->getAt(&c, key2_hash)->asInteger() == 7, "getAt() del diccionario para un valor entero");

    proto::ProtoSparseList* dict2 = dict->removeAt(&c, key1_hash);
    ASSERT(dict->has(&c, key1_hash), "El diccionario original es inmutable después de removeAt");
    ASSERT(dict2->getSize(&c) == 1, "El nuevo diccionario tiene el tamaño correcto después de removeAt");
    ASSERT(!dict2->has(&c, key1_hash), "La clave se elimina en el nuevo diccionario");
    ASSERT(dict2->has(&c, key2_hash), "La otra clave se preserva en el nuevo diccionario");
}

void test_objects_and_prototypes(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoObjectCell (Prototipos) ---\n");

    // Crear un prototipo base
    proto::ProtoObjectCell* proto = new(&c) proto::ProtoObjectCellImplementation(&c);
    proto::ProtoString* attr_name = c.fromUTF8String("version");
    proto = static_cast<proto::ProtoObjectCell*>(proto->setAttribute(&c, attr_name, c.fromInteger(1)));

    ASSERT(proto->hasOwnAttribute(&c, attr_name)->asBoolean(), "El prototipo tiene su propio atributo 'version'");
    ASSERT(proto->getAttribute(&c, attr_name)->asInteger() == 1, "El valor del atributo 'version' es correcto");

    // Crear un hijo que hereda del prototipo
    proto::ProtoObjectCell* child = static_cast<proto::ProtoObjectCell*>(proto->newChild(&c));
    ASSERT(child->getParents(&c)->getSize(&c) == 1, "El hijo tiene un padre");
    ASSERT(!child->hasOwnAttribute(&c, attr_name)->asBoolean(), "El hijo NO tiene su propio atributo 'version'");
    ASSERT(child->hasAttribute(&c, attr_name)->asBoolean(), "El hijo SÍ tiene acceso al atributo 'version' (heredado)");
    ASSERT(child->getAttribute(&c, attr_name)->asInteger() == 1, "El hijo hereda el valor correcto de 'version'");

    // Añadir un atributo al hijo (shadowing)
    proto::ProtoObjectCell* child2 = static_cast<proto::ProtoObjectCell*>(child->setAttribute(&c, attr_name, c.fromInteger(2)));
    ASSERT(child->getAttribute(&c, attr_name)->asInteger() == 1, "El objeto 'child' original no se modifica");
    ASSERT(child2->hasOwnAttribute(&c, attr_name)->asBoolean(), "El nuevo hijo SÍ tiene su propio atributo 'version'");
    ASSERT(child2->getAttribute(&c, attr_name)->asInteger() == 2, "El nuevo hijo devuelve su propio valor para 'version'");

    // Añadir un segundo padre
    proto::ProtoObjectCell* proto2 = new(&c) proto::ProtoObjectCellImplementation(&c);
    proto::ProtoString* attr_name2 = c.fromUTF8String("author");
    proto2 = static_cast<proto::ProtoObjectCell*>(proto2->setAttribute(&c, attr_name2, c.fromUTF8String("gamarino")));

    proto::ProtoObjectCell* child3 = static_cast<proto::ProtoObjectCell*>(child2->addParent(&c, proto2));
    ASSERT(child3->getParents(&c)->getSize(&c) == 2, "El hijo ahora tiene dos padres");
    ASSERT(child3->getAttribute(&c, attr_name2)->isCell(&c), "El hijo puede acceder al atributo del segundo padre");
}

void test_gc_stress(proto::ProtoContext& c) {
    printf("\n--- Probando Recolector de Basura (GC Stress Test) ---\n");

    // Crear una lista raíz para mantener algunos objetos vivos
    proto::ProtoList* root_list = c.newList();
    const int iterations = 2000; // Suficiente para forzar al GC a ejecutarse varias veces

    for (int i = 0; i < iterations; ++i) {
        // Crear una lista temporal que se volverá basura
        proto::ProtoList* tempList = c.newList();
        tempList = tempList->appendLast(&c, c.fromInteger(i));
        tempList = tempList->appendLast(&c, c.fromUTF8String("temp string"));

        // Cada 100 iteraciones, guardar una lista en la raíz para que no sea recolectada
        if (i % 100 == 0) {
            root_list = root_list->appendLast(&c, tempList);
        }
    }

    ASSERT(root_list->getSize(&c) == iterations / 100, "La lista raíz tiene el número correcto de objetos persistentes");

    printf("   Forzando ejecución del GC...\n");
    c.space->triggerGC();
    // Darle tiempo al hilo del GC para que se ejecute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // La prueba pasa si no hay un crash. Verificamos que los objetos raíz sigan siendo válidos.
    proto::ProtoList* first_kept = static_cast<proto::ProtoList*>(root_list->getAt(&c, 0));
    ASSERT(first_kept->getAt(&c, 0)->asInteger() == 0, "El primer objeto persistente sigue siendo válido después del GC");

    proto::ProtoList* last_kept = static_cast<proto::ProtoList*>(root_list->getAt(&c, (iterations / 100) - 1));
    ASSERT(last_kept->getAt(&c, 0)->asInteger() == iterations - 100, "El último objeto persistente sigue siendo válido después del GC");

    printf("   GC Stress Test completado sin fallos.\n");
}


// --- Función Principal de Pruebas ---

proto::ProtoObject* main_test_function(
    proto::ProtoContext *c,
    proto::ProtoObject *self,
    proto::ParentLink *parentLink,
    proto::ProtoList *args,
    proto::ProtoSparseList *kwargs
) {
    printf("=======================================\n");
    printf("      INICIANDO SUITE DE PRUEBAS\n");
    printf("=======================================\n");

    test_primitives(*c);
    test_list(*c);
    test_tuple(*c);
    test_string(*c);
    test_sparse_list(*c);
    test_objects_and_prototypes(*c);
    test_gc_stress(*c);

    printf("\n=======================================\n");
    printf("      TODAS LAS PRUEBAS PASARON\n");
    printf("=======================================\n");

    return PROTO_NONE;
}

// --- Punto de Entrada ---

int main(int argc, char **argv) {
    // El constructor de ProtoSpace inicia el hilo principal con main_test_function
    // y bloquea hasta que termine.
    proto::ProtoSpace space(main_test_function, argc, argv);
    return 0;
}

