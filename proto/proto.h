/*
 * proto.h
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
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

class ProtoObject;

struct ProtoParameterDescription{
	ProtoObject	*name;
	ProtoObject *defaultValue;
};

struct ProtoParametersDescription{
	int parameterCount;
	struct ProtoParameterDescription *parameters;
};

typedef ProtoObject *ProtoMethod(
		ProtoObject *self,
		ProtoObject *unnamedPars,
		ProtoObject *keywordPars
);

class ProtoSpace {
public:
	void ProtoSpace(ProtoMM *memoryManager);
	virtual void ~ProtoSpace();

	virtual ProtoObject *getObject();
	virtual ProtoObject *createThread(void *runFunction(ProtoObject *parameters));
};

class ProtoObject {
public:
	ProtoObject *clone();
	ProtoObject *newChild();

	ProtoObject *getAttribute(ProtoObject *name);
	ProtoObject *hasAttribute(ProtoObject *name);

	ProtoObject *setAttribute(ProtoObject *name, ProtoObject *value);

	ProtoObject *getAttributes();
	ProtoObject *getParents();

	ProtoObject *addParent(ProtoObject *newParent);

	ProtoObject *getHash();
	ProtoObject *isInstanceOf(ProtoObject *prototype);
	ProtoObject *isMutable();
	ProtoObject *asMutable();
	ProtoObject *asNonmutable();

	ProtoObject *activate(ProtoObject *unnamedParameters,
			              ProtoObject *keywordParameters);

	ProtoObject *fromInteger(int value);
	ProtoObject *fromDouble(double value);
	ProtoObject *fromLiteral(char *utf8Value);
	ProtoObject *fromMethod(ProtoMethod *method,
							ProtoParametersDescription *pars);
	ProtoObject *fromPointer(void *pointer);
	ProtoObject *fromBuffer(char *buffer, int length);
	ProtoObject *fromBoolean(int value);

	int         asInteger();
	double      asDouble();
	void        asLiteral(char *utf8Buffer, int maxLength);
	int         asLiteralLength();
	ProtoMethod *asMethod();
	void        *asPointer();
	char		*asBufferPointer();
	int			*asBufferLength();
	int         *asBoolean();

	void		exitThread(int exitCode);
};

#define PROTO_TRUE  0x00000000000034
#define PROTO_FALSE 0x00000000000024
#define PROTO_NONE  0x0

#endif /* PROTO_H_ */
