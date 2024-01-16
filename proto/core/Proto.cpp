/*
 * proto.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include <random>
#include "../headers/proto.h"


using namespace std;

namespace proto {

ProtoObject *getPrototype(ProtoContext *context, ProtoObject *p) {
    ProtoObjectPointer pa;

    pa.oid.oid = p;

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
                case EMBEDED_TYPE_SMALLDOUBLE:
                    return context->space->smallFloatPrototype;
                default:
                    return NULL;
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
        case POINTER_TAG_METHOD_CELL:
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
            return NULL;
	};
}

ProtoObject *ProtoObject::clone(ProtoContext *context, BOOLEAN isMutable) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;

        ProtoObject *newObject = (new(context) ProtoObjectCell(
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

ProtoObject *ProtoObject::newChild(ProtoContext *context, BOOLEAN isMutable) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;

        ProtoObject *newObject = (new(context) ProtoObjectCell(
            context,
            new(context) ParentLink(
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
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;
        if (oc->mutable_ref)
            oc = (ProtoObjectCell *)
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
            NULL,
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
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;
        if (oc->mutable_ref) {
            oc = (ProtoObjectCell *)
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
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid, *inmutableBase = NULL;
        ProtoSparseList *currentRoot, *newRoot;
        if (oc->mutable_ref) {
            inmutableBase = oc;
            currentRoot = context->space->mutableRoot.load();
            oc = (ProtoObjectCell *) currentRoot->getAt(context, oc->mutable_ref);
        }

        unsigned long hash = name->getHash(context);
        ProtoObject *newObject = (new(context) ProtoObjectCell(
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
            return (new(context) ProtoObjectCell(
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
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;
        ProtoSparseList *currentRoot = NULL;
        if (oc->mutable_ref) {
            currentRoot = context->space->mutableRoot.load();
            oc = (ProtoObjectCell *) currentRoot->getAt(context, oc->mutable_ref);
        }

        ProtoSparseList *attributes = context->newSparseList();

        while (oc) {
            ProtoSparseListIterator *ai;

            ai = oc->attributes->getIterator(context);
            while (ai->hasNext(context)) {
                ProtoTuple *tuple = ai->next(context);
                attributes = attributes->setAt(
                    context, 
                    tuple->getAt(context, 0)->asInteger(),
                    tuple->getAt(context, 1)
                );
                ai = ai->advance(context);
            }
            if (oc->parent) {
                oc = oc->parent->object;
                if (oc->mutable_ref)
                    oc = (ProtoObjectCell *) currentRoot->getAt(context, oc->mutable_ref);
            }

        }

        return attributes;
    }
    return NULL;
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
    return NULL;
};

ProtoList *ProtoObject::getParents(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoList *parents = new(context) ProtoList(context);

        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;
        ParentLink *parent = oc->parent;
        while (parent) {
            parents = parents->appendLast(context, parent->object->asObject(context));
        }

        return parents;
    }
    return NULL;
};

void processTail(ProtoContext *context, 
                 ProtoSparseList **existingParents, 
                 ParentLink *currentParent,
                 ParentLink **newParentLink) {

    if (currentParent->parent)
        processTail(context, existingParents, currentParent, newParentLink);

    if (!(*existingParents)->has(context, currentParent->object->getHash(context))) {
        (*existingParents) = (*existingParents)->setAt(context, currentParent->object->getHash(context));
        *newParentLink = new(context) ParentLink(
                            context,
                            *newParentLink,
                            currentParent->object
        );
    }
}

ProtoObject *ProtoObject::addParent(ProtoContext *context, ProtoObjectCell *newParent) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    ProtoSparseList *existingParents = context->newSparseList();

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;

        // Collect existing parents
        ParentLink *currentParent = oc->parent;
        while (currentParent) {
            existingParents = existingParents->setAt(
                context, 
                currentParent->object->getHash(context),
                NULL
            );
        };

        ParentLink *newParentLink = oc->parent;

        // Collect non referenced yet parents of the new parent in the same order!
        processTail(context, &existingParents, currentParent, &newParentLink);    

        return (new(context) ProtoObjectCell(
            context,
            newParentLink,
            oc->mutable_ref,
            oc->attributes
        ))->asObject(context);
    }

    return NULL;
};

ProtoObject *ProtoObject::isInstanceOf(ProtoContext *context, ProtoObject *prototype) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObjectCell *oc = (ProtoObjectCell *) pa.oid.oid;

        ParentLink *p = oc->parent;
        while (p) {
            if (p->object->asObject(context) == prototype)
                return PROTO_TRUE;
            p = p->parent;
        }
        return PROTO_FALSE;
    }
    return PROTO_FALSE;
}; 

ProtoObject *ProtoObject::call(
    ProtoContext *context,
    ParentLink *nextParent,
    ProtoString *methodName,
    ProtoObject *self,
    ProtoList *unnamedParametersList,
    ProtoSparseList *keywordParametersDict
) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    if (pa.op.pointer_tag == POINTER_TAG_OBJECT) {
        ProtoObject *attribute = this->getAttribute(context, methodName);
        pa.oid.oid = attribute;

        if (pa.op.pointer_tag == POINTER_TAG_METHOD_CELL) {
            ProtoMethodCell *m = (ProtoMethodCell *) pa.oid.oid;

            return (*m->method)(
                context, 
                self,
                nextParent,
                unnamedParametersList, 
                keywordParametersDict);
        }
        else
            return this->call(
                context,
                nextParent,
                context->space->literalCallString,
                self,
                unnamedParametersList->appendFirst(context, methodName->asObject(context)),
                keywordParametersDict
            );
    }

    return PROTO_NONE;
};

BOOLEAN	ProtoObject::asBoolean() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_BOOLEAN) 
        return p.booleanValue.booleanValue;
    else
        return FALSE;
};

int ProtoObject::asInteger() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_SMALLINT) 
        return p.si.smallInteger;
    else
        return 0;
};

double ProtoObject::asDouble() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_SMALLDOUBLE) {
        union {
            double d;
            unsigned long l;
        } v;
        v.l = p.sd.smallDouble << TYPE_SHIFT;

        return v.d;
    }
    else
        return 0.0;
};

char ProtoObject::asByte() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_BYTE) 
        return p.byteValue.byteData;
    else
        return 0;
};

BOOLEAN	ProtoObject::isBoolean() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_BOOLEAN) 
        return TRUE;
    else
        return FALSE;
};

BOOLEAN	ProtoObject::isInteger() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_SMALLINT) 
        return TRUE;
    else
        return FALSE;
};

BOOLEAN ProtoObject::isDouble() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_SMALLDOUBLE) 
        return TRUE;
    else
        return FALSE;
};

BOOLEAN ProtoObject::isByte() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_EMBEDEDVALUE &&
        p.op.embedded_type == EMBEDED_TYPE_BYTE)
        return TRUE;
    else
        return FALSE;
};

unsigned long ProtoObject::getHash(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

    return (unsigned long) pa.oid.oid;
};

int ProtoObject::isCell(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid.oid = this;

	switch (pa.op.pointer_tag) {
        case POINTER_TAG_EMBEDEDVALUE:
            return FALSE;
        default:
            return TRUE;
    }

};

Cell *ProtoObject::asCell(ProtoContext *context) {
    if (this->isCell(context))
        return (Cell *) this;
    else
        return NULL;
};

void ProtoObject::finalize(ProtoContext *context) {};

// Apply method recursivelly to all referenced objects, except itself
void ProtoObject::processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	) {
    this->asCell(context)->processReferences(context, self, method);
}


};
