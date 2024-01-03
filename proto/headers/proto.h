/* /*
 * proto.h
 *
 *  Created on: November, 2017
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */

#include <cstddef>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


namespace proto {

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

#define TAG_BITS				  4
#define EMBEDED_BITS			  4

#define TAG_MASK				  ((1LU << TAG_BITS) - 1)
#define TYPE_MASK				  ((1LU << EMBEDED_BITS) - 1)
#define TYPE_SHIFT				  (TAG_BITS + EMBEDED_BITS)

#define POINTER_GET_TAG(p)		  (((unsigned long) p) & TAG_MASK)
#define POINTER_GET_TYPE(p)	  	  ((((unsigned long) p >> TAG_SHIFT) & TYPE_MASK))

#define POINTER_TAG_OBJECT 	      		(0x00LU)
#define POINTER_TAG_EMBEDEDVALUE  		(0x01LU)
#define POINTER_TAG_LIST		  		(0x02LU)
#define POINTER_TAG_SPARSE_LIST   		(0x03LU)
#define POINTER_TAG_TUPLE         		(0x04LU)
#define POINTER_TAG_BYTE_BUFFER   		(0x05LU)
#define POINTER_TAG_EXTERNAL_POINTER	(0x06LU)
#define POINTER_TAG_METHOD_CELL     	(0x07LU)
#define POINTER_TAG_STRING		     	(0x08LU)
#define POINTER_TAG_UNASSIGNED_9     	(0x09LU)
#define POINTER_TAG_UNASSIGNED_A     	(0x0ALU)
#define POINTER_TAG_UNASSIGNED_B     	(0x0BLU)
#define POINTER_TAG_UNASSIGNED_C     	(0x0CLU)
#define POINTER_TAG_UNASSIGNED_D     	(0x0DLU)
#define POINTER_TAG_UNASSIGNED_E     	(0x0ELU)
#define POINTER_TAG_UNASSIGNED_F     	(0x0FLU)

#define EMBEDED_TYPE_BOOLEAN      (0x00LU)
#define EMBEDED_TYPE_UNICODECHAR  (0x01LU)
#define EMBEDED_TYPE_BYTE         (0x02LU)
#define EMBEDED_TYPE_TIMESTAMP    (0x03LU)
#define EMBEDED_TYPE_DATE         (0x04LU)
#define EMBEDED_TYPE_TIMEDELTA    (0x05LU)
#define EMBEDED_TYPE_SMALLINT	  (0x06LU)
#define EMBEDED_TYPE_SMALLDOUBLE  (0x07LU)
#define EMBEDED_TYPE_UNASSIGNED_8 (0x08LU)
#define EMBEDED_TYPE_UNASSIGNED_9 (0x09LU)
#define EMBEDED_TYPE_UNASSIGNED_A (0x0ALU)
#define EMBEDED_TYPE_UNASSIGNED_B (0x0BLU)
#define EMBEDED_TYPE_UNASSIGNED_C (0x0CLU)
#define EMBEDED_TYPE_UNASSIGNED_D (0x0DLU)
#define EMBEDED_TYPE_UNASSIGNED_E (0x0ELU)
#define EMBEDED_TYPE_UNASSIGNED_F (0x0FLU)

#define SPACE_STATE_RUNNING			(0x0)
#define SPACE_STATE_STOPPING_WORLD  (0x1)
#define SPACE_STATE_WORLD_TO_STOP   (0x2)
#define SPACE_STATE_WORLD_STOPPED   (0x3)
#define SPACE_STATE_ENDING          (0x4)

#define THREAD_STATE_MANAGED        (0x0)
#define THREAD_STATE_UNMANAGED      (0x1)
#define THREAD_STATE_STOPPING       (0x3)
#define THREAD_STATE_STOPPED        (0x3)

// Forward declarations

class Cell;
class BigCell;
class ProtoContext;
class ProtoList;
class ProtoSparseList;
class TupleDictionary;

class AllocatedSegment {
public:
    BigCell *memoryBlock;
    int     cellsCount;
    AllocatedSegment *nextBlock;
};

class DirtySegment {
public:
	BigCell	 *cellChain;
	DirtySegment *nextSegment;
};

typedef ProtoObject *(*ProtoMethod)(
	ProtoContext *, 		// context
	ProtoObject *, 			// self
	ProtoObject *, 			// prototype
	ProtoList *, 			// positionalParameters
	ProtoSparseList *		// keywordParameters
);

// Base tree structure used by Dictionaries, Sets and Lists
// Internal use exclusively

union ProtoObjectPointer {
	struct {
		ProtoObject	*oid;
	} oid;

