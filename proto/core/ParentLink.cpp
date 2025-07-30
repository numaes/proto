/*
* parent_link.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

namespace proto
{
    // Usar la lista de inicialización de miembros es más idiomático y eficiente en C++.
    ParentLinkImplementation::ParentLinkImplementation(
        ProtoContext* context,
        ParentLinkImplementation* parent,
        ProtoObjectCellImplementation* object
    ) : Cell(context), parent(parent), object(object)
    {
        // El cuerpo del constructor ahora puede estar vacío.
    };

    // El destructor no necesita realizar ninguna acción.
    ParentLinkImplementation::~ParentLinkImplementation()
    {
    }

    void ParentLinkImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(
            ProtoContext* context,
            void* self,
            Cell* cell
        )
    )
    {
        // Para el recolector de basura, es crucial procesar todas las referencias a otras Cells.

        // 1. Procesar el enlace al padre anterior en la cadena.
        if (this->parent)
        {
            method(context, self, this->parent);
        }

        // 2. CORRECCIÓN CRÍTICA: Procesar el objeto (ProtoObjectCell) que este enlace representa.
        // La versión anterior no procesaba esta referencia, lo que causaría que el objeto
        // fuera recolectado incorrectamente por el GC.
        if (this->object)
        {
            method(context, self, reinterpret_cast<Cell*>(this->object));
        }

        // NOTA: La llamada a 'method(context, self, this)' fue eliminada.
        // El recolector de basura ya está procesando 'this' cuando llama a este método.
        // Volver a pasárselo a sí mismo causaría un bucle infinito durante la recolección.
    }

    // El método finalize debe ser implementado ya que es virtual puro en la clase base.
    void ParentLinkImplementation::finalize(ProtoContext* context)
    {
        // Este método se deja vacío intencionalmente porque ParentLinkImplementation
        // no adquiere recursos que necesiten una limpieza explícita.
    }

    ProtoObject* ParentLinkImplementation::asObject(ProtoContext* context)
    {
        return this->object->asObject(context);
    }

    unsigned long ParentLinkImplementation::getHash(ProtoContext* context)
    {
        // Deberíamos obtener el hash del objeto que este enlace representa,
        // no el hash del enlace en sí.
        if (this->object)
        {
            return this->object->getHash(context);
        }
        // Devolver 0 o algún otro valor por defecto si no hay objeto.
        return 0;
    }
};