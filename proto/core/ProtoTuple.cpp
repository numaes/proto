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
    TupleDictionary::TupleDictionary(
        ProtoContext* context,
        ProtoTupleImplementation* key,
        TupleDictionary* next,
        TupleDictionary* previous
    ): Cell(context)
    {
        this->key = key;
        this->next = next;
        this->previous = previous;
        this->height = (key ? 1 : 0) + std::max((previous ? previous->height : 0), (next ? next->height : 0)),
            this->count = (previous ? previous->count : 0) + (key ? 1 : 0) + (next ? next->count : 0);
    };

    void TupleDictionary::finalize(ProtoContext* context)
    {
    };

    void TupleDictionary::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(
            ProtoContext* context,
            void* self,
            Cell* cell
        )
    )
    {
        if (this->next)
            this->next->processReferences(context, self, method);
        if (this->previous)
            this->previous->processReferences(context, self, method);
        (*method)(context, this, this);
    };

    int TupleDictionary::compareList(ProtoContext* context, ProtoList* list)
    {
        int thisSize = this->key->implGetSize(context);
        int listSize = list->getSize(context);

        int cmpSize = (thisSize < listSize) ? thisSize : listSize;
        int i;
        for (i = 0; i <= cmpSize; i++)
        {
            int thisElementHash = this->key->implGetAt(context, i)->getHash(context);
            int tupleElementHash = list->getAt(context, i)->getHash(context);
            if (thisElementHash > tupleElementHash)
                return 1;
            else if (thisElementHash < tupleElementHash)
                return 1;
        }
        if (i > thisSize)
            return -1;
        else if (i > listSize)
            return 1;
        return 0;
    };

    bool TupleDictionary::hasList(ProtoContext* context, ProtoList* list)
    {
        TupleDictionary* node = this;
        int cmp;

        // Empty tree case
        if (!this->key)
            return false;

        while (node)
        {
            cmp = node->compareList(context, list);
            if (cmp == 0)
                return true;
            if (cmp > 0)
                node = node->next;
            else
                node = node->previous;
        }
        return false;
    };

    bool TupleDictionary::has(ProtoContext* context, ProtoTuple* tuple)
    {
        TupleDictionary* node = this;
        int cmp;

        // Empty tree case
        if (!this->key)
            return false;

        while (node)
        {
            cmp = node->compareTuple(context, tuple);
            if (cmp == 0)
                return true;
            if (cmp > 0)
                node = node->next;
            else
                node = node->previous;
        }
        return false;
    };

    ProtoTupleImplementation* TupleDictionary::getAt(ProtoContext* context, ProtoTupleImplementation* tuple)
    {
        TupleDictionary* node = this;
        int cmp;

        // Empty tree case
        if (!this->key)
            return nullptr;

        while (node)
        {
            cmp = node->compareTuple(context, tuple);
            if (cmp == 0)
                return node->key;
            if (cmp > 0)
                node = node->next;
            else
                node = node->previous;
        }
        return nullptr;
    };

    TupleDictionary* TupleDictionary::set(ProtoContext* context, ProtoTupleImplementation* tuple)
    {
        TupleDictionary* newNode;
        int cmp;

        // Empty tree case
        if (!this->key)
            return new(context) TupleDictionary(
                context,
                tuple
            );

        cmp = this->compareTuple(context, tuple);
        if (cmp > 0)
        {
            if (this->next)
            {
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->previous,
                    this->next->set(context, tuple)
                );
            }
            else
            {
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->previous,
                    new(context) TupleDictionary(
                        context,
                        tuple
                    )
                );
            }
        }
        else if (cmp < 0)
        {
            if (this->previous)
            {
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->previous->set(context, tuple),
                    this->next
                );
            }
            else
            {
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    new(context) TupleDictionary(
                        context,
                        tuple
                    ),
                    this->next
                );
            }
        }
        else
            return this;

        return newNode->rebalance(context);
    };

    TupleDictionary* TupleDictionary::removeFirst(ProtoContext* context)
    {
        TupleDictionary* newNode;

        if (this->previous)
        {
            if (this->previous->previous)
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->next,
                    this->previous->removeFirst(context)
                );
            else if (this->previous->next)
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->next,
                    this->previous->next
                );
            else
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->next,
                    NULL
                );
        }
        else
        {
            return this->next;
        }

        return newNode->rebalance(context);
    };

    ProtoTupleImplementation* TupleDictionary::getFirst(ProtoContext* context)
    {
        TupleDictionary* node = this;

        while (node)
        {
            if (node->previous)
                node = node->previous;
            return node->key;
        }
        return NULL;
    };

    TupleDictionary* TupleDictionary::removeAt(ProtoContext* context, ProtoTupleImplementation* key)
    {
        TupleDictionary* newNode;

        if (this->key == key)
        {
            if (this->previous && this->next)
            {
                if (!this->previous->previous && !this->previous->next)
                    newNode = new(context) TupleDictionary(
                        context,
                        this->previous->key,
                        this->next
                    );
                else
                {
                    ProtoTupleImplementation* first = this->next->getFirst(context);
                    newNode = new(context) TupleDictionary(
                        context,
                        first,
                        this->next->removeFirst(context),
                        this->previous
                    );
                }
            }
            else if (this->previous)
                newNode = this->previous;
            else
                newNode = this->next;
        }
        else
        {
            if (key->getHash(context) < this->key->getHash(context))
            {
                newNode = this->previous->removeAt(context, key);
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    newNode,
                    this->next
                );
            }
            else
            {
                newNode = this->next->removeAt(context, key);
                newNode = new(context) TupleDictionary(
                    context,
                    this->key,
                    this->previous,
                    newNode
                );
            }
        }

        return newNode->rebalance(context);
    };

    int TupleDictionary::compareTuple(ProtoContext* context, ProtoTuple* tuple)
    {
        int thisSize = this->key->implGetSize(context);
        int tupleSize = tuple->getSize(context);

        int cmpSize = (thisSize < tupleSize) ? thisSize : tupleSize;
        int i;
        for (i = 0; i < cmpSize; i++)
        {
            int thisElementHash = this->key->implGetAt(context, i)->getHash(context);
            int tupleElementHash = tuple->getAt(context, i)->getHash(context);
            if (thisElementHash > tupleElementHash)
                return 1;
            else if (thisElementHash < tupleElementHash)
                return 1;
        }
        if (i > thisSize)
            return -1;
        else if (i > tupleSize)
            return 1;
        return 0;
    }

    // A utility function to right rotate subtree rooted with y
    // See the diagram given above.
    TupleDictionary* TupleDictionary::rightRotate(ProtoContext* context)
    {
        if (!this->previous)
            return this;

        TupleDictionary* newRight = new(context) TupleDictionary(
            context,
            this->key,
            this->previous->next,
            this->next
        );
        return new(context) TupleDictionary(
            context,
            this->previous->key,
            this->previous->previous,
            newRight
        );
    }

    // A utility function to left rotate subtree rooted with x
    // See the diagram given above.
    TupleDictionary* TupleDictionary::leftRotate(ProtoContext* context)
    {
        if (!this->next)
            return this;

        TupleDictionary* newLeft = new(context) TupleDictionary(
            context,
            this->key,
            this->previous,
            this->next->previous
        );
        return new(context) TupleDictionary(
            context,
            this->next->key,
            newLeft,
            this->next->next
        );
    }

    TupleDictionary* TupleDictionary::rebalance(ProtoContext* context)
    {
        TupleDictionary* newNode = this;
        while (true)
        {
            // If this node becomes unbalanced, then
            // there are 4 cases

            int balance = (newNode->next ? newNode->next->height : 0) - (newNode->previous
                                                                             ? newNode->previous->height
                                                                             : 0);

            if (balance >= -1 && balance <= 1)
                return newNode;

            // Left Left Case
            if (balance < -1)
            {
                newNode = newNode->rightRotate(context);
            }
            else
            {
                // Right Right Case
                if (balance > 1)
                {
                    newNode = newNode->leftRotate(context);
                }
                // Left Right Case
                else
                {
                    if (balance < 0 && newNode->previous)
                    {
                        newNode = new(context) TupleDictionary(
                            context,
                            newNode->key,
                            newNode->previous->leftRotate(context),
                            newNode->next
                        );
                        if (!newNode->previous)
                            return newNode;
                        newNode = newNode->rightRotate(context);
                    }
                    else
                    {
                        // Right Left Case
                        if (balance > 0 && newNode->next)
                        {
                            newNode = new(context) TupleDictionary(
                                context,
                                newNode->key,
                                newNode->previous,
                                newNode->next->rightRotate(context)
                            );
                            if (!newNode->next)
                                return newNode;
                            newNode = newNode->leftRotate(context);
                        }
                        else
                            return newNode;
                    }
                }
            }
        }
    }

    long unsigned int TupleDictionary::getHash(proto::ProtoContext*)
    {
        // El hash de una Cell se deriva directamente de su dirección de memoria.
        // Esto proporciona un identificador rápido y único para el objeto.
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;

        return p.asHash.hash;
    }

    proto::ProtoObject* TupleDictionary::asObject(proto::ProtoContext*)
    {
        ProtoObjectPointer p;
        p.oid.oid = (ProtoObject*)this;
        p.op.pointer_tag = POINTER_TAG_OBJECT;
        return p.oid.oid;
    }

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

    int ProtoTupleIteratorImplementation::implHasNext(ProtoContext* context)
    {
        // Es más seguro comprobar si la base no es nula.
        if (!this->base)
        {
            return false;
        }
        return this->currentIndex < this->base->implGetSize(context);
    }

    ProtoObject* ProtoTupleIteratorImplementation::implNext(ProtoContext* context)
    {
        // Usar hasNext para una comprobación robusta.
        if (!implHasNext(context))
        {
            return PROTO_NONE;
        }
        // Devuelve el elemento actual. El avance se gestiona por separado.
        return this->base->implGetAt(context, this->currentIndex);
    }

    // CORRECCIÓN CRÍTICA: El iterador debe avanzar creando uno nuevo en la siguiente posición.
    ProtoTupleIteratorImplementation* ProtoTupleIteratorImplementation::implAdvance(ProtoContext* context)
    {
        return new(context) ProtoTupleIteratorImplementation(context, this->base, this->currentIndex + 1);
    }

    ProtoObject* ProtoTupleIteratorImplementation::implAsObject(ProtoContext* context)
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
        const unsigned long elementCount,
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
        const unsigned long elementCount,
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
    ProtoTupleImplementation* ProtoTupleImplementation::implTupleFromList(ProtoContext* context, ProtoList* list)
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
    ProtoObject* ProtoTupleImplementation::implGetAt(ProtoContext* context, int index)
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

    ProtoObject* ProtoTupleImplementation::implAsObject(ProtoContext* context)
    {
        ProtoObjectPointer p{};
        p.oid.oid = reinterpret_cast<ProtoObject*>(this);
        p.op.pointer_tag = POINTER_TAG_TUPLE; // Usar el tag correcto.
        return p.oid.oid;
    }

    unsigned long ProtoTupleImplementation::getHash(ProtoContext* context)
    {
        // La implementación de la clase base es suficiente y eficiente.
        return Cell::getHash(context);
    }

    ProtoTupleIteratorImplementation* ProtoTupleImplementation::implGetIterator(ProtoContext* context)
    {
        // CORRECCIÓN: El iterador debe apuntar a 'this', no a 'nullptr'.
        return new(context) ProtoTupleIteratorImplementation(context, this, 0);
    }

    // Para tuplas inmutables, 'setAt' devuelve una *nueva* tupla con el cambio.
    ProtoObject* ProtoTupleImplementation::implSetAt(ProtoContext* context, int index, ProtoObject* value)
    {
        if (index < 0)
        {
            index += this->elementCount;
        }

        if (index < 0 || (unsigned long)index >= this->elementCount)
        {
            return this->implAsObject(context); // Devolver la tupla original si el índice es inválido.
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
        return (new(context) ProtoTupleImplementation(context, this->elementCount, newData))->implAsObject(context);
    }

    ProtoObject* ProtoTupleImplementation::implGetFirst(ProtoContext* context) { return implGetAt(context, 0); }
    ProtoObject* ProtoTupleImplementation::implGetLast(ProtoContext* context) { return implGetAt(context, implGetSize(context) - 1); }
    ProtoListImplementation* ProtoTupleImplementation::implAsList(ProtoContext* context) {
        ProtoList* list = context->newList();
        for (unsigned long i = 0; i < this->elementCount; ++i) {
            list = (ProtoList*) list->appendLast(context, this->data[i]);
        }
        return (ProtoListImplementation*) list;
    }

    ProtoObject* ProtoTupleImplementation::implGetSlice(ProtoContext* context, int from, int to) { return PROTO_NONE; }
    bool ProtoTupleImplementation::implHas(ProtoContext* context, ProtoObject* value) { return 0; }
    ProtoObject* ProtoTupleImplementation::implInsertAt(ProtoContext* context, int index, ProtoObject* value) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implAppendFirst(ProtoContext* context, ProtoTuple* otherTuple) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implAppendLast(ProtoContext* context, ProtoTuple* otherTuple) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implSplitFirst(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implSplitLast(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implRemoveFirst(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implRemoveLast(ProtoContext* context, int count) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implRemoveAt(ProtoContext* context, int index) { return PROTO_NONE; }
    ProtoObject* ProtoTupleImplementation::implRemoveSlice(ProtoContext* context, int from, int to) { return PROTO_NONE; }

    unsigned long ProtoTupleIteratorImplementation::getHash(ProtoContext* context) {
        return Cell::getHash(context);
    }

} // namespace proto