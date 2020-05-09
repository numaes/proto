/*
 * proto.h
 *
 *  Created on: November, 2017
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */

#ifndef PROTO_H_
#define PROTO_H_

#ifndef BOOLEAN
#define BOOLEAN int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0L
#endif

#define PROTO_NONE ((ProtoObject *) NULL)

class ProtoObject;

class ProtoContext;

typedef struct {
	ProtoObject *keyword;
	ProtoObject *value;
} KeywordParameter;

typedef ProtoObject *(*ProtoMethod)(
	ProtoContext *, 		// context
	ProtoObject *, 			// self
	ProtoObject *, 			// object
	int, 					// positionalCount
	int, 					// keywordCount
	KeywordParameter **, 	// keywordParameters
	ProtoObject **		 	// positionalParameters
);

class ProtoSpace {
public:
	ProtoSpace();
	virtual ~ProtoSpace();

	void * operator new(size_t size);

	ProtoObject	*objectPrototype;
	ProtoObject *integerPrototype;
	ProtoObject *charPrototype;
	ProtoObject *nonePrototype;
	ProtoObject *methodPrototype;
	ProtoObject *bufferPrototype;
	ProtoObject *pointerPrototype;
	ProtoObject *booleanPrototype;
	ProtoObject *doublePrototype;
	ProtoObject *datePrototype;
	ProtoObject *timestampPrototype;
	ProtoObject *timedeltaPrototype;

	ProtoObject *threadPrototype;

	ProtoObject *rootObject;

	ProtoObject *getLiteralFromString(ProtoContext *context, char *zeroTerminatedUtf8String);

	ProtoThread *getNewThread();

	ProtoObject *getThreads();
};

class ProtoContext {
public:
	void *operator new(size_t size);

	Cell *allocCell();
	void returnValue(ProtoObject *value=PROTO_NONE);

	// Constructors for base types, here to get the right context on invoke
	ProtoObject *fromInteger(int value);
	ProtoObject *fromDouble(double value);
	ProtoObject *fromUTF8Char(char *utf8OneCharString);
	ProtoObject *fromUTF8String(char *zeroTerminatedUtf8String);
	ProtoObject *fromMethod(ProtoMethod *method);
	ProtoObject *fromExternalPointer(void *pointer);
	ProtoObject *fromBuffer(char *pointer, unsigned long length);
	ProtoObject *fromBoolean(BOOLEAN value);
	ProtoObject *fromByte(char c);
	ProtoObject *literalFromString(char *zeroTerminatedUtf8String);

	ProtoObject *newMutable(ProtoObject *value=PROTO_NONE);
	ProtoThread *getCurrentThread();

};

class ProtoObject {
public:
	ProtoObject *clone(ProtoContext *c);
	ProtoObject *newChild(ProtoContext *c);

	ProtoObject *getAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *hasAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *hasOwnAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *setAttribute(ProtoContext *c, ProtoObject *name, ProtoObject *value);

	ProtoObject *getAttributes(ProtoContext *c);
	ProtoObject *getOwnAttributes(ProtoContext *c);
	ProtoObject *getParent(ProtoContext *c);

	ProtoObject *addParent(ProtoContext *c, ProtoObject *newParent);
	ProtoSpace	*getSpace(ProtoContext *c);
	ProtoObject *getHash();
	ProtoObject *isInstanceOf(ProtoContext *c, ProtoObject *prototype);

	ProtoObject *call(ProtoContext *c,
					  ProtoObject *method,
					  ProtoObject *unnamedParametersList,
			          ProtoObject *keywordParametersDict);

	// Only for mutables
	ProtoObject *currentValue();
	ProtoObject *setValue(ProtoObject *currentValue, ProtoObject *newValue);


	// Utilities for special values

	int         asInteger();
	double      asDouble();
	int			asUTF8Char();
	ProtoMethod *asMethod();
	void        *asPointer();
	BOOLEAN     *asBoolean();
	char		*asByte();

	ProtoObject *isMutable();
	ProtoObject	*isInteger();
	ProtoObject	*isDouble();
	ProtoObject	*isUTF8Char();
	ProtoObject	*isMethod();
	ProtoObject	*isPointer();
	ProtoObject	*isBoolean();
	ProtoObject	*isByte();
};

class ProtoMutableObject:ProtoObject {
public:
	ProtoObject	*mutableGetAt(ProtoContext *c, ProtoObject *key);
	ProtoObject *mutableHas(ProtoObject *key);
	void *mutableSetAt(ProtoObject *key, ProtoObject *value);
};

class ProtoExternalPointer:ProtoObject {
public:
	void	*getPointer();
	static	ProtoObject *fromPointer(void *value);
};

class ProtoSmallInt:ProtoObject {
public:
	int		asInt();
	static	ProtoObject *fromInt(int value);
};

class ProtoSmallDouble:ProtoObject {
public:
	double	asDouble();
	static	ProtoObject *fromDouble(double value);
};

class ProtoMethod:ProtoObject {
public:
	ProtoMethod	*asMethod();
	static	ProtoObject *fromMethod(ProtoMethod *value);
};

class ProtoBoolean:ProtoObject {
public:
	int		asBoolean();
	static	ProtoObject *fromBoolean(int value);
};

class ProtoByte:ProtoObject {
public:
	int		asByte();
	static	ProtoObject *fromByte(int value);
};

class ProtoPointer:ProtoObject {
public:
	void	*asPointer();
	static	ProtoObject *fromByte(int value);
};

#endif /* PROTO_H_ */
