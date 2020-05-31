/*
 * proto.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"


using namespace std;

namespace proto {

ProtoObjectCell *getBase(ProtoContext *context, ProtoObject *p) {
    ProtoObjectPointer pa;

    pa.oid.oid = p;

	switch (pa.op.pointer_tag) {
	case POINTER_TAG_CELL:
        switch (pa.cell.cell->type) {
        case CELL_TYPE_PROTO_OBJECT:
            return (ProtoObjectCell *) p;
        case CELL_TYPE_BYTE_BUFFER:
            return (ProtoObjectCell *) context->space->bufferPrototype;
        case CELL_TYPE_EXTERNAL_POINTER:
            return (ProtoObjectCell *) context->space->pointerPrototype;
        case CELL_TYPE_IDENTITY_DICT:
            return (ProtoObjectCell *) context->space->identityDictPrototype;
        case CELL_TYPE_PROTO_LIST:
            return (ProtoObjectCell *) context->space->protoListPrototype;
        case CELL_TYPE_PROTO_SET:
            return (ProtoObjectCell *) context->space->protoSetPrototype;
        case CELL_TYPE_PROTO_THREAD:
            return (ProtoObjectCell *) context->space->threadPrototype;
        case CELL_TYPE_METHOD:
            return (ProtoObjectCell *) context->space->methodPrototype;
        default:
            return NULL;
        };
    case POINTER_TAG_EMBEDEDVALUE:
        switch (pa.op.embedded_type) {
        case EMBEDED_TYPE_BOOLEAN:
            return (ProtoObjectCell *) context->space->booleanPrototype;
        case EMBEDED_TYPE_BYTE:
            return (ProtoObjectCell *) context->space->bytePrototype;
        case EMBEDED_TYPE_DATE:
            return (ProtoObjectCell *) context->space->datePrototype;
        case EMBEDED_TYPE_TIMEDELTA:
            return (ProtoObjectCell *) context->space->timedeltaPrototype;
        case EMBEDED_TYPE_TIMESTAMP:
            return (ProtoObjectCell *) context->space->timestampPrototype;
        default:
            return NULL;
        };
    case POINTER_TAG_MUTABLEOBJECT:
        return (ProtoObjectCell *) ((IdentityDict *) (context->space->mutableRoot.load()))->getAt(
            context, context->fromInteger((int) pa.mutableObject.mutableID)
        );
    case POINTER_TAG_SMALLDOUBLE:
        return (ProtoObjectCell *) context->space->doublePrototype;
    case POINTER_TAG_SMALLINT:
        return (ProtoObjectCell *) context->space->integerPrototype;
	default:
        return NULL;
	};
}

ProtoObject *ProtoObject::clone(ProtoContext *context) {
	ProtoObjectCell *base = getBase(context, this);

    return new(context) ProtoObjectCell(
        context,
        base->parent,
        base->attributes
    );
}

ProtoObject *ProtoObject::newChild(ProtoContext *context) {
	ProtoObjectCell *base = getBase(context, this);

    return new(context) ProtoObjectCell(
        context,
        new(context) ParentLink(
            context,
            base->parent,
            base
        ),
        new(context) IdentityDict(context)
    );
}

ProtoObject *ProtoObject::getAttribute(ProtoContext *context, ProtoObject *name) {
	ProtoObjectCell *base = getBase(context, this);
    ParentLink *pl = base->parent;

    while (base) {
        if (base->hasAttribute(context, name))
            return base->attributes->getAt(context, name);
        if (pl) {
            base = pl->object;
            pl = base->parent;
        }
    }
    return PROTO_NONE;
}

ProtoObject *ProtoObject::hasAttribute(ProtoContext *context, ProtoObject *name) {
	ProtoObjectCell *base = getBase(context, this);
    ParentLink *pl = base->parent;

    while (base) {
        if (base->hasAttribute(context, name))
            return PROTO_TRUE;
        if (pl) {
            base = pl->object;
            pl = base->parent;
        }
    }
    return PROTO_FALSE;
}

ProtoObject *ProtoObject::setAttribute(ProtoContext *context, ProtoObject *name, ProtoObject *value) {
	ProtoObjectCell *base = getBase(context, this);

    ProtoObjectCell *newValue = new(context) ProtoObjectCell(
        context,
        base->parent,
        base->attributes->setAt(context, name, value)
    );

    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_MUTABLEOBJECT) {
        IdentityDict *mr;
        Cell *oldValue;
        do {
            mr = (IdentityDict *) (context->space->mutableRoot.load());
            oldValue = mr;
        } while (
            context->space->mutableRoot.compare_exchange_strong(
                oldValue,
                mr->setAt(context, context->fromInteger(p.mutableObject.mutableID), newValue)
            )
        );    
    }

    return newValue;
}

ProtoObject *ProtoObject::currentValue(ProtoContext *context) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_MUTABLEOBJECT) {
        // Ensure a reference to the current value of the mutable is kept in the 
        // system at any time to block a garbage collection of the value.
        // If the mutable object is changed later, be sure
        // that the old value is referenced at least from this thread
        // Use an optimistic lock strategy in order not to block other threads!

        Cell *crc;
        do {
            IdentityDict *currentRoot = (IdentityDict *) (context->space->mutableRoot.load());
            crc = currentRoot; 
            ProtoObject *currentValue = currentRoot->getAt(
                context,
                context->fromInteger(p.mutableObject.mutableID)
            );
            ProtoMutableReference *mr = new(context) ProtoMutableReference(context, currentValue);
        } while (context->space->mutableRoot.compare_exchange_strong(
            crc,
            crc
        ));
    }

    return this;
};

BOOLEAN ProtoObject::setValue(ProtoContext *context, ProtoObject *oldValue, ProtoObject *newValue) {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_MUTABLEOBJECT) {
        IdentityDict *currentRoot = (IdentityDict *) context->space->mutableRoot.load();
        Cell *currentValue = currentRoot;
        ProtoObject *mutableId = context->fromInteger(p.mutableObject.mutableID);
        if (currentRoot->getAt(context, mutableId) == oldValue)
            return context->space->mutableRoot.compare_exchange_strong(
                currentValue,
                currentRoot->setAt(context, mutableId, newValue)
            );
    }
    return FALSE;
};

ProtoObject *ProtoObject::hasOwnAttribute(ProtoContext *context, ProtoObject *name) {
	ProtoObjectCell *base = getBase(context, this);

    if (base->hasAttribute(context, name))
        return PROTO_TRUE;
    return PROTO_FALSE;
};

struct scanAttributesState {
    ProtoSet *attributesSet;
};

void scanAttributes(
    ProtoContext *context,
    void *self,
    ProtoObject *value
) {
    struct scanAttributesState *state = (struct scanAttributesState *) self;

    state->attributesSet = state->attributesSet->add(context, value);
};

ProtoObject *ProtoObject::getAttributes(ProtoContext *context) {
	ProtoObjectCell *base = getBase(context, this);
    ParentLink *pl = base->parent;

    struct scanAttributesState state;

    state.attributesSet = new(context) ProtoSet(context);

    while (base) {
        base->attributes->processKeys(context, &state, scanAttributes);
        if (pl) {
            base = pl->object;
            pl = base->parent;
        }
    }

    return state.attributesSet;
};

ProtoObject *ProtoObject::getOwnAttributes(ProtoContext *context) {
	ProtoObjectCell *base = getBase(context, this);
    struct scanAttributesState state;

    state.attributesSet = new(context) ProtoSet(context);
    base->attributes->processKeys(context, &state, scanAttributes);

    return state.attributesSet;
};

ProtoObject *ProtoObject::getParent(ProtoContext *context) {
	ProtoObjectCell *base = getBase(context, this);

    if (base->parent)
        return base->parent->object;
    return PROTO_NONE;
};

ProtoObject *ProtoObject::addParent(ProtoContext *context, ProtoObject *newParent) {
	ProtoObjectCell *base = getBase(context, this);

    return new(context) ProtoObjectCell(
        context,
        new(context) ParentLink(
            context,
            base->parent,
            (ProtoObjectCell *) newParent
        ),
        base->attributes
    );
};

ProtoObject *ProtoObject::isInstanceOf(ProtoContext *context, ProtoObject *prototype) {
	ProtoObjectCell *base = getBase(context, this);

    ParentLink *pl = base->parent;

    while (base) {
        if (base == prototype)
            return PROTO_TRUE;

        if (pl) {
            base = pl->object;
            pl = base->parent;
        }
    }

    return PROTO_FALSE;
}; 

ProtoObject *ProtoObject::call(
    ProtoContext *context,
    ProtoObject *methodName,
    ProtoObject *unnamedParametersList,
    ProtoObject *keywordParametersDict
) {
    ProtoObject *methodValue = this->getAttribute(context, methodName);

    ProtoObjectPointer p;
    p.oid.oid = methodValue;
    if (methodValue != PROTO_NONE && 
        p.cell.cell->type == CELL_TYPE_METHOD) {
        ProtoMethodCell *mc = (ProtoMethodCell *) p.cell.cell;
        return (*(mc->method))(
            context,
            this, 
            mc->self,
            unnamedParametersList, 
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
    if (p.op.pointer_tag == POINTER_TAG_SMALLINT) 
        return p.si.smallInteger;
    else
        return 0;
};

double ProtoObject::asDouble() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_SMALLDOUBLE) {
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
    if (p.op.pointer_tag == POINTER_TAG_SMALLINT)
        return TRUE;
    else
        return FALSE;
};

BOOLEAN ProtoObject::isDouble() {
    ProtoObjectPointer p;
    p.oid.oid = this;
    if (p.op.pointer_tag == POINTER_TAG_SMALLDOUBLE)
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

};
