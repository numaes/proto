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
        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;

        ProtoObject *newObject = (new(context) ProtoObjectCellImplementation(
            context,
            oc->parent,
            0,
            oc->attributes
        ))->implAsObject(context);

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
                (ProtoSparseList*) currentRoot->setAt(
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
        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;

        ProtoObject *newObject = (new(context) ProtoObjectCellImplementation(
            context,
            new(context) ParentLinkImplementation(
                context,
                oc->parent,
                oc
            ),
            0,
            oc->attributes
        ))->implAsObject(context);

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
                (ProtoSparseList*) currentRoot->setAt(
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
        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
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
            (ProtoList*) parameters->appendFirst(context, name->asObject(context))
        );
    }

    return PROTO_NONE;
}

ProtoObject *ProtoObject::hasAttribute(ProtoContext *context, ProtoString *name) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
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
                (ProtoSparseListImplementation*) oc->attributes->setAt(context, hash, value)
            ))->implAsObject(context);

        if (inmutableBase) {
            do {
                currentRoot = context->space->mutableRoot.load();
                newRoot = (ProtoSparseList*) currentRoot->setAt(
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
                (ProtoSparseListImplementation*) oc->attributes->setAt(context, hash, value)
            ))->implAsObject(context);
        }
    }
    return this;
}

