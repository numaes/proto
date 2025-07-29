/*
 * Proto.cpp
 *
 *  Created on: 2020-5-1
 *      Author: gamarino
 */

#include "../headers/proto_internal.h"

using namespace std;
namespace proto {

ProtoObject *ProtoObject::getPrototype(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

	switch (pa.op.pointer_tag) {
        case POINTER_TAG_EMBEDEDVALUE:
            switch (pa.op.embedded_type) {
                case EMBEDED_TYPE_BOOLEAN:
                    return context->space->booleanPrototype;
                case EMBEDED_TYPE_UNICODECHAR:
                    return context->space->unicodeCharPrototype;
                case EMBEDED_TYPE_BYTE:
                    return context->space->bytePrototype;
                case EMBEDED_TYPE_TIMESTAMP:
                    return context->space->timestampPrototype;
                case EMBEDED_TYPE_DATE:
                    return context->space->datePrototype;
                case EMBEDED_TYPE_TIMEDELTA:
                    return context->space->timedeltaPrototype;
                case EMBEDED_TYPE_SMALLINT:
                    return context->space->smallIntegerPrototype;
                case EMBEDED_TYPE_FLOAT:
                    return context->space->floatPrototype;
                default:
                    return nullptr;
            };
        case POINTER_TAG_LIST:
            return context->space->listPrototype;
        case POINTER_TAG_SPARSE_LIST:
            return context->space->sparseListPrototype;
        case POINTER_TAG_TUPLE:
            return context->space->tuplePrototype;
        case POINTER_TAG_BYTE_BUFFER:
            return context->space->bufferPrototype;
        case POINTER_TAG_EXTERNAL_POINTER:
            return context->space->pointerPrototype;
        case POINTER_TAG_METHOD:
            return context->space->methodPrototype;
        case POINTER_TAG_STRING:
            return context->space->stringPrototype;
        case POINTER_TAG_LIST_ITERATOR:
            return context->space->stringIteratorPrototype;
        case POINTER_TAG_TUPLE_ITERATOR:
            return context->space->tupleIteratorPrototype;
        case POINTER_TAG_SPARSE_LIST_ITERATOR:
            return context->space->sparseListIteratorPrototype;
        case POINTER_TAG_STRING_ITERATOR:
            return context->space->stringIteratorPrototype;
        default:
            return nullptr;
	}
}

ProtoObject *ProtoObject::clone(ProtoContext *context, bool isMutable) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;

        ProtoObject *newObject = (new(context) ProtoObjectCellImplementation(
            context,
            oc->parent,
            0,
            oc->attributes
        ))->asObject(context);

        if (isMutable) {
            ProtoSparseList *currentRoot;
            int randomId = 0;
            do {
                currentRoot = context->space->mutableRoot.load();

                int randomId = rand();
                if (randomId == 0)
                    continue;

                if (currentRoot->has(context, (unsigned long) randomId))
                    continue;

            } while (!context->space->mutableRoot.compare_exchange_strong(
                currentRoot,
                currentRoot->setAt(
                    context,
                    randomId,
                    newObject
                )
            ));
        }
        return newObject;

    }
    return PROTO_NONE;

}

ProtoObject *ProtoObject::newChild(ProtoContext *context, bool isMutable) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;

        ProtoObject *newObject = (new(context) ProtoObjectCellImplementation(
            context,
            new(context) ParentLinkImplementation(
                context,
                oc->parent,
                oc
            ),
            0,
            oc->attributes
        ))->asObject(context);

        if (isMutable) {
            ProtoSparseList *currentRoot;
            int randomId = 0;
            do {
                currentRoot = context->space->mutableRoot.load();

                int randomId = rand();

                if (randomId == 0)
                    continue;

                if (currentRoot->has(context, (unsigned long) randomId))
                    continue;

            } while (!context->space->mutableRoot.compare_exchange_strong(
                currentRoot,
                currentRoot->setAt(
                    context,
                    randomId,
                    newObject
                )
            ));
        }
        return newObject;

    }
    return PROTO_NONE;

}

