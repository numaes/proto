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
#include <atomic>

union ProtoObjectPointer {
	ProtoObjectPointer() {};

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
		unsigned long long pointer_tag:3;
		signed long smallInteger:61;
	} si;
	struct {
		unsigned long long pointer_tag:3;
		unsigned long smallDouble:61;
	} sd;
	struct {
		unsigned long long pointer_tag:3;
		unsigned long mutableID:61;
	} mutableObject;

	// Embedded types dependent values
	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
		unsigned int unicodeValue:32;
	} unicodeChar;
	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
		unsigned booleanValue:1;
	} booleanValue;
	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
		unsigned char byteData;
	} byteValue;

	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
		unsigned long timestamp;
	} timestampValue;

	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
		unsigned day:8;
		unsigned month:8;
		unsigned year:8;
	} date;

	struct {
		unsigned long long pointer_tag:3;
		unsigned long long embedded_type:5;
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
#define POINTER_TAG_BYTE_BUFFER   0x06
#define POINTER_TAG_EMBEDEDVALUE  0x07

#define EMBEDED_TYPE_BOOLEAN      0x00
#define EMBEDED_TYPE_UNICODECHAR  0x01
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
public:
	Cell();
	virtual ~Cell();

	void *operator new(size_t size, ProtoContext *context);

	// Apply method recursivelly to all referenced objects, except itself
    virtual void 	processReferences(
		ProtoContext *context, 
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);

	virtual ProtoObject *getHash(ProtoContext *context);
	virtual Cell		*clone();

    Cell				*nextCell;

};

// ParentPointers are chains of parent classes used to solve attribute access
class ParentLink: Cell, ProtoObject {
protected:
	ProtoObject *parent;
	ProtoObject *object;

public:
	ParentLink(
		ProtoObject *parent=NULL,
		ProtoObject *object=NULL
	);

	virtual ~ParentLink();

};

// Base tree structure used by Dictionaries, Sets and Lists
// Internal use exclusively

class TreeCell: public Cell {
protected:
    ProtoObject		*hash;
	TreeCell	*previous;
	TreeCell	*next;

	unsigned long long count:56;
	unsigned long long height:8;

	ProtoObject *key;

public:
	TreeCell(
		ProtoObject *hash,
		TreeCell *previous=NULL,
		TreeCell *next=NULL,
		unsigned long long count=0,
		unsigned long long height=0,
		ProtoObject *key=NULL
	);

	virtual ~TreeCell();

	virtual TreeCell    *clone(ProtoContext *context);

};

class IdentityDict: public TreeCell, public ProtoObject {
protected:
	ProtoObject *value;

public:
	IdentityDict(
		ProtoObject *hash=NULL,
		TreeCell *previous=NULL,
		TreeCell *next=NULL,
		unsigned long long count=0,
		unsigned long long height=0,
		ProtoObject *key=NULL,
		ProtoObject *value=NULL
	);
	virtual ~IdentityDict();

	virtual ProtoObject	    *treeHas(ProtoObject *key);

	virtual ProtoObject     *getAt(ProtoContext *context, ProtoObject *key);
	virtual IdentityDict    *setAt(ProtoContext *context, ProtoObject *key, ProtoObject *value);
	virtual IdentityDict    *removeAt(ProtoContext *context, ProtoObject *index);
	virtual BOOLEAN 		has(ProtoContext *context, ProtoObject *key);
	virtual int				compareWith(ProtoContext *context, IdentityDict *otherDict);

	virtual unsigned long long getSize(ProtoContext *context);

	void processElements (
		ProtoContext *context,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
	void processKeys (
		ProtoContext *context,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
	void processValues (
		ProtoContext *context,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
};

class ProtoList: public TreeCell, public ProtoObject {
public:
	ProtoList(
		ProtoObject *hash=NULL,
		TreeCell *previous=NULL,
		TreeCell *next=NULL,
		unsigned long long count=0,
		unsigned long long height=0,
		ProtoObject *key=NULL
	);
	virtual ~ProtoList();
	
	ProtoList   *getAt(ProtoContext *context, ProtoObject *index);
	ProtoList   *getFirst(ProtoContext *context);
	ProtoList   *getLast(ProtoContext *context);
	ProtoList   *getKeys(ProtoContext *context);
	ProtoList	*getSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);
	unsigned long long getSize(ProtoContext *context);

	ProtoObject	*has(ProtoContext *context, ProtoObject* value);
	ProtoList 	*setAt(ProtoContext *context, ProtoObject *index, ProtoObject* value);
	ProtoList 	*removeAt(ProtoContext *context, ProtoObject *index);

	ProtoList  	*appendFirst(ProtoContext *context, ProtoObject* value);
	ProtoList  	*appendLast(ProtoContext *context, ProtoObject* value);

	ProtoList   *removeAt(ProtoContext *context, ProtoObject *index);
	ProtoList	*removeFirst(ProtoContext *context);
	ProtoList	*removeLast(ProtoContext *context);
	ProtoList	*removeAt(ProtoContext *context, ProtoObject* index);
	ProtoList  	*removeSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);

	void		processElements(
		ProtoContext *context,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);

};

class ProtoSet: public TreeCell, public ProtoObject {
public:
	ProtoSet(
		ProtoObject *hash=NULL,
		TreeCell *previous=NULL,
		TreeCell *next=NULL,
		unsigned long long count=0,
		unsigned long long height=0,
		ProtoObject *key=NULL
	);
	virtual ~ProtoSet();