	struct {
		int	hash;
	} asHash;

	struct {
		Cell *cell;
	} cell;

	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long value:(64 - TAG_BITS - EMBEDED_BITS);
	} op;

	// Pointer tag dependent values
	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		long smallInteger:(64 - TAG_BITS - EMBEDED_BITS);
	} si;
	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long smallDouble:(64 - TAG_BITS - EMBEDED_BITS);
	} sd;
	// Embedded types dependent values
	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long unicodeValue:32;
	} unicodeChar;
	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long booleanValue:1;
	} booleanValue;
	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long byteData:8;
	} byteValue;

	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long timestamp:(64 - TAG_BITS - EMBEDED_BITS);
	} timestampValue;

	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		unsigned long day:8;
		unsigned long month:8;
		unsigned long year:16;
	} date;

	struct {
		unsigned long pointer_tag:TAG_BITS;
		unsigned long embedded_type:EMBEDED_BITS;
		long timedelta:(64 - TAG_BITS - EMBEDED_BITS);
	} timedeltaValue;

};

class ProtoObject {
public:
	ProtoObject *clone(ProtoContext *c);
	ProtoObject *newChild(ProtoContext *c);

	ProtoObject *getType(ProtoContext *c);
	ProtoObject *getAttribute(ProtoContext *c, ProtoString *name);
	ProtoObject *hasAttribute(ProtoContext *c, ProtoString *name);
	ProtoObject *hasOwnAttribute(ProtoContext *c, ProtoObject *name);
	ProtoObject *setAttribute(ProtoContext *c, ProtoString *name, ProtoObject *value);

	ProtoList   *getAttributes(ProtoContext *c);
	ProtoList   *getOwnAttributes(ProtoContext *c);
	ProtoList   *getParent(ProtoContext *c);

	ProtoObject *addParent(ProtoContext *c, ProtoObject *newParent);
	ProtoObject *isInstanceOf(ProtoContext *c, ProtoObject *prototype);

	ProtoObject *call(ProtoContext *c,
					  ProtoString *method,
					  ProtoList *unnamedParametersList,
			          ProtoSparseList *keywordParametersDict);

	unsigned long getHash(ProtoContext *context);

	void finalize();

	void processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);


	BOOLEAN			 asBoolean();
	int				 asInteger();
	double			 asDouble();
	char			 asByte();

	BOOLEAN			 isBoolean();
	BOOLEAN			 isInteger();
	BOOLEAN			 isDouble();
	BOOLEAN			 isByte();

};

class Cell {
public:
	Cell(ProtoContext *context);
	~Cell();

	void *operator new(size_t size, ProtoContext *context);

	virtual unsigned long getHash(ProtoContext *context);

	// Called before discarding the Cell by garbage collector
	virtual void finalize();

	// Apply method recursivelly to all referenced objects, except itself
    virtual void processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);

    Cell *nextCell;
};


// Used only for block allocations
// The maximun number of pointers is 8
// per Cell.
// Cell itsef uses 2 (virtual class pointer and nextCell), thus only 6 are available to subclasses
// The biggest is IdentityDict
class BigCell : public Cell {
private:
	Cell *p1;
	Cell *p2;
	Cell *p3;
	Cell *p4;
	Cell *p5;
	Cell *p6;
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

	virtual void finalize();

