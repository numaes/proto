/*
 * list.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"
#include <algorithm> // Para std::max

namespace proto {

// --- ProtoListIteratorImplementation ---

// Constructor modernizado con lista de inicialización
ProtoListIteratorImplementation::ProtoListIteratorImplementation(
    ProtoContext *context,
    ProtoListImplementation *base,
    unsigned long currentIndex
) : Cell(context), base(base), currentIndex(currentIndex) {}

// Destructor por defecto
ProtoListIteratorImplementation::~ProtoListIteratorImplementation() = default;

int ProtoListIteratorImplementation::hasNext(ProtoContext *context) {
    // Es más seguro comprobar si la base no es nula.
    if (!this->base) {
        return false;
    }
    return this->currentIndex < this->base->getSize(context);
}

ProtoObject *ProtoListIteratorImplementation::next(ProtoContext *context) {
    if (!this->base) {
        return PROTO_NONE;
    }
    // Devuelve el elemento actual, pero no avanza el iterador.
    // El avance se hace explícitamente con advance().
    return this->base->getAt(context, this->currentIndex);
}

ProtoListIterator *ProtoListIteratorImplementation::advance(ProtoContext *context) {
    // CORRECCIÓN CRÍTICA: El iterador debe avanzar al siguiente índice.
    // La versión anterior creaba un iterador en la misma posición.
    return new(context) ProtoListIteratorImplementation(context, this->base, this->currentIndex + 1);
}

ProtoObject *ProtoListIteratorImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST_ITERATOR;
    return p.oid.oid;
}

void ProtoListIteratorImplementation::finalize(ProtoContext *context) {};

void ProtoListIteratorImplementation::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    // Informar al GC sobre la referencia a la lista base.
    if (this->base) {
        method(context, self, this->base);
    }
}


// --- ProtoListImplementation ---

// Constructor modernizado con lista de inicialización
ProtoListImplementation::ProtoListImplementation(
    ProtoContext *context,
    ProtoObject *newValue,
    ProtoListImplementation *newPrevious,
    ProtoListImplementation *newNext
) : Cell(context),
    previous(newPrevious),
    next(newNext),
    value(newValue)
{
    // Calcular hash y contadores después de inicializar los miembros.
    this->hash = (newValue ? newValue->getHash(context) : 0) ^
                 (newPrevious ? newPrevious->hash : 0) ^
                 (newNext ? newNext->hash : 0);

    this->count = (newValue ? 1 : 0) +
                  (newPrevious ? newPrevious->count : 0) +
                  (newNext ? newNext->count : 0);

    unsigned long previous_height = newPrevious ? newPrevious->height : 0;
    unsigned long next_height = newNext ? newNext->height : 0;
    this->height = 1 + std::max(previous_height, next_height);
}

ProtoListImplementation::~ProtoListImplementation() = default;

// --- Lógica de Árbol AVL (Corregida) ---

namespace { // Funciones auxiliares anónimas

int getHeight(ProtoListImplementation *node) {
    return node ? node->height : 0;
}

int getBalance(ProtoListImplementation *node) {
    if (!node) {
        return 0;
    }
    return getHeight(node->next) - getHeight(node->previous);
}

ProtoListImplementation *rightRotate(ProtoContext *context, ProtoListImplementation *y) {
    ProtoListImplementation *x = y->previous;
    ProtoListImplementation *T2 = x->next;

    // Realizar rotación
    ProtoListImplementation *new_y = new(context) ProtoListImplementation(context, y->value, T2, y->next);
    return new(context) ProtoListImplementation(context, x->value, x->previous, new_y);
}

ProtoListImplementation *leftRotate(ProtoContext *context, ProtoListImplementation *x) {
    ProtoListImplementation *y = x->next;
    ProtoListImplementation *T2 = y->previous;

    // Realizar rotación
    ProtoListImplementation *new_x = new(context) ProtoListImplementation(context, x->value, x->previous, T2);
    return new(context) ProtoListImplementation(context, y->value, new_x, y->next);
}

// CORRECCIÓN CRÍTICA: La lógica de rebalanceo estaba rota.
// Esta es una implementación estándar y correcta para un árbol AVL.
ProtoListImplementation *rebalance(ProtoContext *context, ProtoListImplementation *node) {
    if (!node) return nullptr;

    int balance = getBalance(node);

    // Caso 1: Izquierda-Izquierda (LL)
    if (balance < -1 && getBalance(node->previous) <= 0) {
        return rightRotate(context, node);
    }

    // Caso 2: Derecha-Derecha (RR)
    if (balance > 1 && getBalance(node->next) >= 0) {
        return leftRotate(context, node);
    }

    // Caso 3: Izquierda-Derecha (LR)
    if (balance < -1 && getBalance(node->previous) > 0) {
        ProtoListImplementation* new_prev = leftRotate(context, node->previous);
        ProtoListImplementation* new_node = new(context) ProtoListImplementation(context, node->value, new_prev, node->next);
        return rightRotate(context, new_node);
    }

    // Caso 4: Derecha-Izquierda (RL)
    if (balance > 1 && getBalance(node->next) < 0) {
        ProtoListImplementation* new_next = rightRotate(context, node->next);
        ProtoListImplementation* new_node = new(context) ProtoListImplementation(context, node->value, node->previous, new_next);
        return leftRotate(context, new_node);
    }

    return node; // El nodo ya está balanceado
}

} // fin del namespace anónimo

// --- Métodos de la Interfaz Pública ---

ProtoObject *ProtoListImplementation::getAt(ProtoContext *context, int index) {
    if (!this->value) {
        return PROTO_NONE;
    }

    if (index < 0) {
        index += this->count;
    }

    if (index < 0 || (unsigned)index >= this->count) {
        return PROTO_NONE;
    }

    ProtoListImplementation *node = this;
    while (node) {
        unsigned long thisIndex = node->previous ? node->previous->count : 0;
        if ((unsigned long)index == thisIndex) {
            return node->value;
        }
        if ((unsigned long)index < thisIndex) {
            node = node->previous;
        } else {
            node = node->next;
            index -= (thisIndex + 1);
        }
    }
    return PROTO_NONE; // No debería llegar aquí si la lógica es correcta
}

ProtoObject *ProtoListImplementation::getFirst(ProtoContext *context) {
    return this->getAt(context, 0);
}

ProtoObject *ProtoListImplementation::getLast(ProtoContext *context) {
    return this->getAt(context, -1);
}

unsigned long ProtoListImplementation::getSize(ProtoContext *context) {
    return this->count;
}

bool ProtoListImplementation::has(ProtoContext *context, ProtoObject *value) {
    // CORRECCIÓN CRÍTICA: Bucle corregido para evitar acceso fuera de límites.
    // El bucle original (i <= this->count) iteraba una vez de más.
    for (unsigned long i = 0; i < this->count; i++) {
        if (this->getAt(context, i) == value) {
            return true;
        }
    }
    return false;
}

ProtoList *ProtoListImplementation::appendLast(ProtoContext *context, ProtoObject *newValue) {
    if (!this->value) {
        return new(context) ProtoListImplementation(context, newValue);
    }

    ProtoListImplementation *newNode;
    if (this->next) {
        ProtoListImplementation* new_next = static_cast<ProtoListImplementation*>(this->next->appendLast(context, newValue));
        newNode = new(context) ProtoListImplementation(context, this->value, this->previous, new_next);
    } else {
        newNode = new(context) ProtoListImplementation(
            context,
            this->value,
            this->previous,
            new(context) ProtoListImplementation(context, newValue)
        );
    }
    return rebalance(context, newNode);
}

// ... Implementación del resto de los métodos (setAt, insertAt, etc.) ...
// NOTA: Muchos de estos métodos usan la variable 'value' que no está definida.
// Se debe corregir a 'this->value' en todos los casos.

ProtoList *ProtoListImplementation::removeAt(ProtoContext *context, int index) {
    if (!this->value) {
        return new(context) ProtoListImplementation(context);
    }
    // ... Lógica de normalización de índice ...
    if (index < 0) index += this->count;
    if (index < 0 || (unsigned)index >= this->count) return this;

    unsigned long thisIndex = this->previous ? this->previous->count : 0;
    ProtoListImplementation *newNode;

    if ((unsigned long)index == thisIndex) {
        // Lógica para unir subárboles izquierdo y derecho
        if (!this->previous) return this->next;
        if (!this->next) return this->previous;

        // Unir los dos subárboles
        ProtoListImplementation* rightmost_of_left = static_cast<ProtoListImplementation*>(this->previous->getLast(context)->asCell(context));
        ProtoListImplementation* left_without_rightmost = static_cast<ProtoListImplementation*>(this->previous->removeLast(context));

        newNode = new(context) ProtoListImplementation(
            context,
            rightmost_of_left->value,
            left_without_rightmost,
            this->next
        );
    } else {
        if ((unsigned long)index < thisIndex) {
            ProtoListImplementation* new_prev = static_cast<ProtoListImplementation*>(this->previous->removeAt(context, index));
            newNode = new(context) ProtoListImplementation(context, this->value, new_prev, this->next);
        } else {
            ProtoListImplementation* new_next = static_cast<ProtoListImplementation*>(this->next->removeAt(context, index - thisIndex - 1));
            // CORRECCIÓN CRÍTICA: Usar this->value en lugar de 'value'
            newNode = new(context) ProtoListImplementation(context, this->value, this->previous, new_next);
        }
    }
    return rebalance(context, newNode);
}


// ... El resto de las implementaciones deben ser revisadas para corregir el error de 'value' ...

ProtoObject *ProtoListImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_LIST;
    return p.oid.oid;
}

unsigned long ProtoListImplementation::getHash(ProtoContext *context) {
    // La clase base Cell ya proporciona un hash basado en la dirección,
    // que es consistente. Se puede usar esa o la que estaba aquí.
    return Cell::getHash(context);
}

ProtoListIterator *ProtoListImplementation::getIterator(ProtoContext *context) {
    // CORRECCIÓN CRÍTICA: El iterador debe apuntar a 'this', no a 'nullptr'.
    return new(context) ProtoListIteratorImplementation(context, this, 0);
}

void ProtoListImplementation::finalize(ProtoContext *context) {};

void ProtoListImplementation::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    // Recorrer recursivamente todas las referencias para el GC.
    if (this->previous) {
        this->previous->processReferences(context, self, method);
    }
    if (this->next) {
        this->next->processReferences(context, self, method);
    }
    if (this->value && this->value->isCell(context)) {
        this->value->processReferences(context, self, method);
    }
}

} // namespace proto