ProtoObject *ProtoObject::getAttribute(ProtoContext *context, ProtoString *name) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
        if (oc->mutable_ref)
            oc = (ProtoObjectCellImplementation *)
                context->space->mutableRoot.load()->getAt(context, oc->mutable_ref);

        unsigned long hash = name->getHash(context);
        do {
            if (oc->attributes->has(context, hash))
                return oc->attributes->getAt(context, hash);
            if (oc->parent && oc->parent->object)
                oc = oc->parent->object;
            else
                break;
        } while (oc);
    }

    if (this->hasAttribute(context, context->space->literalGetAttribute)) {
        ProtoList *parameters = context->newList();

        return this->call(
            context,
            nullptr,
            context->space->literalGetAttribute,
            this,
            parameters->appendFirst(context, name->asObject(context))
        );
    }

    return PROTO_NONE;
}

ProtoObject *ProtoObject::hasAttribute(ProtoContext *context, ProtoString *name) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
        if (oc->mutable_ref) {
            oc = (ProtoObjectCellImplementation *)
                context->space->mutableRoot.load()->getAt(context, oc->mutable_ref);
        }

        unsigned long hash = name->getHash(context);
        do {
            if (oc->attributes->has(context, hash))
                return oc->attributes->getAt(context, hash);
            if (oc->parent && oc->parent->object)
                oc = oc->parent->object;
            else
                break;
        } while (oc);
    }
    return PROTO_FALSE;
}

ProtoObject *ProtoObject::setAttribute(ProtoContext *context, ProtoString *name, ProtoObject *value) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid, *inmutableBase = nullptr;
        ProtoSparseList *currentRoot, *newRoot;
        if (oc->mutable_ref) {
            inmutableBase = oc;
            currentRoot = context->space->mutableRoot.load();
            oc = (ProtoObjectCellImplementation *) currentRoot->getAt(context, oc->mutable_ref);
        }

        unsigned long hash = name->getHash(context);
        ProtoObject *newObject = (new(context) ProtoObjectCellImplementation(
                context,
                oc->parent,
                0,
                oc->attributes->setAt(context, hash, value)
            ))->asObject(context);

        if (inmutableBase) {
            do {
                currentRoot = context->space->mutableRoot.load();
                newRoot = currentRoot->setAt(
                    context, inmutableBase->mutable_ref, newObject);
            } while (context->space->mutableRoot.compare_exchange_strong(
                currentRoot,
                newRoot
            ));
            return this;
        }
        else {
            return (new(context) ProtoObjectCellImplementation(
                context,
                oc->parent,
                0,
                oc->attributes->setAt(context, hash, value)
            ))->asObject(context);
        }
    }
    return this;
}

ProtoObject *ProtoObject::hasOwnAttribute(ProtoContext *context, ProtoString *name) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;
        ProtoSparseList *currentRoot;
        if (oc->mutable_ref) {
            currentRoot = context->space->mutableRoot.load();
            oc = (ProtoObjectCell *) currentRoot->getAt(context, oc->mutable_ref);
        }

        unsigned long hash = name->getHash(context);
        return oc->attributes->has(context, hash)? PROTO_TRUE : PROTO_FALSE;
    }
    return PROTO_FALSE;
};

ProtoSparseList *ProtoObject::getAttributes(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
        ProtoSparseList *currentRoot = nullptr;
        if (oc->mutable_ref) {
            currentRoot = context->space->mutableRoot.load();
            oc = (ProtoObjectCellImplementation *) currentRoot->getAt(context, oc->mutable_ref);
        }

        ProtoSparseList *attributes = context->newSparseList();

        while (oc) {
            ProtoSparseListIterator *ai;

            ai = oc->attributes->getIterator(context);
            while (ai->hasNext(context)) {
				unsigned long attributeKey = ai->nextKey(context);
                ProtoObject* attributeValue = oc->attributes->getAt(context, attributeKey);
                attributes = attributes->setAt(
                    context,
					attributeKey,
					attributeValue
                );
                ai = ai->advance(context);
            }
            if (oc->parent) {
                oc = oc->parent->object;
                if (oc->mutable_ref)
                    oc = (ProtoObjectCellImplementation *) currentRoot->getAt(context, oc->mutable_ref);
            }

        }

        return attributes;
    }
    return nullptr;
};

