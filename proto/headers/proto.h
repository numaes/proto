/*
 * proto.h
 *
 *  Created on: November, 2017
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */

#include <cstddef>
#include <atomic>
#include <thread>

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
#define NULL (void *) 0LU
#endif

#define PROTO_NONE ((ProtoObject *) NULL)

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

#define TAG_MASK				  (0x07LU)
#define TYPE_MASK				  (0x0fLU)
#define TYPE_SHIFT				  (0x03LU)

#define POINTER_GET_TAG(p)		  (((unsigned long) p) & TAG_MASK)
#define POINTER_GET_TYPE(p)	  	  ((((unsigned long) p >> TYPE_SHIFT) & TYPE_MASK))	

#define POINTER_TAG_CELL 	      (0x00LU)
#define POINTER_TAG_MUTABLEOBJECT (0x01LU)
#define POINTER_TAG_SMALLINT      (0x02LU)
#define POINTER_TAG_SMALLDOUBLE   (0x03LU)
#define POINTER_TAG_EMBEDEDVALUE  (0x04LU)
#define POINTER_TAG_UNASSIGNED_5  (0x05LU)
#define POINTER_TAG_UNASSIGNED_6  (0x06LU)
#define POINTER_TAG_UNASSIGNED_7  (0x07LU)

#define EMBEDED_TYPE_BOOLEAN      (0x00LU)
#define EMBEDED_TYPE_UNICODECHAR  (0x01LU)
#define EMBEDED_TYPE_BYTE         (0x02LU)
#define EMBEDED_TYPE_TIMESTAMP    (0x03LU)
#define EMBEDED_TYPE_DATE         (0x04LU)
#define EMBEDED_TYPE_TIMEDELTA    (0x05LU)
#define EMBEDED_TYPE_UNASSIGNED_6 (0x06LU)
#define EMBEDED_TYPE_UNASSIGNED_7 (0x07LU)
#define EMBEDED_TYPE_UNASSIGNED_8 (0x08LU)
#define EMBEDED_TYPE_UNASSIGNED_9 (0x09LU)
#define EMBEDED_TYPE_UNASSIGNED_A (0x0ALU)
#define EMBEDED_TYPE_UNASSIGNED_B (0x0BLU)
#define EMBEDED_TYPE_UNASSIGNED_C (0x0CLU)
#define EMBEDED_TYPE_UNASSIGNED_D (0x0DLU)
#define EMBEDED_TYPE_UNASSIGNED_E (0x0ELU)
#define EMBEDED_TYPE_UNASSIGNED_F (0x0FLU)

#define CELL_TYPE_UNASSIGNED	  	(0x00LU)
#define CELL_TYPE_IDENTITY_DICT   	(0x01LU)
#define CELL_TYPE_PROTO_LIST	  	(0x02LU)
#define CELL_TYPE_PROTO_SET		  	(0x03LU)
#define CELL_TYPE_BYTE_BUFFER     	(0x04LU)
#define CELL_TYPE_PROTO_THREAD    	(0x05LU)
#define CELL_TYPE_PROTO_OBJECT    	(0x06LU)
#define CELL_TYPE_EXTERNAL_POINTER 	(0x07LU)
#define CELL_TYPE_METHOD	      	(0x08LU)
#define CELL_TYPE_PARENT_LINK     	(0x09LU)
#define CELL_TYPE_LITERAL_DICT    	(0x0ALU)
#define CELL_TYPE_UNASSIGNED_B    	(0x0BLU)
#define CELL_TYPE_UNASSIGNED_C    	(0x0CLU)
#define CELL_TYPE_UNASSIGNED_D    	(0x0DLU)
#define CELL_TYPE_UNASSIGNED_E    	(0x0ELU)
#define CELL_TYPE_UNASSIGNED_F    	(0x0FLU)

class ProtoContext;
class ProtoObject;
class ProtoThread;
class ProtoObjectCell;
class ProtoSpace;
class Cell;

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

class ProtoSpace {
public:
	ProtoSpace();
	virtual ~ProtoSpace();

	ProtoObject *getThreads();

	ProtoObject	*objectPrototype;
	ProtoObject *integerPrototype;
	ProtoObject *charPrototype;
	ProtoObject *bytePrototype;
	ProtoObject *nonePrototype;
	ProtoObject *methodPrototype;
	ProtoObject *bufferPrototype;
	ProtoObject *pointerPrototype;
	ProtoObject *booleanPrototype;
	ProtoObject *doublePrototype;
	ProtoObject *datePrototype;
	ProtoObject *timestampPrototype;
	ProtoObject *timedeltaPrototype;
	ProtoObject *identityDictPrototype;
	ProtoObject *protoSetPrototype;
	ProtoObject *protoListPrototype;
	ProtoObject *threadPrototype;

	ProtoObject *rootObject;

	void allocThread(ProtoContext *context, ProtoThread *thread);
	void deallocThread(ProtoContext *context, ProtoThread *thread);

