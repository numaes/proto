/*
 * proto_object.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

namespace proto {

// --- Constructor y Destructor ---

// Constructor modernizado con lista de inicialización de miembros.
// AJUSTADO: Se eliminó el tipo de plantilla de 'ProtoSparseListImplementation'.
ProtoObjectCellImplementation::ProtoObjectCellImplementation(
	ProtoContext *context,
	ParentLinkImplementation	*parent,
	unsigned long mutable_ref,
	ProtoSparseListImplementation *attributes
) : Cell(context), parent(parent), mutable_ref(mutable_ref), attributes(attributes) {
	// El cuerpo del constructor ahora puede estar vacío.
}

// Para destructores vacíos, usar '= default' es la práctica recomendada.
ProtoObjectCellImplementation::~ProtoObjectCellImplementation() {};


// --- Métodos de la Interfaz ---

// CORREGIDO: Se ha corregido la llamada al constructor para que pase todos
// los parámetros necesarios, evitando un error de compilación.
ProtoObjectCellImplementation *ProtoObjectCellImplementation::addParent(
	ProtoContext *context,
	ProtoObjectCellImplementation *newParentToAdd
) {
	// Crea un nuevo eslabón en la cadena de herencia.
	ParentLinkImplementation* newParentLink = new(context) ParentLinkImplementation(
		context,
		this->parent,      // El padre del nuevo eslabón es nuestro padre actual.
		newParentToAdd     // El objeto del nuevo eslabón es el nuevo padre.
	);

	// Devuelve una nueva ProtoObjectCell que es una copia de la actual,
	// pero con la cadena de herencia extendida.
	return new(context) ProtoObjectCellImplementation(
		context,
		newParentLink,
		this->mutable_ref, // Se conservan las demás propiedades.
		this->attributes
	);
}

ProtoObject *ProtoObjectCellImplementation::asObject(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = (ProtoObject *) this;
    p.op.pointer_tag = POINTER_TAG_OBJECT;
    return p.oid.oid;
}


// --- Métodos del Recolector de Basura (GC) ---

// Un finalizador vacío también se puede declarar como 'default'.
void ProtoObjectCellImplementation::finalize(ProtoContext *context) {};

// Informa al GC sobre todas las referencias internas para que puedan ser rastreadas.
void ProtoObjectCellImplementation::processReferences(
	ProtoContext *context,
	void *self,
	void (*method)(
		ProtoContext *context,
		void *self,
		Cell *cell
	)
) {
	// 1. Procesar la referencia a la cadena de padres.
	if (this->parent) {
		method(context, self, this->parent);
	}

	// 2. Procesar la referencia a la lista de atributos.
	if (this->attributes) {
		method(context, self, this->attributes);
	}
}

	long unsigned ProtoObjectCellImplementation::getHash(ProtoContext *context) {
	// El hash de una Cell se deriva directamente de su dirección de memoria.
	// Esto proporciona un identificador rápido y único para el objeto.
	ProtoObjectPointer p;
	p.oid.oid = (ProtoObject *) this;

	return p.asHash.hash;
}


} // namespace proto