ProtoSparseList *ProtoObject::getOwnAttributes(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;

        if (oc->mutable_ref) {
            ProtoObjectCell *mutableCurrentObject = (ProtoObjectCell *)
                context->space->mutableRoot.load()->getAt(context, oc->mutable_ref);
            return mutableCurrentObject->attributes;
        }
        else
            return oc->attributes;
    }
    return nullptr;
};

ProtoList *ProtoObject::getParents(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoList *parents = new(context) ProtoListImplementation(context);

        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
        ParentLinkImplementation *parent = (ParentLinkImplementation *) oc->parent;
        while (parent) {
            parents = parents->appendLast(context, parent->object->asObject(context));
        }

        return parents;
    }
    return nullptr;
};

void processTail(ProtoContext *context,
                 ProtoSparseList **existingParents,
                 ParentLinkImplementation *currentParent,
                 ParentLinkImplementation **newParentLink) {

    if (currentParent->parent)
        processTail(context, existingParents, currentParent, newParentLink);

    if (!(*existingParents)->has(context, currentParent->object->getHash(context))) {
        *newParentLink = new(context) ParentLinkImplementation(
                            context,
                            *newParentLink,
                            currentParent->object
        );
        (*existingParents) = (*existingParents)->setAt(
			context,
			currentParent->object->getHash(context),
			(*newParentLink)->asObject(context)
		);
    }
}

ProtoObject *ProtoObject::addParent(ProtoContext *context, ProtoObjectCell *newParent) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    ProtoSparseList *existingParents = context->newSparseList();

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCellImplementation *oc = (ProtoObjectCellImplementation *) pa.oid.oid;

        // Collect existing parents
        ParentLinkImplementation *currentParent = oc->parent;
        while (currentParent) {
            existingParents = existingParents->setAt(
                context,
                currentParent->object->getHash(context),
                nullptr
            );
            currentParent = currentParent->parent;
        };

        ParentLinkImplementation *newParentLink = oc->parent;

        return (new(context) ProtoObjectCellImplementation(
            context,
            newParentLink,
            oc->mutable_ref,
            oc->attributes
        ))->asObject(context);
    }
    else
        return this;
}



bool ProtoObject::isInteger(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
            p.op.embedded_type == EMBEDED_TYPE_SMALLINT);
}

int ProtoObject::asInteger(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return p.si.smallInteger;
}

// --- MÉTODO CORREGIDO ---
bool ProtoObject::isFloat(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
            p.op.embedded_type == EMBEDED_TYPE_FLOAT);
}

// --- MÉTODO CORREGIDO ---
float ProtoObject::asFloat(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;

    // Usamos una unión para reinterpretar los bits del entero
    // como un valor de punto flotante, que es la forma correcta
    // de hacer esta conversión.
    union {
        unsigned int uiv;
        float fv;
    } u;
    u.uiv = p.sd.floatValue;

    return u.fv;
}

bool ProtoObject::isBoolean(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
            p.op.embedded_type == EMBEDED_TYPE_BOOLEAN);
}

bool ProtoObject::asBoolean(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return p.booleanValue.booleanValue;
}

bool ProtoObject::isByte(ProtoContext* context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
            p.op.embedded_type == EMBEDED_TYPE_BYTE);
}

char ProtoObject::asByte(ProtoContext* context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return p.byteValue.byteData;
}

unsigned long ProtoObject::getHash(ProtoContext* context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return p.asHash.hash;
}

int ProtoObject::isCell(ProtoContext* context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return p.op.pointer_tag != POINTER_TAG_EMBEDEDVALUE;
}

ProtoObject* ProtoObject::call(ProtoContext* c,
                 ParentLink* nextParent,
                 ProtoString* method,
                 ProtoObject* self,
                 ProtoList* unnamedParametersList,
                 ProtoSparseList* keywordParametersDict) {
    return PROTO_NONE;
}

Cell* ProtoObject::asCell(ProtoContext* context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    return p.cell.cell;
}

