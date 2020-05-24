/*
 * proto.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"
#include "../headers/proto_internal.h"


ProtoObjectCell *getBase(ProtoContext *context, ProtoObject *p) {
    ProtoObjectPointer pa;

    pa.oid = p;

	switch (pa.op.pointer_tag) {
	case POINTER_TAG_CELL:
        switch (pa.cell->type) {
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
        return (ProtoObjectCell *) context->space->objectPrototype;
    case POINTER_TAG_SMALLDOUBLE:
        return (ProtoObjectCell *) context->space->doublePrototype;
    case POINTER_TAG_SMALLINT:
        return (ProtoObjectCell *) context->space->integerPrototype;
	default:
        return NULL;
	};
}

ProtoObject *ProtoObject::clone(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid = this;

	if (pa.op.pointer_tag == POINTER_TAG_CELL &&
        pa.cell->type == CELL_TYPE_PROTO_OBJECT)
        return new(context) ProtoObjectCell(
            context,
            ((ProtoObjectCell *) this)->parent,
            ((ProtoObjectCell *) this)->attributes
        );

    return this;
}

ProtoObject *ProtoObject::newChild(ProtoContext *context) {
    ProtoObjectPointer pa;

    pa.oid = this;

	if (pa.op.pointer_tag == POINTER_TAG_CELL &&
        pa.cell->type == CELL_TYPE_PROTO_OBJECT)
        return new(context) ProtoObjectCell(
            context,
            new(context) ParentLink(
                context,
                ((ProtoObjectCell *) this)->parent,
                (ProtoObjectCell *) this
            ),
            new(context) IdentityDict(context)
        );

    return this;
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

    return newValue;
}

ProtoObject *ProtoObject::getType(ProtoContext *context) {
	ProtoObjectCell *base = getBase(context, this);

    if (base->parent)
        return base->parent->object;

    return base;
};

ProtoObject *ProtoObject::hasOwnAttribute(ProtoContext *context, ProtoObject *name) {
	ProtoObjectCell *base = getBase(context, this);
    ParentLink *pl = base->parent;

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
    p.oid = methodValue;
    if (methodValue != PROTO_NONE && 
        p.cell->type == CELL_TYPE_METHOD) {
        ProtoMethodCell *mc = (ProtoMethodCell *) p.cell;
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

ProtoObject *ProtoObject::currentValue() {
    return this;
};

