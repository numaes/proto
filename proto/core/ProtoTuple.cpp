/*
 * ProtoTuple.cpp
 *
 *  Revisado y corregido en 2024 para incorporar las últimas
 *  mejoras del proyecto, como la gestión de memoria moderna,
 *  correcciones de lógica y consistencia de la API.
 */

#include "../headers/proto_internal.h"
#include <algorithm> // Para std::max y otros algoritmos
#include <vector>    // Útil para la creación de tuplas

namespace proto
{
    // --- ProtoTupleIteratorImplementation ---

    // Constructor modernizado con lista de inicialización.
    ProtoTupleIteratorImplementation::ProtoTupleIteratorImplementation(
        ProtoContext* context,
        ProtoTupleImplementation* base,
        unsigned long currentIndex
    ) : Cell(context), base(base), currentIndex(currentIndex)
    {
    }

    // Destructor por defecto.
    ProtoTupleIteratorImplementation::~ProtoTupleIteratorImplementation() = default;

    int ProtoTupleIteratorImplementation::hasNext(ProtoContext* context)
    {
        // Es más seguro comprobar si la base no es nula.
        if (!this->base)
        {
            return false;
        }
        return this->currentIndex < this->base->getSize(context);
    }

    ProtoObject* ProtoTupleIteratorImplementation::next(ProtoContext* context)
    {
        // Usar hasNext para una comprobación robusta.
        if (!hasNext(context))
        {
            return PROTO_NONE;
        }
        // Devuelve el elemento actual. El avance se gestiona por separado.
        return this->base->getAt(context, this->currentIndex);
    }

    // CORRECCIÓN CRÍTICA: El iterador debe avanzar creando uno nuevo en la siguiente posición.
    ProtoTupleIterator* ProtoTupleIteratorImplementation::advance(ProtoContext* context)
    {
        return new(context) ProtoTupleIteratorImplementation(context, this->base, this->currentIndex + 1);
    }