	// Apply method recursivelly to all referenced objects, except itself
    virtual void processReferences(
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

class ProtoList: virtual public Cell {
public:
	ProtoList(
		ProtoContext *context,
		ProtoObject *value = PROTO_NONE,
		ProtoList *previous = NULL,
		ProtoList *next = NULL
	);
	~ProtoList();

	virtual ProtoObject   *getAt(ProtoContext *context, int index);
	virtual ProtoObject   *getFirst(ProtoContext *context);
	virtual ProtoObject   *getLast(ProtoContext *context);
	virtual ProtoList	  *getSlice(ProtoContext *context, int from, int to);
	virtual unsigned long  getSize(ProtoContext *context);

	virtual BOOLEAN		   has(ProtoContext *context, ProtoObject* value);
	virtual ProtoList     *setAt(ProtoContext *context, int index, ProtoObject* value = PROTO_NONE);
	virtual ProtoList     *insertAt(ProtoContext *context, int index, ProtoObject* value);

	virtual ProtoList  	  *appendFirst(ProtoContext *context, ProtoObject* value);
	virtual ProtoList  	  *appendLast(ProtoContext *context, ProtoObject* value);

	virtual ProtoList  	  *extend(ProtoContext *context, ProtoList* other);

	virtual ProtoList	  *splitFirst(ProtoContext *context, int index);
	virtual ProtoList     *splitLast(ProtoContext *context, int index);

	virtual ProtoList	  *removeFirst(ProtoContext *context);
	virtual ProtoList	  *removeLast(ProtoContext *context);
	virtual ProtoList	  *removeAt(ProtoContext *context, int index);
	virtual ProtoList  	  *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoTuple    *asTuple(ProtoContext *context);
	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);

	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoList		*previous;
	ProtoList		*next;

	ProtoObject 	*value;

	unsigned long count:52;
	unsigned long height:8;
	unsigned long type:4;
};


#define TUPLE_ATOM_SIZE 4
#define TUPLE_INDIRECT_SIZE 4

class ProtoIndirectTuple: public Cell {
public:
	ProtoIndirectTuple(
		ProtoContext *context,
		unsigned long element_count = 0,
		ProtoIndirectTuple *continuation = NULL,
		ProtoTuple **data = NULL
	);
	virtual ~ProtoIndirectTuple();

	virtual ProtoObject   *getAt(ProtoContext *context, int index);

	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoIndirectTuple    *continuation;

	ProtoTuple   *data[TUPLE_INDIRECT_SIZE];
};

class ProtoTuple: public Cell {
public:
	ProtoTuple(
		ProtoContext *context,
		unsigned long element_count = 0,
		ProtoIndirectTuple *continuation = NULL,
		ProtoObject **data = NULL
	);
	~ProtoTuple();

	virtual ProtoObject   *getAt(ProtoContext *context, int index);
	virtual ProtoObject   *getFirst(ProtoContext *context);
	virtual ProtoObject   *getLast(ProtoContext *context);
	virtual ProtoList	  *getSlice(ProtoContext *context, int from, int to);
	virtual unsigned long  getSize(ProtoContext *context);

	virtual BOOLEAN		   has(ProtoContext *context, ProtoObject* value);
	virtual ProtoTuple    *setAt(ProtoContext *context, int index, ProtoObject* value);
	virtual ProtoTuple    *insertAt(ProtoContext *context, int index, ProtoObject* value);

	virtual ProtoTuple 	  *appendFirst(ProtoContext *context, ProtoObject* value);
	virtual ProtoTuple 	  *appendLast(ProtoContext *context, ProtoObject* value);

	virtual ProtoTuple 	  *extend(ProtoContext *context, ProtoList* other);

	virtual ProtoTuple	  *splitFirst(ProtoContext *context, int index);
	virtual ProtoTuple    *splitLast(ProtoContext *context, int index);

	virtual ProtoTuple	  *removeFirst(ProtoContext *context);
	virtual ProtoTuple	  *removeLast(ProtoContext *context);
	virtual ProtoTuple	  *removeAt(ProtoContext *context, int index);
	virtual ProtoTuple 	  *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoList	  *asList(ProtoContext *context);
	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long  getHash(ProtoContext *context);

	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	unsigned long element_count;
	ProtoIndirectTuple    *continuation;

	ProtoObject   *data[4];
};

class ProtoString: public Cell {
public:
	ProtoString(
		ProtoContext *context,
		ProtoTuple *baseTuple
	);

	~ProtoString();


	int	cmp_to_string(ProtoContext *context, ProtoString *other_string);

	virtual ProtoObject    *getAt(ProtoContext *context, int index);
	virtual ProtoString    *setAt(ProtoContext *context, int index, ProtoObject* character);
	virtual ProtoString    *insertAt(ProtoContext *context, int index, ProtoObject* character);
	unsigned long    	    getSize(ProtoContext *context);
	virtual ProtoString	   *getSlice(ProtoContext *context, int from, int to);

