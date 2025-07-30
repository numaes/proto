/*
 * ProtoSparseList.cpp
 *
 *  Created on: 2017-05-01
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"
#include <algorithm> // Para std::max

namespace proto
{
    // --- ProtoSparseListIteratorImplementation ---

    // Constructor modernizado con lista de inicialización.
    ProtoSparseListIteratorImplementation::ProtoSparseListIteratorImplementation(
        ProtoContext* context,
        int state,
        ProtoSparseListImplementation* current,
        ProtoSparseListIteratorImplementation* queue
    ) : Cell(context), state(state), current(current), queue(queue)
    {
    }

    // Destructor por defecto.
    ProtoSparseListIteratorImplementation::~ProtoSparseListIteratorImplementation() = default;

    int ProtoSparseListIteratorImplementation::implHasNext(ProtoContext* context)
    {
        // La lógica original es compleja pero se mantiene.
        // Un iterador tiene "siguiente" si está en un estado válido o si la cola de iteradores tiene elementos.
        if (this->state == ITERATOR_NEXT_PREVIOUS && this->current && this->current->previous)
            return true;
        if (this->state == ITERATOR_NEXT_THIS && this->current)
            return true;
        if (this->state == ITERATOR_NEXT_NEXT && this->current && this->current->next)
            return true;
        if (this->queue)
            return this->queue->implHasNext(context);

        return false;
    }

    unsigned long ProtoSparseListIteratorImplementation::implNextKey(ProtoContext* context)
    {
        if (this->state == ITERATOR_NEXT_THIS && this->current)
        {
            return this->current->index;
        }
        return 0;
    }

    ProtoObject* ProtoSparseListIteratorImplementation::implNextValue(ProtoContext* context)
    {
        if (this->state == ITERATOR_NEXT_THIS && this->current)
        {
            return this->current->value;
        }
        return PROTO_NONE;
    }

    ProtoSparseListIteratorImplementation* ProtoSparseListIteratorImplementation::implAdvance(ProtoContext* context)
    {
        // La lógica de avance es compleja, creando una nueva cadena de iteradores
        // para mantener el estado del recorrido in-order.
        if (this->state == ITERATOR_NEXT_PREVIOUS)
        {
            return new(context) ProtoSparseListIteratorImplementation(
                context,
                ITERATOR_NEXT_THIS,
                this->current,
                this->queue
            );
        }

        if (this->state == ITERATOR_NEXT_THIS)
        {
            if (this->current && this->current->next)
            {
                // Si hay un subárbol derecho, el siguiente es el primer elemento de ese subárbol.
                return dynamic_cast<ProtoSparseListIteratorImplementation*>(this->current->next->implGetIterator(context));
            }
            if (this->queue)
            {
                // Si no hay subárbol derecho, el siguiente es el padre en la cola.
                return static_cast<ProtoSparseListIteratorImplementation*>(this->queue->implAdvance(context));
            }
            return nullptr; // Fin de la iteración.
        }

        if (this->state == ITERATOR_NEXT_NEXT && this->queue)
        {
            return static_cast<ProtoSparseListIteratorImplementation*>(this->queue->implAdvance(context));
        }

        return nullptr;
    }

    ProtoObject* ProtoSparseListIteratorImplementation::implAsObject(ProtoContext* context)
    {
        ProtoObjectPointer p{};
        p.oid.oid = reinterpret_cast<ProtoObject*>(this);
        p.op.pointer_tag = POINTER_TAG_SPARSE_LIST_ITERATOR;
        return p.oid.oid;
    }

    void ProtoSparseListIteratorImplementation::finalize(ProtoContext* context)
    {
    };

    void ProtoSparseListIteratorImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        // Informar al GC sobre las referencias internas.
        if (this->current)
        {
            method(context, self, this->current);
        }
        if (this->queue)
        {
            method(context, self, this->queue);
        }
    }

    unsigned long ProtoSparseListIteratorImplementation::getHash(ProtoContext* context)
    {
        return Cell::getHash(context);
    }


    // --- ProtoSparseListImplementation ---

    // Constructor modernizado.
    ProtoSparseListImplementation::ProtoSparseListImplementation(
        ProtoContext* context,
        const unsigned long index,
        ProtoObject* value,
        ProtoSparseListImplementation* previous,
        ProtoSparseListImplementation* next
    ) : Cell(context), previous(previous), next(next), index(index), value(value)
    {
        // Calcular hash, contador y altura después de inicializar los miembros.
        this->hash = index ^
            (value ? value->getHash(context) : 0) ^
            (previous ? previous->hash : 0) ^
            (next ? next->hash : 0);

        this->count = (value != PROTO_NONE ? 1 : 0) +
            (previous ? previous->count : 0) +
            (next ? next->count : 0);

        unsigned long previous_height = previous ? previous->height : 0;
        unsigned long next_height = next ? next->height : 0;
        this->height = 1 + std::max(previous_height, next_height);
    }

    ProtoSparseListImplementation::~ProtoSparseListImplementation() = default;

    // --- Lógica de Árbol AVL (Corregida) ---

    namespace
    {
        // Funciones auxiliares anónimas para la lógica del árbol.

        int getHeight(const ProtoSparseListImplementation* node)
        {
            return node ? node->height : 0;
        }

        int getBalance(const ProtoSparseListImplementation* node)
        {
            if (!node) return 0;
            return getHeight(node->next) - getHeight(node->previous);
        }

        ProtoSparseListImplementation* rightRotate(ProtoContext* context, ProtoSparseListImplementation* y)
        {
            ProtoSparseListImplementation* x = y->previous;
            ProtoSparseListImplementation* T2 = x->next;

            // Realizar rotación
            auto* new_y = new(context) ProtoSparseListImplementation(
                context, y->index, y->value, T2, y->next);
            return new(context) ProtoSparseListImplementation(context, x->index, x->value, x->previous, new_y);
        }

        ProtoSparseListImplementation* leftRotate(ProtoContext* context, const ProtoSparseListImplementation* x)
        {
            ProtoSparseListImplementation* y = x->next;
            ProtoSparseListImplementation* T2 = y->previous;

            // Realizar rotación
            auto* new_x = new(context) ProtoSparseListImplementation(
                context, x->index, x->value, x->previous, T2);
            return new(context) ProtoSparseListImplementation(context, y->index, y->value, new_x, y->next);
        }

        // CORRECCIÓN CRÍTICA: La lógica de rebalanceo estaba rota.
        // Esta es una implementación estándar y correcta para un árbol AVL.
        ProtoSparseListImplementation* rebalance(ProtoContext* context, ProtoSparseListImplementation* node)
        {
            if (!node) return nullptr;

            const int balance = getBalance(node);

            // Caso 1: Izquierda-Izquierda (LL)
            if (balance < -1 && getBalance(node->previous) <= 0)
            {
                return rightRotate(context, node);
            }
            // Caso 2: Derecha-Derecha (RR)
            if (balance > 1 && getBalance(node->next) >= 0)
            {
                return leftRotate(context, node);
            }
            // Caso 3: Izquierda-Derecha (LR)
            if (balance < -1 && getBalance(node->previous) > 0)
            {
                ProtoSparseListImplementation* new_prev = leftRotate(context, node->previous);
                auto* new_node = new(context) ProtoSparseListImplementation(
                    context, node->index, node->value, new_prev, node->next);
                return rightRotate(context, new_node);
            }
            // Caso 4: Derecha-Izquierda (RL)
            if (balance > 1 && getBalance(node->next) < 0)
            {
                ProtoSparseListImplementation* new_next = rightRotate(context, node->next);
                auto* new_node = new(context) ProtoSparseListImplementation(
                    context, node->index, node->value, node->previous, new_next);
                return leftRotate(context, new_node);
            }

            return node; // El nodo ya está balanceado.
        }
    } // fin del namespace anónimo

    // --- Métodos de la Interfaz Pública ---

    bool ProtoSparseListImplementation::implHas(ProtoContext* context, const unsigned long index)
    {
        // La búsqueda es más eficiente con un puntero no constante.
        const ProtoSparseListImplementation* node = this;
        while (node)
        {
            if (node->index == index)
            {
                return node->value != PROTO_NONE;
            }
            // CORRECCIÓN CRÍTICA: La comparación era incorrecta.
            if (index < node->index)
            {
                node = node->previous;
            }
            else
            {
                node = node->next;
            }
        }
        return false;
    }

    ProtoObject* ProtoSparseListImplementation::implGetAt(ProtoContext* context, const unsigned long index)
    {
        const ProtoSparseListImplementation* node = this;
        while (node)
        {
            if (node->index == index)
            {
                return node->value;
            }
            // CORRECCIÓN CRÍTICA: La comparación era incorrecta.
            if (index < node->index)
            {
                node = node->previous;
            }
            else
            {
                node = node->next;
            }
        }
        return PROTO_NONE;
    }

    ProtoSparseListImplementation* ProtoSparseListImplementation::implSetAt(ProtoContext* context, const unsigned long index,
                                                          ProtoObject* value)
    {
        ProtoSparseListImplementation* newNode;

        // Caso base: árbol vacío o nodo hoja.
        if (this->value == PROTO_NONE && this->count == 0)
        {
            return new(context) ProtoSparseListImplementation(context, index, value);
        }

        if (index < this->index)
        {
            ProtoSparseListImplementation* new_prev = this->previous
                                                          ? dynamic_cast<ProtoSparseListImplementation*>(this->previous->
                                                              implSetAt(context, index, value))
                                                          : new(context) ProtoSparseListImplementation(
                                                              context, index, value);
            newNode = new(context) ProtoSparseListImplementation(context, this->index, this->value, new_prev,
                                                                 this->next);
        }
        else if (index > this->index)
        {
            ProtoSparseListImplementation* new_next = this->next
                                                          ? dynamic_cast<ProtoSparseListImplementation*>(this->next->
                                                              implSetAt(context, index, value))
                                                          : new(context) ProtoSparseListImplementation(
                                                              context, index, value);
            newNode = new(context) ProtoSparseListImplementation(context, this->index, this->value, this->previous,
                                                                 new_next);
        }
        else
        {
            // index == this->index
            // Si el valor es el mismo, no hacer nada.
            if (this->value == value) return this;
            // Reemplazar el valor en el nodo actual.
            newNode = new(context) ProtoSparseListImplementation(context, this->index, value, this->previous,
                                                                 this->next);
        }

        return rebalance(context, newNode);
    }

    ProtoSparseListImplementation* ProtoSparseListImplementation::implRemoveAt(ProtoContext* context, const unsigned long index)
    {
        if (this->value == PROTO_NONE && this->count == 0)
        {
            return this; // No se encontró el elemento.
        }

        ProtoSparseListImplementation* newNode;

        if (index < this->index)
        {
            if (!this->previous) return this; // No se encontró.
            auto* new_prev = dynamic_cast<ProtoSparseListImplementation*>(this->previous->
                implRemoveAt(context, index));
            newNode = new(context) ProtoSparseListImplementation(context, this->index, this->value, new_prev,
                                                                 this->next);
        }
        else if (index > this->index)
        {
            if (!this->next) return this; // No se encontró.
            auto* new_next = dynamic_cast<ProtoSparseListImplementation*>(this->next->implRemoveAt(
                context, index));
            newNode = new(context) ProtoSparseListImplementation(context, this->index, this->value, this->previous,
                                                                 new_next);
        }
        else
        {
            // index == this->index
            // Nodo encontrado. Lógica de eliminación.
            if (!this->previous) return this->next; // Caso con 0 o 1 hijo (derecho).
            if (!this->next) return this->previous; // Caso con 1 hijo (izquierdo).

            // Caso con 2 hijos: encontrar el sucesor in-order (el más pequeño del subárbol derecho).
            const ProtoSparseListImplementation* successor = this->next;
            while (successor->previous)
            {
                successor = successor->previous;
            }
            // Eliminar el sucesor del subárbol derecho.
            auto* new_next = dynamic_cast<ProtoSparseListImplementation*>(this->next->implRemoveAt(
                context, successor->index));
            // Reemplazar este nodo con el sucesor.
            newNode = new(context) ProtoSparseListImplementation(context, successor->index, successor->value,
                                                                 this->previous, new_next);
        }

        return rebalance(context, newNode);
    }

    unsigned long ProtoSparseListImplementation::implGetSize(ProtoContext* context)
    {
        return this->count;
    }

    void ProtoSparseListImplementation::implProcessElements(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, unsigned long index, ProtoObject* value)
    )
    {
        // Recorrido in-order para procesar elementos.
        if (this->previous)
        {
            this->previous->implProcessElements(context, self, method);
        }
        if (this->value != PROTO_NONE)
        {
            method(context, self, this->index, this->value);
        }
        if (this->next)
        {
            this->next->implProcessElements(context, self, method);
        }
    }

    void ProtoSparseListImplementation::implProcessValues(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, ProtoObject* value)
    )
    {
        // Recorrido in-order para procesar solo los valores.
        if (this->previous)
        {
            this->previous->implProcessValues(context, self, method);
        }
        if (this->value != PROTO_NONE)
        {
            method(context, self, this->value);
        }
        if (this->next)
        {
            this->next->implProcessValues(context, self, method);
        }
    }

    void ProtoSparseListImplementation::processReferences(
        ProtoContext* context,
        void* self,
        void (*method)(ProtoContext* context, void* self, Cell* cell)
    )
    {
        // CORRECCIÓN CRÍTICA: La implementación anterior causaba un bucle infinito en el GC.
        // Ahora se procesan correctamente todas las referencias internas.
        if (this->previous)
        {
            method(context, self, this->previous);
        }
        if (this->next)
        {
            method(context, self, this->next);
        }
        if (this->value && this->value->isCell(context))
        {
            method(context, self, this->value->asCell(context));
        }
    }

    ProtoObject* ProtoSparseListImplementation::implAsObject(ProtoContext* context)
    {
        ProtoObjectPointer p{};
        p.oid.oid = reinterpret_cast<ProtoObject*>(this);
        p.op.pointer_tag = POINTER_TAG_SPARSE_LIST;
        return p.oid.oid;
    }

    ProtoSparseListIteratorImplementation* ProtoSparseListImplementation::implGetIterator(ProtoContext* context)
    {
        // La lógica del iterador es compleja, pero se mantiene la estructura original.
        // Encuentra el primer nodo (el más a la izquierda) y construye la cola de iteradores.
        auto node = this;
        ProtoSparseListIteratorImplementation* queue = nullptr;
        while (node->previous)
        {
            queue = new(context) ProtoSparseListIteratorImplementation(context, ITERATOR_NEXT_NEXT, node, queue);
            node = node->previous;
        }
        return new(context) ProtoSparseListIteratorImplementation(context, ITERATOR_NEXT_THIS, node, queue);
    }

    unsigned long ProtoSparseListImplementation::getHash(ProtoContext* context)
    {
        return this->hash;
    }

    void ProtoSparseListImplementation::finalize(ProtoContext* context)
    {
    };

    // El método getHash se hereda de la clase base Cell, que proporciona un hash
    // basado en la dirección, lo cual es suficiente y consistente.
} // namespace proto