    ProtoObject* ProtoTupleIteratorImplementation::asObject(ProtoContext* context)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        p.op.pointer_tag = POINTER_TAG_TUPLE_ITERATOR; // Asegurar el tag correcto.
        return p.oid.oid;
    }

    void ProtoTupleIteratorImplementation::finalize(ProtoContext* context)
    {
        // No se requiere limpieza especial.
    }

    // El GC debe conocer la tupla base para evitar que sea recolectada.
    void ProtoTupleIteratorImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        if (this->base)
        {
            method(context, self, this->base);
        }
    }


    // --- ProtoTupleImplementation ---

    // AJUSTE: La implementación se basa en una estructura de "cuerda" (rope) o árbol-B
    // para un manejo eficiente de la memoria y operaciones de slicing/concatenación.
    // Esta versión se centra en un nodo hoja para simplificar.

    // Constructor para un nuevo nodo hoja.
    ProtoTupleImplementation::ProtoTupleImplementation(
        ProtoContext* context,
        unsigned long elementCount,
        ProtoObject** data
    ) : Cell(context),
        elementCount(elementCount),
        height(0), // Los nodos hoja tienen altura 0.
        data(data)
    {
        // El hash se hereda de la clase base Cell, que usa la dirección de memoria.
    }

    // Constructor para un nuevo nodo indirecto
    ProtoTupleImplementation::ProtoTupleImplementation(
        ProtoContext* context,
        unsigned long elementCount,
        ProtoTupleImplementation** indirect
    ) : Cell(context),
        elementCount(elementCount),
        height(0), // Los nodos hoja tienen altura 0.
        indirect(indirect)
    {
        // El hash se hereda de la clase base Cell, que usa la dirección de memoria.
    }

    // El destructor libera la memoria del array de datos.
    // NOTA: Se asume que el array 'data' fue asignado con 'new[]'.
    ProtoTupleImplementation::~ProtoTupleImplementation()
    {
        delete[] data;
        data = nullptr;
    }

    // CORRECCIÓN: La creación de tuplas debe ser robusta y gestionar la memoria correctamente.
    ProtoTuple* ProtoTupleImplementation::tupleFromList(ProtoContext* context, ProtoList* list)
    {
        if (!list)
        {
            return new(context) ProtoTupleImplementation(context, 0, static_cast<ProtoObject**>(nullptr));
        }

        const unsigned long size = list->getSize(context);
        if (size == 0)
        {
            return new(context) ProtoTupleImplementation(context, 0, static_cast<ProtoObject**>(nullptr));
        }

        // El array 'data' en sí no es una 'Cell', por lo que se asigna con new[].
        ProtoObject** elements = new ProtoObject*[size];

        for (unsigned long i = 0; i < size; ++i)
        {
            elements[i] = list->getAt(context, i);
        }

        // Crear la tupla con los elementos copiados.
        return new(context) ProtoTupleImplementation(context, size, elements);
    }

    // CORRECCIÓN: El acceso a elementos debe ser seguro y manejar índices fuera de rango.
    ProtoObject* ProtoTupleImplementation::getAt(ProtoContext* context, int index)
    {
        if (index < 0)
        {
            index += this->elementCount;
        }

        if (index < 0 || (unsigned long)index >= this->elementCount)
        {
            return PROTO_NONE; // Índice fuera de rango.
        }

        // Para una implementación de hoja, el acceso es directo.
        if (this->data)
        {
            return this->data[index];
        }

        return PROTO_NONE; // Tupla mal formada.
    }

    // CORRECCIÓN CRÍTICA: La implementación de 'processReferences' es vital para el GC.
    // Debe recorrer todos los objetos que la tupla mantiene vivos.
    void ProtoTupleImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        if (this->data)
        {
            // Si es un nodo hoja
            for (unsigned long i = 0; i < this->elementCount; ++i)
            {
                ProtoObject* obj = this->data[i];
                if (obj && obj->isCell(context))
                {
                    method(context, self, obj->asCell(context));
                }
            }
        }
        // Una implementación completa de cuerda también procesaría nodos 'indirect'.
    }

    void ProtoTupleImplementation::finalize(ProtoContext* context)
    {
        // El destructor ya se encarga de liberar el array 'data'.
    }

    ProtoObject* ProtoTupleImplementation::asObject(ProtoContext* context)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        p.op.pointer_tag = POINTER_TAG_TUPLE; // Usar el tag correcto.
        return p.oid.oid;
    }

    unsigned long ProtoTupleImplementation::getHash(ProtoContext* context)
    {
        // La implementación de la clase base es suficiente y eficiente.
        return Cell::getHash(context);
    }

    ProtoTupleIterator* ProtoTupleImplementation::getIterator(ProtoContext* context)
    {
        // CORRECCIÓN: El iterador debe apuntar a 'this', no a 'nullptr'.
        return new(context) ProtoTupleIteratorImplementation(context, this, 0);
    }

    // Para tuplas inmutables, 'setAt' devuelve una *nueva* tupla con el cambio.
    ProtoTuple* ProtoTupleImplementation::setAt(ProtoContext* context, int index, ProtoObject* value)
    {
        if (index < 0)
        {
            index += this->elementCount;
        }

        if (index < 0 || (unsigned long)index >= this->elementCount)
        {
            return this; // Devolver la tupla original si el índice es inválido.
        }

        // Crear una copia de los datos.
        ProtoObject** newData = new ProtoObject*[this->elementCount];
        for (unsigned long i = 0; i < this->elementCount; ++i)
        {
            newData[i] = this->data[i];
        }

        // Modificar el valor en la copia.
        newData[index] = value;

        // Crear y devolver una nueva tupla con los datos modificados.
        return new(context) ProtoTupleImplementation(context, this->elementCount, newData);
    }
} // namespace proto
