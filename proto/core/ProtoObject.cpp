/*
 * proto_object.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "proto.h"
#include "proto_internal.h"

struct PointerAnalysis {
	union {
		union {
			Object *object;
			DictNode *dictNode;
			ListNode *listNode;
			SetNode *setNode;
			UnboundPointer *unboundPointer;
			Buffer *bufferPointer;
			LiteralNode *literalNode;
		} pointer;
		struct {
			unsigned pointerType:3;
			unsigned valueType:3;
		} tags;
	};
};

unsigned getType(ProtoObject *p) {
	struct PointerAnalysis pa;

	pa = (struct PointerAnalisis) p;
	return pa.tags.pointerType;
}

unsigned getValue(ProtoObject *p) {
	struct PointerAnalysis pa;

	pa = (struct PointerAnalisis) p;
	return pa.tags.valueType;
}

#define TYPE_SHIFT (3)
#define VALUE_SHIFT (6)
#define asType(p, t) (((int) p) << TYPE_SHIFT + (t))
#define asValue(p, t) (((int) p) << VALUE_SHIFT + ((t) << TYPE_SHIFT) + EMBEDED_VALUE)

ProtoObject *getBase(ProtoObject *p) {
    struct PointerAnalysis pa;

	pa = (struct PointerAnalisis) p;

	switch (pa.tags.pointerType ) {
	case OBJECT_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case DICT_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case LIST_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case SET_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case UNBOUND_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case BUFFER_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case LITERAL_POINTER:
		pa.tags.pointerType = 0x0;
		return pa.pointer.object;
	case EMBEDED_VALUE:
	default:
		switch (pa.tags.valueType) {
		case SMALL_INTEGER:
			return baseSpace->smallInteger;
		case SMALL_FLOAT:
			return baseSpace->smallFloat;
		case UNICODE_CHAR:
			return baseSpace->unicodeChar;
		case BOOLEAN:
			return baseSpace->booleanObject;
		case BYTE:
			return baseSpace->byteObject;
		case EXTENDED_VALUE:
		default:
			return baseSpace->extendedValue;
		}
	}
}

ProtoObject *ProtoObject::clone() {
	ProtoObject *base = getBase(this);

	// Only Objects can be cloned
	if (base != this) {
		return this;
	}
}

ProtoObject *ProtoObject::newChild() {
	ProtoObject *base = getBase(this);

	// Only Objects can be cloned
	if (base != this) {
		return this;
	}
}

ProtoObject *ProtoObject::getAttribute(ProtoObject *name) {
	ProtoObject *base = getBase(this);

	// Only Objects can be accessed
	if (base != this)
		return base->getAttribute(name);
	else {
		Object *o = (Object *) this;

		if (o->currentValue)
			return o->currentValue->getAttribute(name);
		else
			return PROTO_NONE;
	}
}

ProtoObject *ProtoObject::hasAttribute(ProtoObject *name) {
	ProtoObject *base = getBase(this);

	// Only Objects can be accessed
	if (base != this)
		return base->hasAttribute(name);
	else {
		Object *o = (Object *) this;

		if (o->currentValue)
			return o->currentValue->hasAttribute(name);
		else
			return PROTO_NONE;
	}
}

ProtoObject *ProtoObject::setAttribute(ProtoObject *name, ProtoObject *value) {
	ProtoObject *base = getBase(this);

	// Only Objects can be accessed
	if (base != this)
		return base->setAttribute(name, value);
	else {
		Object *o = (Object *) this;

		if (o->currentValue)
			return o->currentValue->setAttribute(name, value);
		else
			return PROTO_NONE;
	}
}


ProtoObject *ProtoObject::getAttributes() {

}

ProtoObject *ProtoObject::getParents() {

}


ProtoObject *ProtoObject::addParent(ProtoObject *newParent) {

}


ProtoObject *ProtoObject::getHash() {

}

ProtoObject *ProtoObject::isInstanceOf(ProtoObject *prototype) {

}

ProtoObject *ProtoObject::isMutable() {

}

ProtoObject *ProtoObject::asMutable() {

}

ProtoObject *ProtoObject::asNonmutable() {

}


ProtoObject *ProtoObject::call(ProtoObject *method,
							   ProtoObject *unnamedParameters,
		                       ProtoObject *keywordParameters) {
	ProtoObject *base = getBase(this);

	ProtoObject *m = base->getAttribute(method);

	ProtoMethod *methodPointer = m->asMethod();

	if (!methodPointer)
		return PROTO_NONE;
	else
		return (*ProtoMethod)(this, unnamedParameters, keywordParameters);
}


ProtoObject *ProtoObject::fromInteger(int value) {
	return asValue(value, SMALL_INTEGER);
}

ProtoObject *ProtoObject::fromDouble(double value) {
	return asValue(value, SMALL_FLOAT);
}

ProtoObject *ProtoObject::fromLiteral(char *utf8Value) {
	ProtoObject *base = getBase(this);

	// Only Objects can be accessed
	if (base != this)
		return PROTO_NONE;
	else {
		Object *s = (Object *) this;
		return new(s->space) Object(NULL,
    							    asType(getLiteral(s->space->literalRoot, utf8Value), LITERAL_POINTER),
									FALSE);
	}
}

ProtoObject *ProtoObject::fromMethod(ProtoMethod *method) {
	return asType(method, UNBOUND_POINTER);
}

ProtoObject *ProtoObject::fromPointer(void *pointer) {
	return asType(pointer, UNBOUND_POINTER);
}

ProtoObject *ProtoObject::fromBuffer(int length) {
	ProtoObject *base = getBase(this);

	// Only Objects can be accessed
	if (base != this)
		return PROTO_NONE;
	else {
		Object *s = (Object *) this;
		return new(s->space) Object(NULL,
    							    asType(new(s->space) Buffer(length), BUFFER_POINTER),
									FALSE);
	}
}

ProtoObject *ProtoObject::fromBoolean(int value) {
	return value ? PROTO_TRUE : PROTO_FALSE;
}


int         ProtoObject::asInteger() {

}

double      ProtoObject::asDouble() {

}

void        ProtoObject::asLiteral(char *utf8Buffer, int maxLength) {

}

int         ProtoObject::asLiteralLength() {

}

ProtoMethod *ProtoObject::asMethod() {

}

void        *ProtoObject::asPointer() {

}

char		*ProtoObject::asBufferPointer() {

}

int			*ProtoObject::asBufferLength() {

}

int         *ProtoObject::asBoolean() {

}


void		ProtoObject::exitThread(int exitCode) {

}