	Cell 		*getFreeCells();
	void 		analyzeUsedCells(Cell *cellsChain);
	void 		deallocMemory();

	ProtoObject			*threads;
	std::atomic<Cell *>  mutableRoot;
	std::atomic<BOOLEAN> mutableLock;
	std::atomic<BOOLEAN> threadsLock;
	int					 blocksInCurrentSegment;
	AllocatedSegment 	*segments;
	DirtySegment		*freeSegments;
	DirtySegment 		*dirtySegments;
	std::atomic<BOOLEAN> gcLock;
	int					 state;
	std::thread::id		 mainThreadId;
	std::thread			*gcThread;
};

typedef struct {
	ProtoObject *keyword;
	ProtoObject *value;
} KeywordParameter;

typedef ProtoObject *(*ProtoMethod)(
	ProtoContext *, 		// context
	ProtoObject *, 			// self
	ProtoObject *, 			// type
	ProtoObject *, 			// positionalParameters
	ProtoObject * 			// keywordParameters
);

class ProtoObject {
public:
	ProtoObject *clone(ProtoContext *c);
	ProtoObject *newChild(ProtoContext *c);

	ProtoObject *getType(ProtoContext *c);
	ProtoObject *getAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *hasAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *hasOwnAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *setAttribute(ProtoContext *c, ProtoObject *name, ProtoObject *value);

	ProtoObject *getAttributes(ProtoContext *c);
	ProtoObject *getOwnAttributes(ProtoContext *c);
	ProtoObject *getParent(ProtoContext *c);

	ProtoObject *addParent(ProtoContext *c, ProtoObject *newParent);
	ProtoObject *isInstanceOf(ProtoContext *c, ProtoObject *prototype);

	ProtoObject *call(ProtoContext *c,
					  ProtoObject *method,
					  ProtoObject *unnamedParametersList,
			          ProtoObject *keywordParametersDict);

	ProtoObject		*currentValue(ProtoContext *context);
	BOOLEAN			 setValue(ProtoContext *context, ProtoObject *currentValue, ProtoObject *value);
};

class Cell {
public:
	Cell(
		ProtoContext *context, 
		long unsigned type = CELL_TYPE_UNASSIGNED,
		long unsigned height = 0LU,
		long unsigned count = 0LU
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

// Base tree structure used by Dictionaries, Sets and Lists
// Internal use exclusively

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

// ParentPointers are chains of parent classes used to solve attribute access
class ParentLink: public Cell {
protected:
public:
	ParentLink(
		ProtoContext *context,

		ParentLink *parent,
		ProtoObjectCell *object
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

	ParentLink      *parent;
	ProtoObjectCell *object;
};

class IdentityDict: public Cell, public ProtoObject {
public:
	IdentityDict(
		ProtoContext *context,

		ProtoObject *key = PROTO_NONE,
		ProtoObject *value = PROTO_NONE,
		IdentityDict *previous = NULL,
		IdentityDict *next = NULL
	);
	~IdentityDict();

	BOOLEAN 		has(ProtoContext *context, ProtoObject *key);
	ProtoObject     *getAt(ProtoContext *context, ProtoObject *key);
	IdentityDict    *setAt(ProtoContext *context, ProtoObject *key, ProtoObject *value);
	IdentityDict    *removeAt(ProtoContext *context, ProtoObject *key);
	int				isEqual(ProtoContext *context, IdentityDict *otherDict);

	unsigned long 	getSize(ProtoContext *context);

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
			ProtoObject *key,
			ProtoObject *value
		)
	);
	void processKeys (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *key
		)
	);
	void processValues (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *value
		)
	);

    ProtoObject		*hash;
	IdentityDict	*previous;
	IdentityDict	*next;

	ProtoObject 	*key;
	ProtoObject 	*value;
};

class ProtoList: public Cell, public ProtoObject {
public:
	ProtoList(
		ProtoContext *context,

		ProtoObject *value = PROTO_NONE,
		ProtoList *previous = NULL,
		ProtoList *next = NULL
	);
	~ProtoList();

	ProtoObject   *getAt(ProtoContext *context, int index);
	ProtoObject   *getFirst(ProtoContext *context);
	ProtoObject   *getLast(ProtoContext *context);
	ProtoList	  *getSlice(ProtoContext *context, int from, int to);
	unsigned long  getSize(ProtoContext *context);

	BOOLEAN		   has(ProtoContext *context, ProtoObject* value);
	ProtoList     *setAt(ProtoContext *context, int index, ProtoObject* value);
	ProtoList     *insertAt(ProtoContext *context, int index, ProtoObject* value);

	ProtoList  	  *appendFirst(ProtoContext *context, ProtoObject* value);
	ProtoList  	  *appendLast(ProtoContext *context, ProtoObject* value);