	virtual ProtoString    *setAtString(ProtoContext *context, int index, ProtoString* other_string);
	virtual ProtoString    *insertAtString(ProtoContext *context, int index, ProtoString* other_string);

	virtual ProtoString    *appendFirst(ProtoContext *context, ProtoString* other_string);
	virtual ProtoString    *appendLast(ProtoContext *context, ProtoString* other_string);

	virtual ProtoString	   *splitFirst(ProtoContext *context, int count);
	virtual ProtoString    *splitLast(ProtoContext *context, int count);

	virtual ProtoString	   *removeFirst(ProtoContext *context, int count = 1);
	virtual ProtoString	   *removeLast(ProtoContext *context, int counte = 1);
	virtual ProtoString	   *removeAt(ProtoContext *context, int index);
	virtual ProtoString    *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoObject	   *asObject(ProtoContext *context);
	virtual ProtoList	   *asList(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);

	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoTuple *baseTuple;

};

class ProtoSparseList: public Cell {
public:
	ProtoSparseList(
		ProtoContext *context,
		unsigned long index = 0,
		ProtoObject *value = PROTO_NONE,
		ProtoSparseList *previous = NULL,
		ProtoSparseList *next = NULL
	);
	~ProtoSparseList();

	virtual BOOLEAN			has(ProtoContext *context, unsigned long index);
	virtual ProtoObject     *getAt(ProtoContext *context, unsigned long index);
	virtual ProtoSparseList *setAt(ProtoContext *context, unsigned long index, ProtoObject *value = PROTO_NONE);
	virtual ProtoSparseList *removeAt(ProtoContext *context, unsigned long index);
	virtual int				isEqual(ProtoContext *context, ProtoSparseList *otherDict);

	unsigned long 	getSize(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize();

	virtual void processReferences (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	virtual void processElements (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			unsigned long index,
			ProtoObject *value
		)
	);

	virtual void processIndexes (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *key
		)
	);

	virtual void processValues (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *value
		)
	);

	ProtoSparseList    	*previous;
	ProtoSparseList		*next;

	unsigned long 	index;
	ProtoObject 	*value;

	unsigned long count:52 = 0;
	unsigned long height:8 = 0;
	unsigned long type:4 = 0;
};

class ProtoByteBuffer: public Cell {
public:
	ProtoByteBuffer(
		ProtoContext *context,
		unsigned long size,
		char 		  *buffer	
	);
	~ProtoByteBuffer();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	char			*buffer;
	unsigned long  	size;
};

class ProtoExternalPointer: public Cell {
public:
	ProtoExternalPointer(
		ProtoContext *context,
		void		*pointer
	);
	~ProtoExternalPointer();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize();

	virtual void processReferences(
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

class ProtoMethodCell: public Cell {
public:
	ProtoMethodCell(
		ProtoContext *context,
		ProtoObject  *self,
		ProtoMethod	 method
	);
	~ProtoMethodCell();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoMethod	*getMethod();

	ProtoMethod	method;
	ProtoObject *self;
};

class ProtoMutableReference: public Cell {
public:
	ProtoMutableReference(
		ProtoContext *context,
		ProtoObject  *reference 
	);
	~ProtoMutableReference();

	virtual void finalize();

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoObject *reference;
};

class ProtoObjectCell: public Cell {
public:
	ProtoObjectCell(
		ProtoContext *context,
		ParentLink	*parent = NULL,
		ProtoMutableReference *mutable_ref = NULL,
		ProtoSparseList  *attributes = NULL
	);
	 ~ProtoObjectCell();

	virtual ProtoObjectCell *addParent(ProtoContext *context, ProtoObjectCell *object);

	virtual ProtoObject	*asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize();

	// Apply method recursivelly to all referenced objects, except itself
    virtual void processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);

	ProtoMutableReference *mutable_ref;
	ParentLink	*parent;
	ProtoSparseList  *attributes;
};

class ProtoThread: public Cell {
public:
	ProtoThread(
		ProtoContext *context,
		ProtoString *name,
		ProtoSpace	*space,
		ProtoMethod *code = NULL,
		ProtoObject *args = NULL,
		ProtoObject *kwargs = NULL
	);
	~ProtoThread();

