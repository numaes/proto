/*
 * ProtoByteBuffer.cpp
 *
 *  Created on: 2020-05-23
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"
#include <utility> // Para std::move y otras utilidades

namespace proto {

// --- Constructor ---
// Usa la lista de inicialización de miembros para mayor eficiencia y claridad.
ProtoByteBufferImplementation::ProtoByteBufferImplementation (
    ProtoContext *context,
    unsigned long size,
    char *buffer
) : Cell(context), size(size), buffer(nullptr), freeOnExit(false) {
    if (buffer) {
        // Si se proporciona un búfer externo, simplemente lo envolvemos.
        // No somos dueños de esta memoria.
        this->buffer = buffer;
        this->freeOnExit = false;
    } else {
        // Si no se proporciona un búfer, creamos uno nuevo.
        // Somos dueños de esta memoria y debemos liberarla.
        // Usar 'new char[size]' es más seguro y idiomático en C++ que 'malloc'.
        this->buffer = new char[size];
        this->freeOnExit = true;
    }
}

// --- Destructor ---
ProtoByteBufferImplementation::~ProtoByteBufferImplementation() {
    // Liberamos la memoria solo si somos los dueños.
    // Se usa 'delete[]' porque la memoria se asignó con 'new[]'.
    if (this->buffer && this->freeOnExit) {
        delete[] this->buffer;
    }
    // Usar nullptr es la práctica moderna en C++.
    this->buffer = nullptr;
}

// --- Métodos de Acceso ---

// Función auxiliar privada para normalizar el índice.
// Esto evita la duplicación de código y mejora la legibilidad.
bool ProtoByteBufferImplementation::normalizeIndex(int& index) {
    if (this->size == 0) {
        return false; // No hay índices válidos en un búfer vacío.
    }

    // Manejo de índices negativos (desde el final del búfer).
    if (index < 0) {
        index += this->size;
    }

    // Verificación de límites. Si está fuera de rango, no es válido.
    // Usar 'unsigned long' para la comparación evita problemas de signo.
    if (index < 0 || (unsigned long)index >= this->size) {
        return false;
    }

    return true;
}

char ProtoByteBufferImplementation::getAt(ProtoContext *context, int index) {
    // Usamos la función auxiliar para validar y normalizar el índice.
    if (normalizeIndex(index)) {
        return this->buffer[index];
    }
    // Si el índice no es válido, devolvemos 0 como valor por defecto.
    return 0;
}

void ProtoByteBufferImplementation::setAt(ProtoContext *context, int index, char value) {
    // Solo escribimos si el índice es válido.
    if (normalizeIndex(index)) {
        this->buffer[index] = value;
    }
}

// --- Métodos del Recolector de Basura (GC) ---

void ProtoByteBufferImplementation::processReferences(
    ProtoContext *context,
    void *self,
    void (*method) (
        ProtoContext *context,
        void *self,
        Cell *cell
    )
) {
    // CORRECCIÓN CRÍTICA: Este método DEBE estar vacío.
    // Un ProtoByteBuffer contiene datos crudos, no referencias a otras 'Cells'.
    // La implementación anterior (method(context, self, this)) causaría un
    // bucle infinito en el recolector de basura, bloqueando el programa.
}

// El finalizador no necesita hacer nada, por lo que usamos '= default'.
void ProtoByteBufferImplementation::finalize(ProtoContext *context) {};


// --- Métodos de Interfaz ---

ProtoObject *ProtoByteBufferImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_BYTE_BUFFER;
    return p.oid.oid;
}

unsigned long ProtoByteBufferImplementation::getHash(ProtoContext *context) {
// El hash de una Cell se deriva directamente de su dirección de memoria.
// Esto proporciona un identificador rápido y único para el objeto.
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;

    return p.asHash.hash;
}


} // namespace proto