	ProtoList	  *removeFirst(ProtoContext *context);
	ProtoList	  *removeLast(ProtoContext *context);
	ProtoList	  *removeAt(ProtoContext *context, int index);
	ProtoList  	  *removeSlice(ProtoContext *context, int from, int to);

	void		processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

    ProtoObject		*hash;
	ProtoList		*previous;
	ProtoList		*next;

	ProtoObject 	*value;
};

class ProtoSet: public Cell, public ProtoObject {
public:
	ProtoSet(
		ProtoContext *context,

		ProtoObject *value = PROTO_NONE,
		ProtoSet *previous = NULL,
		ProtoSet *next = NULL
	);
	~ProtoSet();

	ProtoSet        *removeAt(ProtoContext *context, ProtoObject *value);
	BOOLEAN			has(ProtoContext *context, ProtoObject* value);
	ProtoSet		*add(ProtoContext *context, ProtoObject* value);
	int				isEqual(ProtoContext *context, ProtoSet *other);

	unsigned long    getSize(ProtoContext *context);

	ProtoList		*asList(ProtoContext *context);
	ProtoSet    	*getUnion(ProtoContext *context, ProtoSet* otherSet);
	ProtoSet    	*getIntersection(ProtoContext *context, ProtoSet* otherSet);
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

	void processValues (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *value
		)
	);

    ProtoObject		*hash;
	ProtoSet 		*previous;
	ProtoSet		*next;
	ProtoObject 	*value;
};

class ProtoByteBuffer: public Cell, public ProtoObject {
public:
	ProtoByteBuffer(
		ProtoContext *context,
		unsigned long size
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

	char		*buffer;
	unsigned long  size;
};

class ProtoExternalPointer: public Cell, public ProtoObject {
public:
	ProtoExternalPointer(
		ProtoContext *context,
		void		*pointer
	);
	~ProtoExternalPointer();

	void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	void *pointer;
};

class ProtoMethodCell: public Cell, public ProtoObject {
public:
	ProtoMethodCell(
		ProtoContext *context,
		ProtoObject  *self,
		ProtoMethod	 *method
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

	ProtoMethod	*method;
	ProtoObject *self;
};


class ProtoObjectCell: public Cell, public ProtoObject {
public:
	ProtoObjectCell(
		ProtoContext *context,
		ParentLink	*parent = NULL,
		IdentityDict *attributes = NULL
	);
	 ~ProtoObjectCell();

	ProtoObject *addParent(ProtoContext *context, ProtoObject *object);

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

	ParentLink	*parent;
	IdentityDict *attributes;
};

class ProtoThread: public Cell, public ProtoObject {
public:
	ProtoThread(
		ProtoContext *context,

		ProtoObject *name,
		ProtoSpace	*space,
		ProtoMethod *code = NULL,
		ProtoObject *args = NULL,
		ProtoObject *kwargs = NULL
	);
	~ProtoThread();

	void		detach(ProtoContext *context);
	void 		join(ProtoContext *context);
	void		exit(ProtoContext *context); // ONLY for current thread!!!

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
	std::thread			*osThread;
	ProtoSpace			*space;
	Cell				*freeCells;
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

class ProtoContext {
public:
	ProtoContext(
		ProtoContext *previous = NULL,
		ProtoSpace *space = NULL,
		ProtoThread *thread = NULL
	);

	~ProtoContext();

	ProtoContext	*previous;
	ProtoSpace		*space;
	ProtoThread		*thread;
	Cell			*returnChain;
	ProtoObject		*returnSet;
	Cell			*lastCellPreviousContext;
	
	Cell 			*allocCell();
	void returnValue(ProtoObject *value=PROTO_NONE);

	// Constructors for base types, here to get the right context on invoke
	ProtoObject 	*fromInteger(int value);
	ProtoObject 	*fromDouble(double value);
	ProtoObject 	*fromUTF8Char(char *utf8OneCharString);
	ProtoObject 	*fromUTF8String(char *zeroTerminatedUtf8String);
	ProtoObject 	*fromMethod(ProtoObject *self, ProtoMethod *method);
	ProtoObject 	*fromExternalPointer(void *pointer);
	ProtoObject 	*newBuffer(unsigned long length);
	ProtoObject 	*fromBoolean(BOOLEAN value);
	ProtoObject 	*fromByte(char c);
	ProtoObject 	*literalFromUTF8String(char *zeroTerminatedUtf8String);
	ProtoObject 	*literalFromString(ProtoList *string);

	ProtoThread 	*getCurrentThread();

	ProtoObject 	*newMutable(ProtoObject *value=PROTO_NONE);
	// Only for already created mutables
};

// Usefull constants.
// ATENTION: They should be kept on synch with proto_internal.h!

#define PROTO_TRUE ((ProtoObject *)  0x0107L)
#define PROTO_FALSE ((ProtoObject *) 0x0007L)
#define PROTO_NULL ((ProtoObject *)  0x00L)

#endif /* PROTO_H_ */
