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
	ProtoObject	*oid;
	Cell *cell;

	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
		unsigned long value:56;
	} op;

	// Pointer tag dependent values
	struct {
		unsigned long pointer_tag:3;
		signed long smallInteger:61;
	} si;
	struct {
		unsigned long pointer_tag:3;
		unsigned long smallDouble:61;
	} sd;
	struct {
		unsigned long pointer_tag:3;
		unsigned long mutableID:61;
	} mutableObject;

	// Embedded types dependent values
	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
		unsigned int unicodeValue:32;
	} unicodeChar;
	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
		unsigned booleanValue:1;
	} booleanValue;
	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
		unsigned char byteData;
	} byteValue;

	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
		unsigned long timestamp;
	} timestampValue;

	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
		unsigned day:8;
		unsigned month:8;
		unsigned year:8;
	} date;

	struct {
		unsigned long pointer_tag:3;
		unsigned long embedded_type:5;
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
#define POINTER_TAG_SMALLINT      0x02
#define POINTER_TAG_SMALLDOUBLE   0x03
#define POINTER_TAG_EMBEDEDVALUE  0x04
#define POINTER_TAG_UNASSIGNED_5  0x05
#define POINTER_TAG_UNASSIGNED_6  0x06
#define POINTER_TAG_UNASSIGNED_7  0x07

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

#define CELL_TYPE_PARENT_LINK	  0x00
#define CELL_TYPE_IDENTITY_DICT   0x01
#define CELL_TYPE_PROTO_LIST	  0x02
#define CELL_TYPE_PROTO_SET		  0x03
#define CELL_TYPE_BYTE_BUFFER     0x04
#define CELL_TYPE_PROTO_THREAD    0x05
#define CELL_TYPE_PROTO_OBJECT    0x06
#define CELL_TYPE_EXTERNAL_POINTER 0x07
#define CELL_TYPE_METHOD	      0x08
#define CELL_TYPE_UNASSIGNED_9    0x09
#define CELL_TYPE_UNASSIGNED_A    0x0A
#define CELL_TYPE_UNASSIGNED_B    0x0B
#define CELL_TYPE_UNASSIGNED_C    0x0C
#define CELL_TYPE_UNASSIGNED_D    0x0D
#define CELL_TYPE_UNASSIGNED_E    0x0E
#define CELL_TYPE_UNASSIGNED_F    0x0F


#define PROTO_TRUE ((ProtoObject *)  0x0107)
#define PROTO_FALSE ((ProtoObject *) 0x0007)
#define PROTO_NULL ((ProtoObject *)  0x00L)

class ProtoSpaceImplementation: ProtoSpace {
private:
	ProtoSpaceImplementation();
	 ProtoContext *baseContext();

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
// Cells should be always smaller or equal to 64 bytes.
// Been a power of two size has huge advantages and it opens
// the posibility to extend the model to massive parallel computers
//
// Allocations of Cells will be performed in OS page size chunks
// Page size is allways a power of two and bigger than 64 bytes
// There is no other form of allocation for proto objects or scalars
// 

class Cell {
public:
	Cell(
		ProtoContext *context, 
		Cell *nextCell = NULL,
		unsigned long type = CELL_TYPE_UNASSIGNED_F,
		unsigned long height = 0,
		unsigned long count = 0
	);
	~Cell();

	void *operator new(size_t size, ProtoContext *context);

	// Apply method recursivelly to all referenced objects, except itself
    void 	processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);

    Cell				*nextCell;
	ProtoContext		*context;

	unsigned long count:52;
	unsigned long height:8;
	unsigned long type:4;
};

// ParentPointers are chains of parent classes used to solve attribute access
class ParentLink: public Cell {
protected:
	ProtoObject *parent;
	ProtoObject *object;

public:
	ParentLink(
		ProtoContext *context,

		Cell *nextCell = NULL,
		ProtoObject *parent = PROTO_NONE,
		ProtoObject *object = PROTO_NONE
	);

	~ParentLink();

	// Apply method recursivelly to all referenced objects, except itself
    void 	processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);
};

// Base tree structure used by Dictionaries, Sets and Lists
// Internal use exclusively

class TreeCell: public Cell {
protected:
    ProtoObject		*hash;
	TreeCell		*previous;
	TreeCell		*next;

	ProtoObject 	*key;

public:
	TreeCell(
		ProtoContext *context,

		Cell *nextCell = NULL,
		ProtoObject *hash = PROTO_NONE,
		TreeCell *previous = NULL,
		TreeCell *next = NULL,
		unsigned long count = 0,
		unsigned long height = 0,
		ProtoObject *key = PROTO_NONE
	);

	~TreeCell();
};

class IdentityDict: public TreeCell, public ProtoObject {
protected:
	ProtoObject *value;

public:
	IdentityDict(
		ProtoContext *context,

		ProtoObject *hash = PROTO_NONE,
		TreeCell *previous = NULL,
		TreeCell *next = NULL,
		unsigned long count = 0,
		unsigned long height = 0,
		ProtoObject *key = PROTO_NONE,
		ProtoObject *value = PROTO_NONE,

		Cell *nextCell = NULL
	);
	~IdentityDict();

	IdentityDict    *clone(ProtoContext *context);