	void		detach(ProtoContext *context);
	void 		join(ProtoContext *context);
	void		exit(ProtoContext *context); // ONLY for current thread!!!

	void		setManaged();
	void		setUnmanaged();
	void		synchToGC();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize();

	// Apply method recursivelly to all referenced objects, except itself
    virtual void processReferences(
		ProtoContext *context, 
		void *self,
		void (*method)(
			ProtoContext *context, 
			void *self,
			Cell *cell
		)
	);

	Cell	    *allocCell();

	ProtoString			*name;
	std::thread			*osThread;
	ProtoSpace			*space;
	BigCell				*freeCells;
	ProtoContext		*firstContext;
	ProtoContext		*currentContext;
	unsigned int		state;
	int					unmanagedCount;
};

class ProtoContext {
public:
	ProtoContext(
		ProtoContext *previous = NULL,
		void *localsBase = NULL,
		unsigned int localsCount = 0, 
		ProtoThread *thread = NULL,
		ProtoSpace *space = NULL
	);

	~ProtoContext();

	ProtoContext	*previous;
	ProtoSpace		*space;
	ProtoThread		*thread;
	ProtoObject		*returnValue;
	Cell			*lastAllocatedCell;
	ProtoObjectPointer *localsBase;
	unsigned int	localsCount;
	unsigned int	allocatedCellsCount;
	
	Cell 			*allocCell();
	void			checkCellsCount();

	// Constructors for base types, here to get the right context on invoke
	ProtoObject 	*fromInteger(int value);
	ProtoObject 	*fromDouble(double value);
	ProtoTuple      *tupleFromList(ProtoList *list);
	ProtoObject 	*fromUTF8Char(char *utf8OneCharString);
	ProtoString 	*fromUTF8String(char *zeroTerminatedUtf8String);
	ProtoMethodCell		 	*fromMethod(ProtoObject *self, ProtoMethod method);
	ProtoExternalPointer 	*fromExternalPointer(void *pointer);
	ProtoByteBuffer 		*fromBuffer(unsigned long length, void*buffer);
	ProtoByteBuffer		 	*newBuffer(unsigned long length);
	ProtoObject 	*fromBoolean(BOOLEAN value);
	ProtoObject 	*fromByte(char c);
	ProtoObject     *fromDate(unsigned year, unsigned month, unsigned day);
	ProtoObject     *fromTimestamp(unsigned long timestamp);
	ProtoObject     *fromTimeDelta(long timedelta);

	ProtoList		*newList();
	ProtoTuple		*newTuple();

	ProtoThread 	*getCurrentThread();

	ProtoObject		*newInmutable();
	ProtoObject 	*newMutable();
};

std::mutex		globalMutex;

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

	Cell 		*getFreeCells(ProtoThread *currentThread);
	void 		analyzeUsedCells(Cell *cellsChain);
	void		triggerGC();

	// TODO Should it has a dictionary to access threads by name?
	std::atomic<TupleDictionary *> tupleRoot;
	

	ProtoList			*threads;

	Cell			 	*freeCells;
	DirtySegment 		*dirtySegments;
	int					 state;

	unsigned int 		 maxAllocatedCellsPerContext;
	int					 blocksPerAllocation;
	int					 heapSize;
	int					 maxHeapSize;
	int 			     freeCellsCount;
	unsigned int		 gcSleepMilliseconds;
	int					 blockOnNoMemory;

	std::atomic<ProtoSparseList *>  mutableRoot;
	std::atomic<BOOLEAN> mutableLock;
	std::atomic<BOOLEAN> threadsLock;
	std::atomic<BOOLEAN> gcLock;
	std::thread::id		 mainThreadId;
	std::thread			*gcThread;
	std::condition_variable stopTheWorldCV;
	std::condition_variable restartTheWorldCV;
	std::condition_variable gcCV;
	int					 gcStarted;
};

// Used just to compute the number of bytes needed for a Cell at allocation time

static_assert (sizeof(ProtoSparseList) == 56);

// Usefull constants.
// ATENTION: They should be kept on synch with proto_internal.h!

#define PROTO_TRUE ((ProtoObject *)  0x0107L)
#define PROTO_FALSE ((ProtoObject *) 0x0007L)
#define PROTO_NULL ((ProtoObject *)  0x00L)

};

#endif /* PROTO_H_ */