ProtoObject *ProtoObject::hasOwnAttribute(ProtoContext *context, ProtoString *name) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        auto *oc = (ProtoObjectCell *) pa.oid.oid;
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
        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
        ProtoSparseList *currentRoot = nullptr;
        if (oc->mutable_ref) {
            currentRoot = context->space->mutableRoot.load();
            oc = (ProtoObjectCellImplementation *) currentRoot->getAt(context, oc->mutable_ref);
        }

        ProtoSparseList *attributes = context->newSparseList();

        while (oc) {
            ProtoListIterator *ai;

            ai = (ProtoListIterator*) oc->attributes->getIterator(context);
            while (ai->hasNext(context)) {
				unsigned long attributeKey = ((ProtoSparseListIterator*)ai)->nextKey(context);
                ProtoObject* attributeValue = oc->attributes->getAt(context, attributeKey);
                attributes = (ProtoSparseList*) attributes->setAt(
                    context,
					attributeKey,
					attributeValue
                );
                ai = (ProtoListIterator*) ((ProtoSparseListIterator*)ai)->advance(context);
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
        auto *oc = (ProtoObjectCell *) pa.oid.oid;

        if (oc->mutable_ref) {
            auto *mutableCurrentObject = (ProtoObjectCell *)
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

        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;
        auto *parent = (ParentLinkImplementation *) oc->parent;
        while (parent) {
            parents = (ProtoList*) parents->appendLast(context, parent->object->asObject(context));
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
        (*existingParents) = (ProtoSparseList*) (*existingParents)->setAt(
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
        auto *oc = (ProtoObjectCellImplementation *) pa.oid.oid;

        // Collect existing parents
        ParentLinkImplementation *currentParent = oc->parent;
        while (currentParent) {
            existingParents = (ProtoSparseList*) existingParents->setAt(
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
        ))->implAsObject(context);
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
    ProtoObjectPointer p{};
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



// ------------------- ParentLink -------------------

ProtoObject* ParentLink::getObject(ProtoContext* context) {
    return toImpl<ParentLinkImplementation>(this)->asObject(context);
}

ParentLink* ParentLink::getParent(ProtoContext* context) {
    return toImpl<ParentLinkImplementation>(this)->parent;
}

// ------------------- ProtoListIterator -------------------

int ProtoListIterator::hasNext(ProtoContext* context) {
    return toImpl<ProtoListIteratorImplementation>(this)->implHasNext(context);
}

ProtoObject* ProtoListIterator::next(ProtoContext* context) {
    return toImpl<ProtoListIteratorImplementation>(this)->implNext(context);
}

ProtoListIterator* ProtoListIterator::advance(ProtoContext* context) {
    return toImpl<ProtoListIteratorImplementation>(this)->implAdvance(context);
}

ProtoObject* ProtoListIterator::asObject(ProtoContext* context) {
    return toImpl<ProtoListIteratorImplementation>(this)->implAsObject(context);
}

// ------------------- ProtoList -------------------

ProtoObject* ProtoList::getAt(ProtoContext* context, int index) {
    return toImpl<ProtoListImplementation>(this)->implGetAt(context, index);
}

ProtoObject* ProtoList::getFirst(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implGetFirst(context);
}

ProtoObject* ProtoList::getLast(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implGetLast(context);
}

ProtoList* ProtoList::getSlice(ProtoContext* context, int from, int to) {
    return toImpl<ProtoListImplementation>(this)->implGetSlice(context, from, to);
}

unsigned long ProtoList::getSize(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implGetSize(context);
}

bool ProtoList::has(ProtoContext* context, ProtoObject* value) {
    return toImpl<ProtoListImplementation>(this)->implHas(context, value);
}

ProtoList* ProtoList::setAt(ProtoContext* context, int index, ProtoObject* value) {
    return toImpl<ProtoListImplementation>(this)->implSetAt(context, index, value);
}

ProtoList* ProtoList::insertAt(ProtoContext* context, int index, ProtoObject* value) {
    return toImpl<ProtoListImplementation>(this)->implInsertAt(context, index, value);
}

ProtoList* ProtoList::appendFirst(ProtoContext* context, ProtoObject* value) {
    return toImpl<ProtoListImplementation>(this)->implAppendFirst(context, value);
}

ProtoList* ProtoList::appendLast(ProtoContext* context, ProtoObject* value) {
    return toImpl<ProtoListImplementation>(this)->implAppendLast(context, value);
}

ProtoList* ProtoList::extend(ProtoContext* context, ProtoList* other) {
    return toImpl<ProtoListImplementation>(this)->implExtend(context, other);
}

ProtoList* ProtoList::splitFirst(ProtoContext* context, int index) {
    return toImpl<ProtoListImplementation>(this)->implSplitFirst(context, index);
}

ProtoList* ProtoList::splitLast(ProtoContext* context, int index) {
    return toImpl<ProtoListImplementation>(this)->implSplitLast(context, index);
}

ProtoList* ProtoList::removeFirst(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implRemoveFirst(context);
}

ProtoList* ProtoList::removeLast(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implRemoveLast(context);
}

ProtoList* ProtoList::removeAt(ProtoContext* context, int index) {
    return toImpl<ProtoListImplementation>(this)->implRemoveAt(context, index);
}

ProtoList* ProtoList::removeSlice(ProtoContext* context, int from, int to) {
    return toImpl<ProtoListImplementation>(this)->implRemoveSlice(context, from, to);
}

ProtoObject* ProtoList::asObject(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implAsObject(context);
}

unsigned long ProtoList::getHash(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->getHash(context);
}

ProtoListIterator* ProtoList::getIterator(ProtoContext* context) {
    return toImpl<ProtoListImplementation>(this)->implGetIterator(context);
}

// ------------------- ProtoTupleIterator -------------------

int ProtoTupleIterator::hasNext(ProtoContext* context) {
    return toImpl<ProtoTupleIteratorImplementation>(this)->implHasNext(context);
}

ProtoObject* ProtoTupleIterator::next(ProtoContext* context) {
    return toImpl<ProtoTupleIteratorImplementation>(this)->implNext(context);
}

ProtoTupleIterator* ProtoTupleIterator::advance(ProtoContext* context) {
    return toImpl<ProtoTupleIteratorImplementation>(this)->implAdvance(context);
}

ProtoObject* ProtoTupleIterator::asObject(ProtoContext* context) {
    return toImpl<ProtoTupleIteratorImplementation>(this)->implAsObject(context);
}

// ------------------- ProtoTuple -------------------

ProtoObject* ProtoTuple::getAt(ProtoContext* context, int index) {
    return toImpl<ProtoTupleImplementation>(this)->implGetAt(context, index);
}

ProtoObject* ProtoTuple::getFirst(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->implGetFirst(context);
}

ProtoObject* ProtoTuple::getLast(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->implGetLast(context);
}

ProtoObject* ProtoTuple::getSlice(ProtoContext* context, int from, int to) {
    return toImpl<ProtoTupleImplementation>(this)->implGetSlice(context, from, to);
}

unsigned long ProtoTuple::getSize(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->implGetSize(context);
}

bool ProtoTuple::has(ProtoContext* context, ProtoObject* value) {
    return toImpl<ProtoTupleImplementation>(this)->implHas(context, value);
}

ProtoObject* ProtoTuple::setAt(ProtoContext* context, int index, ProtoObject* value) {
    return toImpl<ProtoTupleImplementation>(this)->implSetAt(context, index, value);
}

ProtoObject* ProtoTuple::insertAt(ProtoContext* context, int index, ProtoObject* value) {
    return toImpl<ProtoTupleImplementation>(this)->implInsertAt(context, index, value);
}

ProtoObject* ProtoTuple::appendFirst(ProtoContext* context, ProtoTuple* otherTuple) {
    return toImpl<ProtoTupleImplementation>(this)->implAppendFirst(context, otherTuple);
}

ProtoObject* ProtoTuple::appendLast(ProtoContext* context, ProtoTuple* otherTuple) {
    return toImpl<ProtoTupleImplementation>(this)->implAppendLast(context, otherTuple);
}

ProtoObject* ProtoTuple::splitFirst(ProtoContext* context, int count) {
    return toImpl<ProtoTupleImplementation>(this)->implSplitFirst(context, count);
}

ProtoObject* ProtoTuple::splitLast(ProtoContext* context, int count) {
    return toImpl<ProtoTupleImplementation>(this)->implSplitLast(context, count);
}

ProtoObject* ProtoTuple::removeFirst(ProtoContext* context, int count) {
    return toImpl<ProtoTupleImplementation>(this)->implRemoveFirst(context, count);
}

ProtoObject* ProtoTuple::removeLast(ProtoContext* context, int count) {
    return toImpl<ProtoTupleImplementation>(this)->implRemoveLast(context, count);
}

ProtoObject* ProtoTuple::removeAt(ProtoContext* context, int index) {
    return toImpl<ProtoTupleImplementation>(this)->implRemoveAt(context, index);
}

ProtoObject* ProtoTuple::removeSlice(ProtoContext* context, int from, int to) {
    return toImpl<ProtoTupleImplementation>(this)->implRemoveSlice(context, from, to);
}

ProtoList* ProtoTuple::asList(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->implAsList(context);
}

ProtoObject* ProtoTuple::asObject(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->implAsObject(context);
}

unsigned long ProtoTuple::getHash(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->getHash(context);
}

ProtoTupleIterator* ProtoTuple::getIterator(ProtoContext* context) {
    return toImpl<ProtoTupleImplementation>(this)->implGetIterator(context);
}

// ------------------- ProtoStringIterator -------------------

int ProtoStringIterator::hasNext(ProtoContext* context) {
    return toImpl<ProtoStringIteratorImplementation>(this)->implHasNext(context);
}

ProtoObject* ProtoStringIterator::next(ProtoContext* context) {
    return toImpl<ProtoStringIteratorImplementation>(this)->implNext(context);
}

ProtoStringIterator* ProtoStringIterator::advance(ProtoContext* context) {
    return toImpl<ProtoStringIteratorImplementation>(this)->implAdvance(context);
}

ProtoObject* ProtoStringIterator::asObject(ProtoContext* context) {
    return toImpl<ProtoStringIteratorImplementation>(this)->implAsObject(context);
}

// ------------------- ProtoString -------------------

int ProtoString::cmp_to_string(ProtoContext* context, ProtoString* otherString) {
    return toImpl<ProtoStringImplementation>(this)->implCmpToString(context, otherString);
}

ProtoObject* ProtoString::getAt(ProtoContext* context, int index) {
    return toImpl<ProtoStringImplementation>(this)->implGetAt(context, index);
}

ProtoString* ProtoString::setAt(ProtoContext* context, int index, ProtoObject* character) {
    return toImpl<ProtoStringImplementation>(this)->implSetAt(context, index, character);
}

ProtoString* ProtoString::insertAt(ProtoContext* context, int index, ProtoObject* character) {
    return toImpl<ProtoStringImplementation>(this)->implInsertAt(context, index, character);
}

unsigned long ProtoString::getSize(ProtoContext* context) {
    return toImpl<ProtoStringImplementation>(this)->implGetSize(context);
}

ProtoString* ProtoString::getSlice(ProtoContext* context, int from, int to) {
    return toImpl<ProtoStringImplementation>(this)->implGetSlice(context, from, to);
}

ProtoString* ProtoString::setAtString(ProtoContext* context, int index, ProtoString* otherString) {
    return toImpl<ProtoStringImplementation>(this)->implSetAtString(context, index, otherString);
}

ProtoString* ProtoString::insertAtString(ProtoContext* context, int index, ProtoString* otherString) {
    return toImpl<ProtoStringImplementation>(this)->implInsertAtString(context, index, otherString);
}

ProtoString* ProtoString::appendFirst(ProtoContext* context, ProtoString* otherString) {
    return toImpl<ProtoStringImplementation>(this)->implAppendFirst(context, otherString);
}

ProtoString* ProtoString::appendLast(ProtoContext* context, ProtoString* otherString) {
    return toImpl<ProtoStringImplementation>(this)->implAppendLast(context, otherString);
}

ProtoString* ProtoString::splitFirst(ProtoContext* context, int count) {
    return toImpl<ProtoStringImplementation>(this)->implSplitFirst(context, count);
}

ProtoString* ProtoString::splitLast(ProtoContext* context, int count) {
    return toImpl<ProtoStringImplementation>(this)->implSplitLast(context, count);
}

ProtoString* ProtoString::removeFirst(ProtoContext* context, int count) {
    return toImpl<ProtoStringImplementation>(this)->implRemoveFirst(context, count);
}

ProtoString* ProtoString::removeLast(ProtoContext* context, int count) {
    return toImpl<ProtoStringImplementation>(this)->implRemoveLast(context, count);
}

ProtoString* ProtoString::removeAt(ProtoContext* context, int index) {
    return toImpl<ProtoStringImplementation>(this)->implRemoveAt(context, index);
}

ProtoString* ProtoString::removeSlice(ProtoContext* context, int from, int to) {
    return toImpl<ProtoStringImplementation>(this)->implRemoveSlice(context, from, to);
}

ProtoObject* ProtoString::asObject(ProtoContext* context) {
    return toImpl<ProtoStringImplementation>(this)->implAsObject(context);
}

ProtoList* ProtoString::asList(ProtoContext* context) {
    return toImpl<ProtoStringImplementation>(this)->implAsList(context);
}

unsigned long ProtoString::getHash(ProtoContext* context) {
    return toImpl<ProtoStringImplementation>(this)->getHash(context);
}

ProtoStringIterator* ProtoString::getIterator(ProtoContext* context) {
    return toImpl<ProtoStringImplementation>(this)->implGetIterator(context);
}

// ------------------- ProtoSparseListIterator -------------------

int ProtoSparseListIterator::hasNext(ProtoContext* context) {
    return toImpl<ProtoSparseListIteratorImplementation>(this)->implHasNext(context);
}

unsigned long ProtoSparseListIterator::nextKey(ProtoContext* context) {
    return toImpl<ProtoSparseListIteratorImplementation>(this)->implNextKey(context);
}

ProtoObject* ProtoSparseListIterator::nextValue(ProtoContext* context) {
    return toImpl<ProtoSparseListIteratorImplementation>(this)->implNextValue(context);
}

ProtoSparseListIterator* ProtoSparseListIterator::advance(ProtoContext* context) {
    return toImpl<ProtoSparseListIteratorImplementation>(this)->implAdvance(context);
}

ProtoObject* ProtoSparseListIterator::asObject(ProtoContext* context) {
    return toImpl<ProtoSparseListIteratorImplementation>(this)->implAsObject(context);
}

void ProtoSparseListIterator::finalize(ProtoContext* context) {
    toImpl<ProtoSparseListIteratorImplementation>(this)->finalize(context);
}

// ------------------- ProtoSparseList -------------------

bool ProtoSparseList::has(ProtoContext* context, unsigned long index) {
    return toImpl<ProtoSparseListImplementation>(this)->implHas(context, index);
}

ProtoObject* ProtoSparseList::getAt(ProtoContext* context, unsigned long index) {
    return toImpl<ProtoSparseListImplementation>(this)->implGetAt(context, index);
}

ProtoSparseList* ProtoSparseList::setAt(ProtoContext* context, unsigned long index, ProtoObject* value) {
    return toImpl<ProtoSparseListImplementation>(this)->implSetAt(context, index, value);
}

ProtoSparseList* ProtoSparseList::removeAt(ProtoContext* context, unsigned long index) {
    return toImpl<ProtoSparseListImplementation>(this)->implRemoveAt(context, index);
}

unsigned long ProtoSparseList::getSize(ProtoContext* context) {
    return toImpl<ProtoSparseListImplementation>(this)->implGetSize(context);
}

ProtoObject* ProtoSparseList::asObject(ProtoContext* context) {
    return toImpl<ProtoSparseListImplementation>(this)->implAsObject(context);
}

unsigned long ProtoSparseList::getHash(ProtoContext* context) {
    return toImpl<ProtoSparseListImplementation>(this)->getHash(context);
}

ProtoSparseListIterator* ProtoSparseList::getIterator(ProtoContext* context) {
    return toImpl<ProtoSparseListImplementation>(this)->implGetIterator(context);
}

void ProtoSparseList::processElements(ProtoContext* context, void* self,
                                      void (*method)(ProtoContext* context, void* self, unsigned long index,
                                                     ProtoObject* value)) {
    toImpl<ProtoSparseListImplementation>(this)->implProcessElements(context, self, method);
}

void ProtoSparseList::processValues(ProtoContext* context, void* self,
                                    void (*method)(ProtoContext* context, void* self, ProtoObject* value)) {
    toImpl<ProtoSparseListImplementation>(this)->implProcessValues(context, self, method);
}

// ------------------- ProtoByteBuffer -------------------

unsigned long ProtoByteBuffer::getSize(ProtoContext* context) {
    return toImpl<ProtoByteBufferImplementation>(this)->implGetSize(context);
}

char* ProtoByteBuffer::getBuffer(ProtoContext* context) {
    return toImpl<ProtoByteBufferImplementation>(this)->implGetBuffer(context);
}

char ProtoByteBuffer::getAt(ProtoContext* context, int index) {
    return toImpl<ProtoByteBufferImplementation>(this)->implGetAt(context, index);
}

void ProtoByteBuffer::setAt(ProtoContext* context, int index, char value) {
    toImpl<ProtoByteBufferImplementation>(this)->implSetAt(context, index, value);
}

ProtoObject* ProtoByteBuffer::asObject(ProtoContext* context) {
    return toImpl<ProtoByteBufferImplementation>(this)->implAsObject(context);
}

unsigned long ProtoByteBuffer::getHash(ProtoContext* context) {
    return toImpl<ProtoByteBufferImplementation>(this)->getHash(context);
}

// ------------------- ProtoExternalPointer -------------------

void* ProtoExternalPointer::getPointer(ProtoContext* context) {
    return toImpl<ProtoExternalPointerImplementation>(this)->implGetPointer(context);
}

ProtoObject* ProtoExternalPointer::asObject(ProtoContext* context) {
    return toImpl<ProtoExternalPointerImplementation>(this)->implAsObject(context);
}

unsigned long ProtoExternalPointer::getHash(ProtoContext* context) {
    return toImpl<ProtoExternalPointerImplementation>(this)->getHash(context);
}

// ------------------- ProtoMethodCell -------------------

ProtoObject* ProtoMethodCell::getSelf(ProtoContext* context) {
    return toImpl<ProtoMethodCellImplementation>(this)->implGetSelf(context);
}

ProtoMethod ProtoMethodCell::getMethod(ProtoContext* context) {
    return toImpl<ProtoMethodCellImplementation>(this)->implGetMethod(context);
}

ProtoObject* ProtoMethodCell::asObject(ProtoContext* context) {
    return toImpl<ProtoMethodCellImplementation>(this)->implAsObject(context);
}

unsigned long ProtoMethodCell::getHash(ProtoContext* context) {
    return toImpl<ProtoMethodCellImplementation>(this)->getHash(context);
}

// ------------------- ProtoObjectCell -------------------

ProtoObjectCell* ProtoObjectCell::addParent(ProtoContext* context, ProtoObjectCell* object) {
    return toImpl<ProtoObjectCellImplementation>(this)->implAddParent(context, object);
}

// ------------------- ProtoThread -------------------

ProtoThread* ProtoThread::getCurrentThread(ProtoContext* context) {
    return ProtoThreadImplementation::implGetCurrentThread(context);
}

void ProtoThread::detach(ProtoContext* context) {
    toImpl<ProtoThreadImplementation>(this)->implDetach(context);
}

void ProtoThread::join(ProtoContext* context) {
    toImpl<ProtoThreadImplementation>(this)->implJoin(context);
}

void ProtoThread::exit(ProtoContext* context) {
    toImpl<ProtoThreadImplementation>(this)->implExit(context);
}

ProtoObject* ProtoThread::getName(ProtoContext* context) {
    return (ProtoObject*) toImpl<ProtoThreadImplementation>(this)->name;
}

ProtoObject* ProtoThread::asObject(ProtoContext* context) {
    return toImpl<ProtoThreadImplementation>(this)->implAsObject(context);
}

unsigned long ProtoThread::getHash(ProtoContext* context) {
    return toImpl<ProtoThreadImplementation>(this)->getHash(context);
}

void ProtoThread::setCurrentContext(ProtoContext* context) {
    toImpl<ProtoThreadImplementation>(this)->implSetCurrentContext(context);
}

void ProtoThread::setManaged() {
    toImpl<ProtoThreadImplementation>(this)->implSetManaged();
}

void ProtoThread::setUnmanaged() {
    toImpl<ProtoThreadImplementation>(this)->implSetUnmanaged();
}

void ProtoThread::synchToGC() {
    toImpl<ProtoThreadImplementation>(this)->implSynchToGC();
}

} // namespace proto
