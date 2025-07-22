/*
 * ProtoExternalPointer.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

namespace proto
{
    // --- Constructor y Destructor ---

    // Se utiliza la lista de inicialización de miembros, que es más eficiente e idiomática en C++.
    ProtoExternalPointerImplementation::ProtoExternalPointerImplementation(
        ProtoContext* context,
        void* pointer
    ) : Cell(context), pointer(pointer)
    {
        // El cuerpo del constructor ahora puede estar vacío.
    }

    // Para destructores vacíos, usar '= default' es la práctica recomendada.
    ProtoExternalPointerImplementation::~ProtoExternalPointerImplementation() = default;

    // --- Métodos de la Interfaz ---

    void* ProtoExternalPointerImplementation::getPointer(ProtoContext* context)
    {
        return this->pointer;
    }

    ProtoObject* ProtoExternalPointerImplementation::asObject(ProtoContext* context)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        p.op.pointer_tag = POINTER_TAG_EXTERNAL_POINTER;
        return p.oid.oid;
    }

    // --- Métodos del Recolector de Basura (GC) ---

    void ProtoExternalPointerImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(
            ProtoContext* context,
            void* self,
            Cell* cell
        )
    )
    {
        // Este método se deja vacío intencionadamente.
        // Un ProtoExternalPointer contiene un puntero opaco (void*) que no es
        // gestionado por el recolector de basura de Proto. Por lo tanto, no
        // hay referencias a otras 'Cells' que necesiten ser procesadas.
    }

    // Un finalizador vacío también se puede declarar como 'default'.
    void ProtoExternalPointerImplementation::finalize(ProtoContext* context)
    {
    };

    unsigned long ProtoExternalPointerImplementation::getHash(ProtoContext* context)
    {
        // El hash de una Cell se deriva directamente de su dirección de memoria.
        // Esto proporciona un identificador rápido y único para el objeto.
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;

        return p.asHash.hash;
    }
} // namespace proto