	ProtoSet        *removeAt(ProtoContext *context, ProtoObject *value);
	ProtoObject		*has(ProtoContext *context, ProtoObject* value);
	ProtoObject		*add(ProtoContext *context, ProtoObject* value);
	int				cmp(ProtoContext *context, ProtoSet *other);

	unsigned long long getSize(ProtoContext *context);

	ProtoList		*asList(ProtoContext *context);
	ProtoSet    	*getUnion(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getIntersection(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getPlus(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getLess(ProtoContext *context, ProtoSet* otherSet);

	void			processElements(
		ProtoContext *context,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
};

class ProtoByteBuffer: public Cell, public ProtoObject {
protected:
	char		*buffer;
	unsigned long  size;

public:
	ProtoByteBuffer(
		char		*buffer=NULL,
		unsigned long size=0
	);
	virtual ~ProtoByteBuffer();

	char 				*getBuffer();
	unsigned long 	    getSize();
};

class ProtoObjectCell: public Cell, public ProtoObject {
protected:
	ParentLink	*parent;
	IdentityDict *value;

public:
	ProtoObjectCell(
		ParentLink	*parent=NULL,
		IdentityDict *value=NULL
	);
	virtual ~ProtoObjectCell();

	void processElements (
		ProtoContext *context,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
};

class ProtoContextImplementation: public ProtoContext {
public:
	ProtoContextImplementation(
		ProtoContextImplementation *previous = NULL,
		ProtoSpace *space = NULL,
		ProtoThread *thread = NULL
	);

	~ProtoContextImplementation();

	ProtoContextImplementation	*previous;
	ProtoSpaceImplementation  	*space;
	ProtoThread					*thread;
	Cell						*returnChain;
	ProtoSet					*returnSet;
	Cell						*lastCellPreviousContext;
};

class ProtoThread: public Cell, public ProtoObject {
public:
	ProtoThread(
		ProtoObject *name=NULL,
		Cell		*currentWorkingSet=NULL,
		Cell		*freeCells=NULL,
		ProtoSpaceImplementation	*space=NULL
	);
	virtual ~ProtoThread();

	virtual void		run(ProtoContext *context=NULL, ProtoObject *parameter=NULL, ProtoMethod *code=NULL);
	virtual ProtoObject	*end(ProtoContext *context, ProtoObject *exitCode);
	virtual ProtoObject *join(ProtoContext *context);
	virtual void        kill(ProtoContext *context);

	virtual void		suspend(long ms);
	virtual long	    getCPUTimestamp();

	virtual Cell	    *allocCell();
	virtual Cell		*getLastAllocatedCell();
	virtual void		setLastAllocatedCell(Cell *someCell);

	ProtoObject			*name;
	Cell				*currentWorkingSet;
	int					osThreadId;
	ProtoSpaceImplementation *space;
	Cell				*freeCells;
};

class AllocatedSegment {
public:
    Cell    *memoryBlock;
    int     cellsCount;
    AllocatedSegment *nextBlock;
};

class DirtySegment {
public:
	Cell	*cellChain;
	DirtySegment *nextSegment;
};

class ProtoSpaceImplementation : public ProtoSpace {
protected:
	AllocatedSegment *segments;
	int blocksInCurrentSegment;
	DirtySegment *dirtySegments;

public:
	ProtoSpaceImplementation();
	~ProtoSpaceImplementation();

	Cell 		*getFreeCells();
	void		analyzeUsedCells(Cell *cellChain);
	void		deallocMemory();

	ProtoSet	*threads;
	AllocatedSegment *segments;
	std::atomic<IdentityDict *> mutableRoot;
};


// Used just to compute the number of bytes needed for a Cell at allocation time

union BigCell {
	Cell	cell;
	ParentLink parentLink;
	IdentityDict identityLink;
	ProtoList protoList;
	ProtoSet protoSet;
	ProtoByteBuffer protoMemoryBuffer;
	ProtoObjectCell protoObjectCell;
	ProtoThread thread;
};

// Global literal dictionary
class LiteralDictionary {
public:
	ProtoList	*getFromZeroTerminatedString(ProtoContext *context, const char *name);
	ProtoList	*getFromString(ProtoContext *context, ProtoObject *string);
};

std::atomic<LiteralDictionary *> literalRoot;


#endif /* PROTO_INTERNAL_H_ */
