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

	ProtoObject	*threads;

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

	ProtoObject *threads;
	ProtoThread *allocThread(ProtoContext *context, ProtoThread *thread);
	ProtoThread *deallocThread(ProtoContext *context, ProtoThread *thread);

	Cell 		*getFreeCells();
	void 		analyzeUsedCells(Cell *cellsChain);
	void 		deallocMemory();

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

	ProtoObject *currentValue();

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
	ProtoObject 	*literalFromUTF8String(char *zeroTerminatedUtf8String);
	ProtoObject 	*literalFromString(ProtoList *string);

	ProtoObject 	*newMutable(ProtoObject *value=PROTO_NONE);
	ProtoThread 	*getCurrentThread();
};

// Usefull constants.
// ATENTION: They should be kept on synch with proto_internal.h!

#define PROTO_TRUE ((ProtoObject *)  0x0107)
#define PROTO_FALSE ((ProtoObject *) 0x0007)
#define PROTO_NULL ((ProtoObject *)  0x00L)

#endif /* PROTO_H_ */