	ProtoObject	    *treeHas(ProtoObject *key);

	ProtoObject     *getAt(ProtoContext *context, ProtoObject *key);
	IdentityDict    *setAt(ProtoContext *context, ProtoObject *key, ProtoObject *value);
	IdentityDict    *removeAt(ProtoContext *context, ProtoObject *index);
	BOOLEAN 		has(ProtoContext *context, ProtoObject *key);
	int				compareWith(ProtoContext *context, IdentityDict *otherDict);

	unsigned long getSize(ProtoContext *context);

	void processReferences (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	void processElements (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
	void processKeys (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *element
		)
	);
	void processValues (
		ProtoContext *context,
		void *self,
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
		ProtoContext *context,

		ProtoObject *hash = PROTO_NONE,
		TreeCell *previous = NULL,
		TreeCell *next = NULL,
		unsigned long count = 0,
		unsigned long height = 0,
		ProtoObject *key = PROTO_NONE,

		Cell *nextCell = NULL
	);
	~ProtoList();

	ProtoList	*clone(ProtoContext *context);
	
	ProtoList   *getAt(ProtoContext *context, ProtoObject *index);
	ProtoList   *getFirst(ProtoContext *context);
	ProtoList   *getLast(ProtoContext *context);
	ProtoList   *getKeys(ProtoContext *context);
	ProtoList	*getSlice(ProtoContext *context, ProtoObject *from, ProtoObject *to);
	unsigned long getSize(ProtoContext *context);

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

	void		processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

};

class ProtoSet: public TreeCell, public ProtoObject {
public:
	ProtoSet(
		ProtoContext *context,

		ProtoObject *hash = PROTO_NONE,
		TreeCell *previous = NULL,
		TreeCell *next = NULL,
		unsigned long count = 0,
		unsigned long height = 0,
		ProtoObject *key = PROTO_NONE,

		Cell *nextCell = NULL
	);
	~ProtoSet();

	ProtoSet		*clone(ProtoContext *context);
	ProtoSet        *removeAt(ProtoContext *context, ProtoObject *value);
	ProtoObject		*has(ProtoContext *context, ProtoObject* value);
	ProtoSet		*add(ProtoContext *context, ProtoObject* value);
	int				cmp(ProtoContext *context, ProtoSet *other);

	unsigned long getSize(ProtoContext *context);

	ProtoList		*asList(ProtoContext *context);
	ProtoSet    	*getUnion(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getIntersection(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getPlus(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getLess(ProtoContext *context, ProtoSet* otherSet);

	void			processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);
};

class ProtoByteBuffer: public Cell, public ProtoObject {
protected:
	char		*buffer;
	unsigned long  size;

public:
	ProtoByteBuffer(
		ProtoContext *context,
		char		*buffer,
		unsigned long size,

		Cell *nextCell = NULL
	);
	~ProtoByteBuffer();

	void			processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	char 				*getBuffer();
	unsigned long 	    getSize();
};

class ProtoExternalPointer: public Cell, public ProtoObject {
protected:
	void *pointer;

public:
	ProtoExternalPointer(
		ProtoContext *context,
		void		*pointer,

		Cell *nextCell = NULL
	);
	~ProtoExternalPointer();

	void			processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	void 				*getPointer();
};

class ProtoMethodCell: public Cell, public ProtoObject {
protected:
	ProtoMethod	*method;

public:
	ProtoMethodCell(
		ProtoContext *context,
		ProtoMethod	 *method,

		Cell *nextCell = NULL
	);
	~ProtoMethodCell();

	void			processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoMethod	*getMethod();
};


class ProtoObjectCell: public Cell, public ProtoObject {
protected:
	ParentLink	*parent;
	IdentityDict *attributes;

public:
	ProtoObjectCell(
		ProtoContext *context,
		ParentLink	*parent = NULL,
		IdentityDict *attributes = NULL
	);
	 ~ProtoObjectCell();

	// Apply method recursivelly to all referenced objects, except itself
    void 	processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
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
		ProtoContext *context,

		ProtoObject *name=NULL,
		Cell		*currentWorkingSet=NULL,
		Cell		*freeCells=NULL,
		ProtoSpaceImplementation	*space=NULL
	);
	 ~ProtoThread();

	void		run(ProtoContext *context=NULL, ProtoObject *parameter=NULL, ProtoMethod *code=NULL);
	ProtoObject	*end(ProtoContext *context, ProtoObject *exitCode);
	ProtoObject *join(ProtoContext *context);
	void        kill(ProtoContext *context);

	void		suspend(long ms);
	long	    getCPUTimestamp();

	// Apply method recursivelly to all referenced objects, except itself
    void 	processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);

	Cell	    *allocCell();
	Cell		*getLastAllocatedCell();
	void		setLastAllocatedCell(Cell *someCell);

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

static_assert (sizeof(IdentityDict) == 64);

// Global literal dictionary
class LiteralDictionary {
public:
	ProtoList	*getFromZeroTerminatedString(ProtoContext *context, const char *name);
	ProtoList	*getFromString(ProtoContext *context, ProtoObject *string);
};

std::atomic<LiteralDictionary *> literalRoot;


#endif /* PROTO_INTERNAL_H_ */
