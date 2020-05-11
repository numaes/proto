/*
 * proto.h
 *
 *  Created on: November, 2017
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */

#include <cstddef>
#include <atomic>

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

class ProtoContext;
class ProtoObject;
class ProtoThread;

class Cell {
public:
	Cell(
		ProtoContext *context, 
		Cell *nextCell = NULL,
		unsigned long type = 0,
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

		ProtoObject *key = PROTO_NONE,
		ProtoObject *hash = PROTO_NONE,
		TreeCell *previous = NULL,
		TreeCell *next = NULL,
		unsigned long type = 0,
		unsigned long count = 0,
		unsigned long height = 0,

		Cell *nextCell = NULL
	);

	~TreeCell();
};

// Global literal dictionary
class LiteralDictionary: public TreeCell {
public:
	LiteralDictionary(
		ProtoContext *context,

		ProtoObject *key = PROTO_NONE,
		ProtoObject *hash = PROTO_NONE,
		TreeCell *previous = NULL,
		TreeCell *next = NULL,
		unsigned long count = 0,
		unsigned long height = 0,
		Cell *nextCell = NULL
	);
	~LiteralDictionary();

	ProtoObject			*get(ProtoObject *newName);
	LiteralDictionary	*set(ProtoContext *context, ProtoObject *newName);
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

class ProtoSpace {
protected:
	int					blocksInCurrentSegment;
	AllocatedSegment 	*segments;
	DirtySegment 		*dirtySegments;

public:
	ProtoSpace();
	virtual ~ProtoSpace();

	ProtoObject	*threads;

	ProtoObject *getThreads();

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

	ProtoThread *getNewThread();

	Cell 		*getFreeCells();
	void 		analyzeUsedCells(Cell *cellsChain);
	void 		deallocMemory();
	std::atomic<Cell *> mutableRoot;
};

typedef struct {
	ProtoObject *keyword;
	ProtoObject *value;
} KeywordParameter;

typedef ProtoObject *(*ProtoMethod)(
	ProtoContext *, 		// context
	ProtoObject *, 			// self
	ProtoObject *, 			// type
	int, 					// positionalCount
	int, 					// keywordCount
	KeywordParameter **, 	// keywordParameters
	ProtoObject **		 	// positionalParameters
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
	ProtoObject *getHash();
	ProtoObject *isInstanceOf(ProtoContext *c, ProtoObject *prototype);

	ProtoObject *call(ProtoContext *c,
					  ProtoObject *method,
					  ProtoObject *unnamedParametersList,
			          ProtoObject *keywordParametersDict);

	ProtoObject *currentValue();
	ProtoObject *setValue(ProtoObject *currentValue, ProtoObject *newValue);
};

class ProtoThread: public Cell, public ProtoObject {
public:
	ProtoThread(
		ProtoContext *context,

		ProtoObject *name=NULL,
		Cell		*currentWorkingSet=NULL,
		Cell		*freeCells=NULL,
		ProtoSpace	*space=NULL
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
	ProtoSpace			*space;
	Cell				*freeCells;
};

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
	ProtoObject 	*fromBuffer(char *pointer, unsigned long length);
	ProtoObject 	*fromBoolean(BOOLEAN value);
	ProtoObject 	*fromByte(char c);
	ProtoObject 	*literalFromString(char *zeroTerminatedUtf8String);

	ProtoObject 	*newMutable(ProtoObject *value=PROTO_NONE);
	ProtoThread 	*getCurrentThread();
};

// Usefull constants.
// ATENTION: They should be kept on synch with proto_internal.h!

#define PROTO_TRUE ((ProtoObject *)  0x0107)
#define PROTO_FALSE ((ProtoObject *) 0x0007)
#define PROTO_NULL ((ProtoObject *)  0x00L)

#endif /* PROTO_H_ */
