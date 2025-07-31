/*
* Cell.cpp
 *
 *  Created on: 2020-05-01
 *      Author: gamarino
 */


#include "../headers/proto_internal.h"

namespace proto
{
    Cell::Cell(ProtoContext* context)
    {
        // Cada Cell recién creada se registra inmediatamente en el contexto actual
        // para la gestión de memoria y el seguimiento del recolector de basura.
        context->addCell2Context(this);
    };

    // Es una buena práctica usar '= default' para destructores simples en C++ moderno.
    Cell::~Cell() = default;

    unsigned long Cell::getHash(ProtoContext* context)
    {
        // El hash de una Cell se deriva directamente de su dirección de memoria.
        // Esto proporciona un identificador rápido y único para el objeto.
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;

        return p.asHash.hash;
    }

    // Implementación base para la finalización.
    // Las clases derivadas deben sobreescribir este método si necesitan realizar
    // alguna limpieza antes de ser reclamadas por el recolector de basura.
    void Cell::finalize(ProtoContext* context)
    {
        // No hace nada en la clase base.
    };

    ProtoObject* Cell::asObject(ProtoContext* context) {
        return reinterpret_cast<ProtoObject*>(this);
    }

    // Sobrecarga del operador 'new' para usar el asignador de memoria del contexto.
    void* Cell::operator new(unsigned long size, ProtoContext* context)
    {
        return context->allocCell();
    };

    // Implementación base para el recorrido de referencias del recolector de basura.
    // Las clases derivadas DEBEN sobreescribir este método para llamar al 'method'
    // en cada ProtoObject* o Cell* que contengan, permitiendo al GC marcar los objetos alcanzables.
    void Cell::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(
            ProtoContext* context,
            void* self,
            Cell* cell
        )
    )
    {
        // No hace nada en la clase base, ya que no contiene referencias a otros objetos.
    };
};
