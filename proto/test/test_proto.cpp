/*
 * test_proto.cpp
 *
 *  Suite de pruebas completa para el runtime de Proto.
 *  Estructurada para ejecutarse dentro del ciclo de vida de ProtoSpace.
 */
#include <cstdio>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include "headers/proto.h"
#include "headers/proto_internal.h"

// --- Macro de Aserción Simple ---
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(">> FAILED: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            assert(condition); \
        } else { \
            printf("   OK: %s\n", message); \
        } \
    } while (false)

// --- Declaraciones de las Funciones de Prueba ---
void test_primitives(proto::ProtoContext& c);
void test_list(proto::ProtoContext& c);
void test_tuple(proto::ProtoContext& c);
void test_string(proto::ProtoContext& c);
void test_sparse_list(proto::ProtoContext& c);
void test_objects_and_prototypes(proto::ProtoContext& c);
void test_gc_stress(proto::ProtoContext& c);


// --- Función Principal de Pruebas (que se ejecutará dentro de ProtoSpace) ---

proto::ProtoObject* main_test_function(
    proto::ProtoContext* c,
    proto::ProtoObject* self,
    proto::ParentLink* parentLink,
    proto::ProtoList* args,
    proto::ProtoSparseList* kwargs
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

// --- Punto de Entrada del Ejecutable ---
// Su única responsabilidad es iniciar el ProtoSpace con nuestra función de prueba.
int main(int argc, char **argv) {
    proto::ProtoSpace space(main_test_function, argc, argv);
    return 0;
}


// --- Implementaciones de las Pruebas ---

void test_primitives(proto::ProtoContext& c) {
    printf("\n--- Probando Tipos Primitivos ---\n");

    proto::ProtoObject* i1 = c.fromInteger(123);
    proto::ProtoObject* i2 = c.fromInteger(-456);
    proto::ProtoObject* b_true = c.fromBoolean(true);
    proto::ProtoObject* b_false = c.fromBoolean(false);
    proto::ProtoObject* byte = c.fromByte('A');

    ASSERT(i1->isInteger(&c), "isInteger para entero positivo");
    ASSERT(i1->asInteger(&c) == 123, "asInteger para entero positivo");
    ASSERT(i2->isInteger(&c), "isInteger para entero negativo");
    ASSERT(i2->asInteger(&c) == -456, "asInteger para entero negativo");
    ASSERT(b_true->isBoolean(&c), "isBoolean para true");
    ASSERT(b_true->asBoolean(&c) == true, "asBoolean para true");
    ASSERT(b_false->isBoolean(&c), "isBoolean para false");
    ASSERT(b_false->asBoolean(&c) == false, "asBoolean para false");
    ASSERT(i1->asBoolean(&c) == true, "Conversión de entero a booleano (true)");
    ASSERT(c.fromInteger(0)->asBoolean(&c) == false, "Conversión de entero a booleano (false)");
    ASSERT(byte->isByte(&c), "isByte para char");
    ASSERT(byte->asByte(&c) == 'A', "asByte para char");
}

void test_list(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoList ---\n");

    proto::ProtoList* list = c.newList();
    ASSERT(list->getSize(&c) == 0, "Una lista nueva debe estar vacía");

    list = list->appendLast(&c, c.fromInteger(10));
    list = list->appendLast(&c, c.fromInteger(20));
    list = list->appendLast(&c, c.fromInteger(30));

    ASSERT(list->getSize(&c) == 3, "Tamaño de la lista después de 3 inserciones");
    ASSERT(list->getAt(&c, 0)->asInteger(&c) == 10, "getAt(0)");
    ASSERT(list->getAt(&c, 1)->asInteger(&c) == 20, "getAt(1)");
    ASSERT(list->getAt(&c, 2)->asInteger(&c) == 30, "getAt(2)");
    ASSERT(list->getAt(&c, -1)->asInteger(&c) == 30, "getAt(-1) para el último elemento");

    ASSERT(list->has(&c, c.fromInteger(20)), "has() para un elemento existente");
    ASSERT(!list->has(&c, c.fromInteger(99)), "has() para un elemento no existente");

    proto::ProtoList* list2 = list->setAt(&c, 1, c.fromInteger(25));
    ASSERT(list->getAt(&c, 1)->asInteger(&c) == 20, "La lista original es inmutable después de setAt");
    ASSERT(list2->getAt(&c, 1)->asInteger(&c) == 25, "La nueva lista tiene el valor actualizado después de setAt");
    ASSERT(list2->getSize(&c) == 3, "El tamaño se preserva después de setAt");

    proto::ProtoList* list3 = list2->insertAt(&c, 0, c.fromInteger(5));
    ASSERT(list3->getSize(&c) == 4, "Tamaño después de insertAt al principio");
    ASSERT(list3->getAt(&c, 0)->asInteger(&c) == 5, "Valor después de insertAt al principio");
    ASSERT(list3->getAt(&c, 1)->asInteger(&c) == 10, "Valor desplazado después de insertAt");

    proto::ProtoList* list4 = list3->removeAt(&c, 2);
    ASSERT(list4->getSize(&c) == 3, "Tamaño después de removeAt");
    ASSERT(list4->getAt(&c, 1)->asInteger(&c) == 10, "Elemento antes del eliminado");
    ASSERT(list4->getAt(&c, 2)->asInteger(&c) == 30, "Elemento después del eliminado");

    proto::ProtoList* slice = list->getSlice(&c, 1, 3);
    ASSERT(slice->getSize(&c) == 2, "Tamaño del slice");
    ASSERT(slice->getAt(&c, 0)->asInteger(&c) == 20, "Elemento 0 del slice");
    ASSERT(slice->getAt(&c, 1)->asInteger(&c) == 30, "Elemento 1 del slice");

    // Probar el iterador
    proto::ProtoListIterator* iter = list->getIterator(&c);
    ASSERT(iter->hasNext(&c), "El iterador tiene hasNext en una lista no vacía");
    ASSERT(iter->next(&c)->asInteger(&c) == 10, "El iterador next() devuelve el primer elemento");
    iter = iter->advance(&c);
    ASSERT(iter->next(&c)->asInteger(&c) == 20, "El iterador next() devuelve el segundo elemento");
    iter = iter->advance(&c);
    ASSERT(iter->next(&c)->asInteger(&c) == 30, "El iterador next() devuelve el tercer elemento");
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

    proto::ProtoTuple* tuple1 = proto::ProtoTupleImplementation::tupleFromList(&c, list1);
    proto::ProtoTuple* tuple2 = proto::ProtoTupleImplementation::tupleFromList(&c, list2);

    ASSERT(tuple1->getSize(&c) == 2, "Tamaño de la tupla");
    ASSERT(tuple1->getAt(&c, 1)->asInteger(&c) == 2, "getAt de la tupla");
    ASSERT(tuple1 == tuple2, "Internalización: tuplas idénticas deben ser el mismo objeto");

    proto::ProtoList* list3 = c.newList();
    list3 = list3->appendLast(&c, c.fromInteger(1));
    list3 = list3->appendLast(&c, c.fromInteger(3));
    proto::ProtoTuple* tuple3 = proto::ProtoTupleImplementation::tupleFromList(&c, list3);

    ASSERT(tuple1 != tuple3, "Internalización: tuplas diferentes deben ser objetos diferentes");
}

void test_string(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoString ---\n");

    proto::ProtoString* s1 = c.fromUTF8String("hola");
    proto::ProtoString* s2 = c.fromUTF8String(" mundo");

    ASSERT(s1->getSize(&c) == 4, "Tamaño del string");
    ASSERT(s1->getAt(&c, 1)->asInteger(&c) == 'o', "getAt del string (un char es un entero)");

    proto::ProtoString* s3 = s1->appendLast(&c, s2);
    ASSERT(s3->getSize(&c) == 10, "Tamaño del string concatenado");

    proto::ProtoList* s3_list = s3->asList(&c);
    ASSERT(s3_list->getAt(&c, 4)->asInteger(&c) == ' ', "Verificación del contenido del string concatenado");
    ASSERT(s3_list->getAt(&c, 9)->asInteger(&c) == 'o', "Verificación del contenido al final");

    proto::ProtoString* slice = s3->getSlice(&c, 5, 10);
    ASSERT(slice->getSize(&c) == 5, "Tamaño del slice de string");
    ASSERT(slice->getAt(&c, 0)->asInteger(&c) == 'm', "Contenido del slice de string");
}

void test_sparse_list(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoSparseList (Diccionario) ---\n");

    proto::ProtoSparseList* dict = c.newSparseList();
    ASSERT(dict->getSize(&c) == 0, "Una lista dispersa nueva está vacía");

    proto::ProtoString* key1 = c.fromUTF8String("nombre");
    proto::ProtoString* key2 = c.fromUTF8String("edad");
    unsigned long key1_hash = key1->getHash(&c);
    unsigned long key2_hash = key2->getHash(&c);

    dict = dict->setAt(&c, key1_hash, (proto::ProtoObject*) c.fromUTF8String("proto"));
    dict = dict->setAt(&c, key2_hash, c.fromInteger(7));

    ASSERT(dict->getSize(&c) == 2, "Tamaño del diccionario después de dos inserciones");
    ASSERT(dict->has(&c, key1_hash), "has() del diccionario para una clave existente");
    ASSERT(!dict->has(&c, c.fromUTF8String("x")->getHash(&c)), "has() del diccionario para una clave no existente");

    proto::ProtoObject* name_val = dict->getAt(&c, key1_hash);
    // ASSERT(name_val->cmp_to_string(&c, c.fromUTF8String("proto")) == 0, "getAt() del diccionario para un valor string");
    ASSERT(dict->getAt(&c, key2_hash)->asInteger(&c) == 7, "getAt() del diccionario para un valor entero");

    proto::ProtoSparseList* dict2 = dict->removeAt(&c, key1_hash);
    ASSERT(dict->has(&c, key1_hash), "El diccionario original es inmutable después de removeAt");
    ASSERT(dict2->getSize(&c) == 1, "El nuevo diccionario tiene el tamaño correcto después de removeAt");
    ASSERT(!dict2->has(&c, key1_hash), "La clave se elimina en el nuevo diccionario");
    ASSERT(dict2->has(&c, key2_hash), "La otra clave se preserva en el nuevo diccionario");
}

void test_objects_and_prototypes(proto::ProtoContext& c) {
    printf("\n--- Probando ProtoObjectCell (Prototipos) ---\n");

    proto::ProtoObject* proto = (proto::ProtoObject*) new(&c) proto::ProtoObjectCellImplementation(&c, nullptr, 0, (proto::ProtoSparseListImplementation*) c.newSparseList());
    proto::ProtoString* attr_name = c.fromUTF8String("version");
    proto = proto->setAttribute(&c, attr_name, c.fromInteger(1));

    ASSERT(proto->hasOwnAttribute(&c, attr_name)->asBoolean(&c), "El prototipo tiene su propio atributo 'version'");
    ASSERT(proto->getAttribute(&c, attr_name)->asInteger(&c) == 1, "El valor del atributo 'version' es correcto");

    proto::ProtoObject* child = proto->newChild(&c);
    ASSERT(child->getParents(&c)->getSize(&c) == 1, "El hijo tiene un padre");
    ASSERT(!child->hasOwnAttribute(&c, attr_name)->asBoolean(&c), "El hijo NO tiene su propio atributo 'version'");
    ASSERT(child->hasAttribute(&c, attr_name)->asBoolean(&c), "El hijo SÍ tiene acceso al atributo 'version' (heredado)");
    ASSERT(child->getAttribute(&c, attr_name)->asInteger(&c) == 1, "El hijo hereda el valor correcto de 'version'");

    proto::ProtoObject* child2 = child->setAttribute(&c, attr_name, c.fromInteger(2));
    ASSERT(child->getAttribute(&c, attr_name)->asInteger(&c) == 1, "El objeto 'child' original no se modifica");
    ASSERT(child2->hasOwnAttribute(&c, attr_name)->asBoolean(&c), "El nuevo hijo SÍ tiene su propio atributo 'version'");
    ASSERT(child2->getAttribute(&c, attr_name)->asInteger(&c) == 2, "El nuevo hijo devuelve su propio valor para 'version'");

    proto::ProtoObject* proto2 = (proto::ProtoObject*) new(&c) proto::ProtoObjectCellImplementation(&c, nullptr, 0, (proto::ProtoSparseListImplementation*) c.newSparseList());
    proto::ProtoString* attr_name2 = c.fromUTF8String("author");
    proto2 = proto2->setAttribute(&c, attr_name2, (proto::ProtoObject*) c.fromUTF8String("gamarino"));

    proto::ProtoObject* child3 = child2->addParent(&c, (proto::ProtoObjectCell*) proto2);
    ASSERT(child3->getParents(&c)->getSize(&c) == 2, "El hijo ahora tiene dos padres");
    ASSERT(child3->getAttribute(&c, attr_name2)->isCell(&c), "El hijo puede acceder al atributo del segundo padre");
}

void test_gc_stress(proto::ProtoContext& c) {
    printf("\n--- Probando Recolector de Basura (GC Stress Test) ---\n");

    proto::ProtoList* root_list = c.newList();
    const int iterations = 2000;

    for (int i = 0; i < iterations; ++i) {
        proto::ProtoList* tempList = c.newList();
        tempList = tempList->appendLast(&c, c.fromInteger(i));
        tempList = tempList->appendLast(&c, (proto::ProtoObject*) c.fromUTF8String("temp string"));
        if (i % 100 == 0) {
            root_list = root_list->appendLast(&c, tempList->asObject(&c));
        }
    }

    ASSERT(root_list->getSize(&c) == iterations / 100, "La lista raíz tiene el número correcto de objetos persistentes");

    printf("   Forzando ejecución del GC...\n");
    c.space->triggerGC();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    proto::ProtoList* first_kept = (proto::ProtoList*) root_list->getAt(&c, 0);
    ASSERT(first_kept->getAt(&c, 0)->asInteger(&c) == 0, "El primer objeto persistente sigue siendo válido después del GC");

    proto::ProtoList* last_kept = (proto::ProtoList*) root_list->getAt(&c, (iterations / 100) - 1);
    ASSERT(last_kept->getAt(&c, 0)->asInteger(&c) == iterations - 100, "El último objeto persistente sigue siendo válido después del GC");

    printf("   GC Stress Test completado sin fallos.\n");
}