void ProtoObject::processReferences(ProtoContext* context,
                 void* self,
                 void (*method)(
                    ProtoContext* context,
                    void* self,
                    Cell* cell
                 )) {
}

/*
 *  Este bloque proporciona implementaciones "stub" para las clases de la
 *  interfaz pública definidas en proto.h. Su único propósito es darle al
 *  compilador un lugar para generar las v-tables necesarias para el enlazado,
 *  respetando el diseño de ocultación de la implementación.
 *
 *  Estas funciones nunca deberían ser llamadas directamente. El polimorfismo
 *  debería redirigir las llamadas a las clases de implementación.
 */

    // --- Stubs para ParentLink ---
    ProtoObject* ParentLink::getObject(ProtoContext* context) { return PROTO_NONE; }
    ParentLink* ParentLink::getParent(ProtoContext* context) { return nullptr; }

    // --- Stubs para ProtoListIterator ---
    int ProtoListIterator::hasNext(ProtoContext* context) { return 0; }
    ProtoObject* ProtoListIterator::next(ProtoContext* context) { return PROTO_NONE; }
    ProtoListIterator* ProtoListIterator::advance(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoListIterator::asObject(ProtoContext* context) { return PROTO_NONE; }

    // --- Stubs para ProtoList ---
    ProtoObject* ProtoList::getAt(ProtoContext* context, int index) { return PROTO_NONE; }
    ProtoObject* ProtoList::getFirst(ProtoContext* context) { return PROTO_NONE; }
    ProtoObject* ProtoList::getLast(ProtoContext* context) { return PROTO_NONE; }
    ProtoList* ProtoList::getSlice(ProtoContext* context, int from, int to) { return nullptr; }
    unsigned long ProtoList::getSize(ProtoContext* context) { return 0; }
    bool ProtoList::has(ProtoContext* context, ProtoObject* value) { return false; }
    ProtoList* ProtoList::setAt(ProtoContext* context, int index, ProtoObject* value) { return nullptr; }
    ProtoList* ProtoList::insertAt(ProtoContext* context, int index, ProtoObject* value) { return nullptr; }
    ProtoList* ProtoList::appendFirst(ProtoContext* context, ProtoObject* value) { return nullptr; }
    ProtoList* ProtoList::appendLast(ProtoContext* context, ProtoObject* value) { return nullptr; }
    ProtoList* ProtoList::extend(ProtoContext* context, ProtoList* other) { return nullptr; }
    ProtoList* ProtoList::splitFirst(ProtoContext* context, int index) { return nullptr; }
    ProtoList* ProtoList::splitLast(ProtoContext* context, int index) { return nullptr; }
    ProtoList* ProtoList::removeFirst(ProtoContext* context) { return nullptr; }
    ProtoList* ProtoList::removeLast(ProtoContext* context) { return nullptr; }
    ProtoList* ProtoList::removeAt(ProtoContext* context, int index) { return nullptr; }
    ProtoList* ProtoList::removeSlice(ProtoContext* context, int from, int to) { return nullptr; }
    ProtoObject* ProtoList::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoList::getHash(ProtoContext* context) { return 0; }
    ProtoListIterator* ProtoList::getIterator(ProtoContext* context) { return nullptr; }

    // --- Stubs para ProtoTupleIterator ---
    int ProtoTupleIterator::hasNext(ProtoContext* context) { return 0; }
    ProtoObject* ProtoTupleIterator::next(ProtoContext* context) { return PROTO_NONE; }
    ProtoTupleIterator* ProtoTupleIterator::advance(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoTupleIterator::asObject(ProtoContext* context) { return PROTO_NONE; }

    // --- Stubs para ProtoTuple ---
    ProtoObject* ProtoTuple::getAt(ProtoContext* context, int index) { return PROTO_NONE; }
    ProtoObject* ProtoTuple::getFirst(ProtoContext* context) { return PROTO_NONE; }
    ProtoObject* ProtoTuple::getLast(ProtoContext* context) { return PROTO_NONE; }
    ProtoObject* ProtoTuple::getSlice(ProtoContext* context, int from, int to) { return nullptr; }
    unsigned long ProtoTuple::getSize(ProtoContext* context) { return 0; }
    bool ProtoTuple::has(ProtoContext* context, ProtoObject* value) { return false; }
    ProtoObject* ProtoTuple::setAt(ProtoContext* context, int index, ProtoObject* value) { return nullptr; }
    ProtoObject* ProtoTuple::insertAt(ProtoContext* context, int index, ProtoObject* value) { return nullptr; }
    ProtoObject* ProtoTuple::appendFirst(ProtoContext* context, ProtoTuple* otherTuple) { return nullptr; }
    ProtoObject* ProtoTuple::appendLast(ProtoContext* context, ProtoTuple* otherTuple) { return nullptr; }
    ProtoObject* ProtoTuple::splitFirst(ProtoContext* context, int count) { return nullptr; }
    ProtoObject* ProtoTuple::splitLast(ProtoContext* context, int count) { return nullptr; }
    ProtoObject* ProtoTuple::removeFirst(ProtoContext* context, int count) { return nullptr; }
    ProtoObject* ProtoTuple::removeLast(ProtoContext* context, int count) { return nullptr; }
    ProtoObject* ProtoTuple::removeAt(ProtoContext* context, int index) { return nullptr; }
    ProtoObject* ProtoTuple::removeSlice(ProtoContext* context, int from, int to) { return nullptr; }
    ProtoList* ProtoTuple::asList(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoTuple::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoTuple::getHash(ProtoContext* context) { return 0; }
    ProtoTupleIterator* ProtoTuple::getIterator(ProtoContext* context) { return nullptr; }

    // --- Stubs para ProtoStringIterator ---
    int ProtoStringIterator::hasNext(ProtoContext* context) { return 0; }
    ProtoObject* ProtoStringIterator::next(ProtoContext* context) { return PROTO_NONE; }
    ProtoStringIterator* ProtoStringIterator::advance(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoStringIterator::asObject(ProtoContext* context) { return PROTO_NONE; }

    // --- Stubs para ProtoString ---
    int ProtoString::cmp_to_string(ProtoContext* context, ProtoString* otherString) { return -1; }
    ProtoObject* ProtoString::getAt(ProtoContext* context, int index) { return PROTO_NONE; }
    ProtoString* ProtoString::setAt(ProtoContext* context, int index, ProtoObject* character) { return nullptr; }
    ProtoString* ProtoString::insertAt(ProtoContext* context, int index, ProtoObject* character) { return nullptr; }
    unsigned long ProtoString::getSize(ProtoContext* context) { return 0; }
    ProtoString* ProtoString::getSlice(ProtoContext* context, int from, int to) { return nullptr; }
    ProtoString* ProtoString::setAtString(ProtoContext* context, int index, ProtoString* otherString) { return nullptr; }
    ProtoString* ProtoString::insertAtString(ProtoContext* context, int index, ProtoString* otherString) { return nullptr; }
    ProtoString* ProtoString::appendFirst(ProtoContext* context, ProtoString* otherString) { return nullptr; }
    ProtoString* ProtoString::appendLast(ProtoContext* context, ProtoString* otherString) { return nullptr; }
    ProtoString* ProtoString::splitFirst(ProtoContext* context, int count) { return nullptr; }
    ProtoString* ProtoString::splitLast(ProtoContext* context, int count) { return nullptr; }
    ProtoString* ProtoString::removeFirst(ProtoContext* context, int count) { return nullptr; }
    ProtoString* ProtoString::removeLast(ProtoContext* context, int count) { return nullptr; }
    ProtoString* ProtoString::removeAt(ProtoContext* context, int index) { return nullptr; }
    ProtoString* ProtoString::removeSlice(ProtoContext* context, int from, int to) { return nullptr; }
    ProtoObject* ProtoString::asObject(ProtoContext* context) { return PROTO_NONE; }
    ProtoList* ProtoString::asList(ProtoContext* context) { return nullptr; }
    unsigned long ProtoString::getHash(ProtoContext* context) { return 0; }
    ProtoStringIterator* ProtoString::getIterator(ProtoContext* context) { return nullptr; }

    // --- Stubs para ProtoSparseListIterator ---
    int ProtoSparseListIterator::hasNext(ProtoContext* context) { return 0; }
    unsigned long ProtoSparseListIterator::nextKey(ProtoContext* context) { return 0; }
    ProtoObject* ProtoSparseListIterator::nextValue(ProtoContext* context) { return PROTO_NONE; }
    ProtoSparseListIterator* ProtoSparseListIterator::advance(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoSparseListIterator::asObject(ProtoContext* context) { return PROTO_NONE; }
    void ProtoSparseListIterator::finalize(ProtoContext* context) {}

    // --- Stubs para ProtoSparseList ---
    bool ProtoSparseList::has(ProtoContext* context, unsigned long index) { return false; }
    ProtoObject* ProtoSparseList::getAt(ProtoContext* context, unsigned long index) { return PROTO_NONE; }
    ProtoSparseList* ProtoSparseList::setAt(ProtoContext* context, unsigned long index, ProtoObject* value) { return nullptr; }
    ProtoSparseList* ProtoSparseList::removeAt(ProtoContext* context, unsigned long index) { return nullptr; }
    int ProtoSparseList::isEqual(ProtoContext* context, ProtoSparseList* otherDict) { return 0; }
    ProtoObject* ProtoSparseList::getAtOffset(ProtoContext* context, int offset) { return PROTO_NONE; }
    unsigned long ProtoSparseList::getSize(ProtoContext* context) { return 0; }
    ProtoObject* ProtoSparseList::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoSparseList::getHash(ProtoContext* context) { return 0; }
    ProtoSparseListIterator* ProtoSparseList::getIterator(ProtoContext* context) { return nullptr; }
    void ProtoSparseList::processElements(ProtoContext* context, void* self, void (*method)(ProtoContext* context, void* self, unsigned long index, ProtoObject* value)) {}
    void ProtoSparseList::processValues(ProtoContext* context, void* self, void (*method)(ProtoContext* context, void* self, ProtoObject* value)) {}

    // --- Stubs para ProtoByteBuffer ---
    unsigned long ProtoByteBuffer::getSize(ProtoContext* context) { return 0; }
    char* ProtoByteBuffer::getBuffer(ProtoContext* context) { return nullptr; }
    char ProtoByteBuffer::getAt(ProtoContext* context, int index) { return 0; }
    void ProtoByteBuffer::setAt(ProtoContext* context, int index, char value) {}
    ProtoObject* ProtoByteBuffer::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoByteBuffer::getHash(ProtoContext* context) { return 0; }

    // --- Stubs para ProtoExternalPointer ---
    void* ProtoExternalPointer::getPointer(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoExternalPointer::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoExternalPointer::getHash(ProtoContext* context) { return 0; }

    // --- Stubs para ProtoMethodCell ---
    ProtoObject* ProtoMethodCell::getSelf(ProtoContext* context) { return PROTO_NONE; }
    ProtoMethod ProtoMethodCell::getMethod(ProtoContext* context) { return nullptr; }
    ProtoObject* ProtoMethodCell::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoMethodCell::getHash(ProtoContext* context) { return 0; }

    // --- Stubs para ProtoObjectCell ---
    ProtoObjectCell* ProtoObjectCell::addParent(ProtoContext* context, ProtoObjectCell* object) { return nullptr; }

    // --- Stubs para ProtoThread ---
    ProtoThread* ProtoThread::getCurrentThread(ProtoContext* context) { return nullptr; }
    void ProtoThread::detach(ProtoContext* context) {}
    void ProtoThread::join(ProtoContext* context) {}
    void ProtoThread::exit(ProtoContext* context) {}
    ProtoObject* ProtoThread::getName(ProtoContext* context) { return PROTO_NONE; }
    ProtoObject* ProtoThread::asObject(ProtoContext* context) { return PROTO_NONE; }
    unsigned long ProtoThread::getHash(ProtoContext* context) { return 0; }
    void ProtoThread::setCurrentContext(ProtoContext* context) {}
    void ProtoThread::setManaged() {}
    void ProtoThread::setUnmanaged() {}
    void ProtoThread::synchToGC() {}

}

