/*
 * ProtoString.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"
#include <algorithm> // Para std::max y std::min

namespace proto
{
    // --- ProtoStringIteratorImplementation ---

    // Constructor modernizado con lista de inicialización.
    ProtoStringIteratorImplementation::ProtoStringIteratorImplementation(
        ProtoContext* context,
        ProtoStringImplementation* base,
        unsigned long currentIndex
    ) : Cell(context), base(base), currentIndex(currentIndex)
    {
    }

    // Destructor por defecto.
    ProtoStringIteratorImplementation::~ProtoStringIteratorImplementation() = default;

    int ProtoStringIteratorImplementation::hasNext(ProtoContext* context)
    {
        // Es más seguro comprobar si la base no es nula.
        if (!this->base)
        {
            return false;
        }
        return this->currentIndex < this->base->getSize(context);
    }

    ProtoObject* ProtoStringIteratorImplementation::next(ProtoContext* context)
    {
        if (!this->base)
        {
            return PROTO_NONE;
        }
        // Devuelve el elemento actual, pero no avanza el iterador.
        // El avance se hace explícitamente con advance().
        return this->base->getAt(context, this->currentIndex);
    }

    ProtoStringIteratorImplementation* ProtoStringIteratorImplementation::advance(ProtoContext* context)
    {
        // CORRECCIÓN CRÍTICA: El iterador debe avanzar al siguiente índice.
        // La versión anterior creaba un iterador en la misma posición.
        return new(context) ProtoStringIteratorImplementation(context, this->base, this->currentIndex + 1);
    }

    ProtoObject* ProtoStringIteratorImplementation::asObject(ProtoContext* context)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        // CORRECCIÓN CRÍTICA: Usar el tag correcto para el iterador de string.
        p.op.pointer_tag = POINTER_TAG_STRING_ITERATOR;
        return p.oid.oid;
    }

    // El finalizador no necesita hacer nada, por lo que usamos '= default'.
    void ProtoStringIteratorImplementation::finalize(ProtoContext* context)
    {
    };

    void ProtoStringIteratorImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        // CORRECCIÓN: Informar al GC sobre la referencia a la string base.
        if (this->base)
        {
            method(context, self, this->base);
        }
    }


    // --- ProtoStringImplementation ---

    // Constructor modernizado.
    ProtoStringImplementation::ProtoStringImplementation(
        ProtoContext* context,
        ProtoTuple* baseTuple
    ) : Cell(context), baseTuple(baseTuple)
    {
    }

    // Destructor por defecto.
    ProtoStringImplementation::~ProtoStringImplementation() = default;

    // --- Métodos de Acceso y Utilidad ---

    ProtoObject* ProtoStringImplementation::getAt(ProtoContext* context, int index)
    {
        // Delega directamente a la tupla base.
        return this->baseTuple->getAt(context, index);
    }

    unsigned long ProtoStringImplementation::getSize(ProtoContext* context)
    {
        // Delega directamente a la tupla base.
        return this->baseTuple->getSize(context);
    }

    // Función auxiliar para normalizar los índices de un slice.
    namespace
    {
        void normalizeSliceIndices(int& from, int& to, int size)
        {
            if (from < 0) from += size;
            if (to < 0) to += size;

            from = std::max(0, from);
            to = std::max(0, to);

            if (from > size) from = size;
            if (to > size) to = size;
        }
    }

    ProtoString* ProtoStringImplementation::getSlice(ProtoContext* context, int from, int to)
    {
        int thisSize = this->baseTuple->getSize(context);
        normalizeSliceIndices(from, to, thisSize);

        if (from >= to)
        {
            // Devuelve una string vacía si el rango no es válido.
            return new(context) ProtoStringImplementation(
                context, static_cast<ProtoTuple*>(context->newTuple()));
        }

        ProtoList* sourceList = context->newList();
        for (int i = from; i < to; i++)
        {
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        }

        return new(context) ProtoStringImplementation(
            context,
            static_cast<ProtoTuple*>(ProtoTupleImplementation::tupleFromList(context, sourceList))
        );
    }

    // --- Métodos de Modificación (Inmutables) ---

    ProtoString* ProtoStringImplementation::setAt(ProtoContext* context, int index, ProtoObject* value)
    {
        if (!value)
        {
            return this; // Devolver la string original si el valor es nulo.
        }

        int thisSize = this->baseTuple->getSize(context);
        if (index < 0)
        {
            index += thisSize;
        }

        if (index < 0 || index >= thisSize)
        {
            return this; // Índice fuera de rango, devolver la string original.
        }

        ProtoList* sourceList = context->newList();
        for (int i = 0; i < thisSize; i++)
        {
            if (i == index)
            {
                sourceList = sourceList->appendLast(context, value);
            }
            else
            {
                sourceList = sourceList->appendLast(context, this->getAt(context, i));
            }
        }

        return new(context) ProtoStringImplementation(
            context,
            static_cast<ProtoTuple*>(ProtoTupleImplementation::tupleFromList(context, sourceList))
        );
    }

    ProtoString* ProtoStringImplementation::insertAt(ProtoContext* context, int index, ProtoObject* value)
    {
        if (!value)
        {
            return this;
        }

        int thisSize = this->baseTuple->getSize(context);
        if (index < 0)
        {
            index += thisSize;
        }
        // Permitir inserción al final.
        if (index < 0) index = 0;
        if (index > thisSize) index = thisSize;

        ProtoList* sourceList = context->newList();
        for (int i = 0; i < index; i++)
        {
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        }
        sourceList = sourceList->appendLast(context, value);
        for (int i = index; i < thisSize; i++)
        {
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        }

        return new(context) ProtoStringImplementation(
            context,
            static_cast<ProtoTuple*>(ProtoTupleImplementation::tupleFromList(context, sourceList))
        );
    }

    ProtoString* ProtoStringImplementation::appendLast(ProtoContext* context, ProtoString* otherString)
    {
        if (!otherString)
        {
            return this;
        }

        ProtoList* sourceList = this->asList(context);
        unsigned long otherSize = otherString->getSize(context);
        for (unsigned long i = 0; i < otherSize; i++)
        {
            sourceList = sourceList->appendLast(context, otherString->getAt(context, i));
        }

        return new(context) ProtoStringImplementation(
            context,
            static_cast<ProtoTuple*>(ProtoTupleImplementation::tupleFromList(context, sourceList))
        );
    }

    ProtoString* ProtoStringImplementation::appendFirst(ProtoContext* context, ProtoString* otherString)
    {
        // CORRECCIÓN CRÍTICA: La lógica original era incorrecta.
        if (!otherString)
        {
            return this;
        }

        ProtoList* sourceList = otherString->asList(context);
        unsigned long thisSize = this->getSize(context);
        for (unsigned long i = 0; i < thisSize; i++)
        {
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        }

        return new(context) ProtoStringImplementation(
            context,
            static_cast<ProtoTuple*>(ProtoTupleImplementation::tupleFromList(context, sourceList))
        );
    }

    ProtoString* ProtoStringImplementation::removeSlice(ProtoContext* context, int from, int to)
    {
        // CORRECCIÓN: La lógica original creaba un slice, no eliminaba uno.
        int thisSize = this->baseTuple->getSize(context);
        normalizeSliceIndices(from, to, thisSize);

        if (from >= to)
        {
            return this; // No hay nada que eliminar.
        }

        ProtoList* sourceList = context->newList();
        // Parte antes del slice
        for (int i = 0; i < from; i++)
        {
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        }
        // Parte después del slice
        for (int i = to; i < thisSize; i++)
        {
            sourceList = sourceList->appendLast(context, this->getAt(context, i));
        }

        return new(context) ProtoStringImplementation(
            context,
            static_cast<ProtoTuple*>(ProtoTupleImplementation::tupleFromList(context, sourceList))
        );
    }

    ProtoString* ProtoStringImplementation::setAtString(ProtoContext* context, int index, ProtoString* otherString) { return PROTO_NONE; }
    ProtoString* ProtoStringImplementation::insertAtString(ProtoContext* context, int index, ProtoString* otherString) { return PROTO_NONE; }
    ProtoString* ProtoStringImplementation::splitFirst(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoString* ProtoStringImplementation::splitLast(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoString* ProtoStringImplementation::removeFirst(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoString* ProtoStringImplementation::removeLast(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoString* ProtoStringImplementation::removeAt(ProtoContext* context, int index) { return PROTO_NONE; }

    // --- Métodos de Conversión y GC ---

    ProtoListImplementation* ProtoStringImplementation::asList(ProtoContext* context)
    {
        // AJUSTADO: Se eliminó la sintaxis de plantillas.
        auto* result = dynamic_cast<ProtoListImplementation*>(context->newList());
        unsigned long thisSize = this->getSize(context);
        for (unsigned long i = 0; i < thisSize; i++)
        {
            result = result->appendLast(context, this->getAt(context, i));
        }
        return result;
    }

    void ProtoStringImplementation::finalize(ProtoContext* context)
    {
    };

    void ProtoStringImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        // Informar al GC sobre la tupla base que contiene los caracteres.
        if (this->baseTuple)
        {
            method(context, self, reinterpret_cast<Cell*>(this->baseTuple));
        }
    }

    ProtoObject* ProtoStringImplementation::asObject(ProtoContext* context)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        p.op.pointer_tag = POINTER_TAG_STRING;
        return p.oid.oid;
    }

    unsigned long ProtoStringImplementation::getHash(ProtoContext* context)
    {
        // Delega el hash a la tupla base para consistencia.
        // Si la tupla es la misma, el hash de la string será el mismo.
        return this->baseTuple ? this->baseTuple->getHash(context) : 0;
    }

    ProtoStringIteratorImplementation* ProtoStringImplementation::getIterator(ProtoContext* context)
    {
        // AJUSTADO: Se eliminó la sintaxis de plantillas.
        return new(context) ProtoStringIteratorImplementation(context, this, 0);
    }

    // ... Las demás implementaciones (removeFirst, removeLast, etc.) pueden
    // ser implementadas usando removeSlice para mayor simplicidad.
} // namespace proto
