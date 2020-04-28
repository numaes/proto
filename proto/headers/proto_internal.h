/*
 * proto_internal.h
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#ifndef PROTO_INTERNAL_H_
#define PROTO_INTERNAL_H_

#include "proto.h"
#include <cstddef>

union ProtoObjectPointer {
	ProtoObject	*oid;
	Cell *cell;
	ProtoMethod methodPointer;

	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
		unsigned long long value:56;
	} op;

	// Pointer tag dependent values
	struct {
		signed long pad:3;
		signed long smallInteger:61;
	} si;
	struct {
		unsigned long pad:3;
		unsigned long smallDouble:61;
	} sd;
	struct {
		unsigned long pad:3;
		unsigned long mutableID:61;
	} mutableObject;

	// Embedded types dependent values
	struct {
		char pad;
		char utfChar[4];
	} utf8Char;
	struct {
		unsigned pad:8;
		unsigned booleanValue:1;
	} booleanValue;
	struct {
		unsigned char pad;
		unsigned char byteData;
	} byteValue;

	struct {
		unsigned long pad:8;
		unsigned long timestamp;
	} timestampValue;

	struct {
		unsigned pad:8;
		unsigned day:8;
		unsigned month:8;
		unsigned year:8;
	} date;

	struct {
		long pad:8;
		long timedelta:56;
	} timedeltaValue;

};

#define TAG_MASK				  0x07
#define TYPE_MASK				  0x0f
#define TYPE_SHIFT				  0x03

#define POINTER_GET_TAG(p)		  (((long) p) & TAG_MASK)
#define POINTER_GET_TYPE(p)	  	  ((((long) p >> TYPE_SHIFT) & TYPE_MASK))	

#define POINTER_TAG_CELL 	      0x00
#define POINTER_TAG_MUTABLEOBJECT 0x01
#define POINTER_TAG_EXTERNAL_POINTER 0x02
#define POINTER_TAG_SMALLINT      0x03
#define POINTER_TAG_SMALLDOUBLE   0x04
#define POINTER_TAG_METHOD        0x05
#define POINTER_TAG_UNASSIGNED_6  0x06
#define POINTER_TAG_EMBEDEDVALUE  0x07

#define EMBEDED_TYPE_BOOLEAN      0x00
#define EMBEDED_TYPE_UTF8CHAR     0x01
#define EMBEDED_TYPE_BYTE         0x02
#define EMBEDED_TYPE_TIMESTAMP    0x03
#define EMBEDED_TYPE_DATE         0x04
#define EMBEDED_TYPE_TIMEDELTA    0x05
#define EMBEDED_TYPE_UNASSIGNED_6 0x06
#define EMBEDED_TYPE_UNASSIGNED_7 0x07
#define EMBEDED_TYPE_UNASSIGNED_8 0x08
#define EMBEDED_TYPE_UNASSIGNED_9 0x09
#define EMBEDED_TYPE_UNASSIGNED_A 0x0A
#define EMBEDED_TYPE_UNASSIGNED_B 0x0B
#define EMBEDED_TYPE_UNASSIGNED_C 0x0C
#define EMBEDED_TYPE_UNASSIGNED_D 0x0D
#define EMBEDED_TYPE_UNASSIGNED_E 0x0E
#define EMBEDED_TYPE_UNASSIGNED_F 0x0F


#define PROTO_TRUE ((ProtoObject *)  0x0107)
#define PROTO_FALSE ((ProtoObject *) 0x0007)
#define PROTO_NULL ((ProtoObject *)  0x00L)

class ProtoSpaceImplementation: ProtoSpace {
private:
	ProtoSpaceImplementation();
	virtual ProtoContext *baseContext();

	ProtoObject	*threads;
	ProtoList	*memorySegments;

public:
	ProtoSpaceImplementation(ProtoSpace *space);
	~ProtoSpaceImplementation();

	ProtoObject 	*getThreads();
};

// Root base of any internal structure.
// All Cell objects should be non mutable once initialized
// They could only change their context and nextCell, assuming that the
// new nextCell includes the previous chain.
// Changing the context or the nextCell are the ONLY changes allowed
// (taking into account the previous restriction).
// Changes should be atomic
//
// Cells should be allways smaller or equal to 64 bytes.
// (size_t, including the vtable pointer)
// Been a power of two size has huge advantages and it opens
// the posibility to extend the model to massive parallel computers
//
// Allocations of Cells will be performed in OS page size chunks
// Page size is allways a power of two and bigger than 64 bytes
// There is no other form of allocation
// 

class Cell {
protected:
	Cell();
public:
	virtual ~Cell();

	void *operator new(size_t size, ProtoContext *context);

    Cell			*nextCell;
    ProtoContext	*context;

	// Apply method recursivelly to all referenced objects, except itself
    virtual void 	processReferences(
		ProtoContext *context, 
		void (*method)(ProtoContext *context, Cell *cell)
	);
 
};

// ParentPointers are chains of parent classes used to solve attribute access
class ParentLink: Cell{
public:
	virtual ~ParentLink();

	ProtoObject *parent;
	ProtoObject *object;
};

// Base tree structure used by Dictionaries, Sets and Lists
// Internal use exclusively

class TreeCell: Cell {
public:
	TreeCell(
		ProtoObject *key, 
		TreeCell *previous = NULL, 
		TreeCell *next = NULL
	);
	virtual ~TreeCell();

	TreeCell	*previous;
	TreeCell	*next;

	unsigned long long count:56;
	unsigned long long height:8;

	ProtoObject *key;

    virtual void 	processReferences(
		ProtoContext *context, 
		void (*method)(ProtoContext *context, Cell *cell)
	);

	virtual BOOLEAN 	has(ProtoContext *context, ProtoObject *key);

	virtual TreeCell    *clone(ProtoContext *context);
};

class ProtoDictCell: TreeCell {
public:
	ProtoObject *value;

	virtual ProtoObject	*treeHas(ProtoObject *key);

	virtual TreeCell    *clone();

	ProtoObject      *getHash(ProtoContext *context);

	ProtoDictCell    *getAt(ProtoContext *context, ProtoObject *key);
	ProtoDictCell    *setAt(ProtoContext *context, ProtoObject *index);

	ProtoDictCell    *removeAt(ProtoContext *context, ProtoObject *index);
	ProtoDictCell    *removeFirst(ProtoContext *context);
	ProtoDictCell    *removeLast(ProtoContext *context);

	ProtoDictCell    *getFirst(ProtoContext *context);
	ProtoDictCell    *getLast(ProtoContext *context);
	ProtoDictCell	 *getSlice(ProtoContext *context);
	unsigned long long getSize(ProtoContext *context);

	void			 *processKeys(ProtoContext *context, ProtoMethod *method);
	void			 *processValues(ProtoContext *context, ProtoMethod *method);
};

class ProtoDict: Cell {
public:
	ProtoDictCell	*root;
	ProtoObject		*hash;

	ProtoObject  	*getHash(ProtoContext *context);
	ProtoDict    	*getFirst(ProtoContext *context);
	ProtoDict    	*getLast(ProtoContext *context);

	ProtoDict    	*getAt(ProtoContext *context, ProtoObject *key);
	ProtoDict    	*setAt(ProtoContext *context, ProtoObject *key, ProtoObject *value);
	int				cmp(ProtoContext *context, ProtoDict *other);

	ProtoDict    	*removeAt(ProtoContext *context, ProtoObject *index);
	ProtoDict    	*removeFirst(ProtoContext *context);
	ProtoDict    	*removeLast(ProtoContext *context);

	void			*processKeys(ProtoContext *context, ProtoMethod *method);
	void			*processValues(ProtoContext *context, ProtoMethod *method);
	unsigned long long getSize(ProtoContext *context);

	ProtoList	 	*getItems(ProtoContext *context);
	ProtoList	 	*getKeys(ProtoContext *context);
	ProtoList	 	*getValues(ProtoContext *context);
};

class ProtoListCell: TreeCell {
public:
	ProtoListCell   *getAt(ProtoContext *context, ProtoObject *index, ProtoObject *value);
	ProtoListCell   *getFirst(ProtoContext *context);
	ProtoListCell   *getLast(ProtoContext *context);
	ProtoListCell   *getKeys(ProtoContext *context);
	ProtoListCell	*getSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);
	unsigned long long getSize(ProtoContext *context);

	ProtoObject		*has(ProtoContext *context, ProtoObject* value);
	ProtoListCell 	*setAt(ProtoContext *context, ProtoObject *index, ProtoObject* value);
	ProtoListCell 	*removeAt(ProtoContext *context, ProtoObject *index);

	ProtoListCell  	*appendFirst(ProtoContext *context, ProtoObject* value);
	ProtoListCell  	*appendLast(ProtoContext *context, ProtoObject* value);

	ProtoListCell   *removeAt(ProtoContext *context, ProtoObject *index);
	ProtoListCell	*removeFirst(ProtoContext *context);
	ProtoListCell	*removeLast(ProtoContext *context);
	ProtoListCell	*removeAt(ProtoContext *context, ProtoObject* index);
	ProtoListCell  	*removeSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);
};

class ProtoList: Cell {
public:
	ProtoListCell	*root;
	ProtoObject		*hash;

	ProtoObject  	*getHash(ProtoContext *context);
	ProtoList		*getAt(ProtoContext *context, ProtoObject *key);
	ProtoList		*getAt(ProtoContext *context, ProtoObject *index, ProtoObject *value);
	ProtoList		*getFirst(ProtoContext *context);
	ProtoList		*getLast(ProtoContext *context);
	ProtoList		*getKeys(ProtoContext *context);
	ProtoList		*getSize(ProtoContext *context);
	ProtoList		*getSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);

	int				cmp(ProtoContext *context, ProtoList *other);

	ProtoList		*removeAt(ProtoContext *context, ProtoObject *index);
	ProtoList		*removeFirst(ProtoContext *context);
	ProtoList		*removeLast(ProtoContext *context);
	ProtoList		*removeAt(ProtoContext *context, ProtoObject *index);
	ProtoList		*removeSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);

	ProtoObject		*has(ProtoContext *context, ProtoObject* value);

	ProtoList		*appendFirst(ProtoContext *context, ProtoObject* value);
	ProtoList		*appendLast(ProtoContext *context, ProtoObject* value);
};

class ProtoSetCell: TreeCell {
public:
	ProtoSetCell    *removeAt(ProtoContext *context, ProtoObject *value);
	ProtoObject		*setHas(ProtoContext *context, ProtoObject* value);
	ProtoObject		*add(ProtoContext *context, ProtoObject* value);

	unsigned long long getSize(ProtoContext *context);

	ProtoList		*asList(ProtoContext *context);
	ProtoSetCell	*getUnion(ProtoContext *context, ProtoSetCell* otherSet);
	ProtoSetCell	*getIntersection(ProtoContext *context, ProtoSetCell* otherSet);
	ProtoSetCell	*getPlus(ProtoContext *context, ProtoSetCell* otherSet);
	ProtoSetCell	*getLess(ProtoContext *context, ProtoSetCell* otherSet);
};

class ProtoSet: Cell {
public:
	ProtoSetCell	*root;
	ProtoObject		*hash;

	ProtoObject  	*getHash(ProtoContext *context);
	ProtoSet		*add(ProtoContext *context, ProtoObject* value);
	ProtoObject		*has(ProtoContext *context, ProtoObject* value);
	int				cmp(ProtoContext *context, ProtoSet *other);

	ProtoObject 	*getSize(ProtoContext *context);
	ProtoList		*asList(ProtoContext *context);
	ProtoSet		*getUnion(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet		*getIntersection(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet		*getPlus(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet		*getLess(ProtoContext *context, ProtoSet* otherSet);
};

class ProtoMemoryBuffer: Cell {
public:
	char		*buffer;
	unsigned long long size;

	char 		*getBuffer();
	unsigned long long getSize();
};

class ObjectCell: Cell {
public:
	ParentLink     *parent;
	ProtoDictCell  *currentValue;
};

class ContextCell: Cell {
private:
	Cell		 *firstCell;
	ProtoContext *previous;
	ProtoSpace   *space;
	int			 osThreadId;

	ProtoThread	 *getThread();
};

class ProtoThread: Cell {
public:
	virtual ~ProtoThread();

	ProtoObject			*name;

	virtual void		run(ProtoContext *context, ProtoObject *parameter, ProtoMethod *code);
	virtual ProtoObject	*end(ProtoContext *context, ProtoObject *exitCode);
	virtual ProtoObject *join(ProtoContext *context, ProtoThread *threadToJoin);
	virtual void        kill(ProtoContext *context);

	virtual void		suspend(long ms);
	virtual long	    getCPUTimestamp();

	Cell			    *allocCell();
};

#endif /* PROTO_INTERNAL_H_ */
