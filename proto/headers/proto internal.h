/* 
 * proto.h
 *
 *  Created on: November, 2017 - Redesign January, 2024
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */


#ifndef PROTO_INTERNAL_H
#define PROTO_INTERNAL_H

#include "../headers/proto.h"
#include <thread>

namespace proto {

#ifdef NULL
#undef NULL
#endif

#define NULL 0L
#define TRUE 1

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
#define POINTER_TAG_LIST_ITERATOR     	(0x09LU)
#define POINTER_TAG_TUPLE_ITERATOR     	(0x0ALU)
#define POINTER_TAG_STRING_ITERATOR    	(0x0BLU)
#define POINTER_TAG_SPARSE_LIST_ITERATOR (0x0CLU)
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

// Base definition of an ProtoObject *

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

class Cell {
public:
	Cell(ProtoContext *context);
	virtual ~Cell();

	void *operator new(unsigned long size, ProtoContext *context);

	virtual unsigned long getHash(ProtoContext *context);

	// Called before discarding the Cell by garbage collector
	virtual void finalize(ProtoContext *context);

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

class ProtoObjectCellImplementation;


// ParentPointers are chains of parent classes used to solve attribute access
class ParentLinkImplementation: public Cell, public ParentLink {
protected:
public:
	ParentLinkImplementation(
		ProtoContext *context,
		ParentLinkImplementation *parent,
		ProtoObjectCellImplementation *object
	);

	virtual ~ParentLinkImplementation();

	virtual void finalize(ProtoContext *context);

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

	ParentLinkImplementation *parent;
	ProtoObjectCellImplementation *object;
};

template<class T> class ProtoListImplementation;

template<class T> class ProtoListIteratorImplementation: public Cell {
public:
	ProtoListIteratorImplementation(
		ProtoContext *context,
		ProtoListImplementation<T> *base,
		unsigned long currentIndex
	);
	virtual ~ProtoListIteratorImplementation();

	virtual int hasNext(ProtoContext *context);
	virtual ProtoObject *next(ProtoContext *context);
	virtual ProtoListIteratorImplementation *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

private:
	ProtoListImplementation<T> *base;
	unsigned long currentIndex;

};

template<class T> class ProtoListImplementation: public Cell, public ProtoList<T> {
public:
	ProtoListImplementation(
		ProtoContext *context,
		ProtoObject *value = PROTO_NONE,
		ProtoListImplementation *previous = NULL,
		ProtoListImplementation *next = NULL
	);
	virtual ~ProtoListImplementation();

	virtual T *getAt(ProtoContext *context, int index);
	virtual T *getFirst(ProtoContext *context);
	virtual T *getLast(ProtoContext *context);
	virtual ProtoListImplementation	*getSlice(ProtoContext *context, int from, int to);
	virtual unsigned long getSize(ProtoContext *context);

	virtual BOOLEAN has(ProtoContext *context, T *value);
	virtual ProtoListImplementation *setAt(ProtoContext *context, int index, T *value = PROTO_NONE);
	virtual ProtoListImplementation *insertAt(ProtoContext *context, int index, T *value);

	virtual ProtoListImplementation *appendFirst(ProtoContext *context, T *value);
	virtual ProtoListImplementation *appendLast(ProtoContext *context, T *value);

	virtual ProtoListImplementation *extend(ProtoContext *context, ProtoList* other);

	virtual ProtoListImplementation	*splitFirst(ProtoContext *context, int index);
	virtual ProtoListImplementation *splitLast(ProtoContext *context, int index);

	virtual ProtoListImplementation	*removeFirst(ProtoContext *context);
	virtual ProtoListImplementation	*removeLast(ProtoContext *context);
	virtual ProtoListImplementation	*removeAt(ProtoContext *context, int index);
	virtual ProtoListImplementation *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoTuple *asTuple(ProtoContext *context);
	virtual ProtoObject	*asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	virtual ProtoListIteratorImplementation *getIterator(ProtoContext *context);

	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoListImplementation	*previous;
	ProtoListImplementation *next;

	ProtoObject 	*value;
	unsigned long   hash;

	unsigned long count:52;
	unsigned long height:8;
	unsigned long type:4;
};

#define TUPLE_SIZE 5

class ProtoTupleImplementation;
class ProtoTupleIteratorImplementation: public Cell, public ProtoTupleIterator {
public:
	ProtoTupleIteratorImplementation (
		ProtoContext *context,
		ProtoTupleImplementation *base,
		unsigned long currentIndex
	);
	virtual ~ProtoTupleIteratorImplementation();

	virtual int hasNext(ProtoContext *context);
	virtual ProtoObject *next(ProtoContext *context);
	virtual ProtoTupleIteratorImplementation *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);

	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

private:
	ProtoTupleImplementation *base;
	unsigned long currentIndex;
};

#define TUPLE_SIZE 5

class ProtoTupleImplementation: public Cell, public ProtoTuple {
public:
	ProtoTupleImplementation(
		ProtoContext *context,
		unsigned long elementCount = 0,
		unsigned long height=0,
		ProtoObject **data = (ProtoObject **) NULL,
		ProtoTupleImplementation **indirect = (ProtoTupleImplementation **) NULL
	);
	virtual ~ProtoTupleImplementation();

	static ProtoTupleImplementation *tupleFromList(ProtoContext *context, ProtoList *list);
	static TupleDictionary *createTupleRoot(ProtoContext *context);

	virtual ProtoObject   *getAt(ProtoContext *context, int index);
	virtual ProtoObject   *getFirst(ProtoContext *context);
	virtual ProtoObject   *getLast(ProtoContext *context);
	virtual ProtoTupleImplementation	  *getSlice(ProtoContext *context, int from, int to);
	virtual unsigned long  getSize(ProtoContext *context);

	virtual BOOLEAN		   has(ProtoContext *context, ProtoObject* value);
	virtual ProtoTupleImplementation    *setAt(ProtoContext *context, int index, ProtoObject* value);
	virtual ProtoTupleImplementation    *insertAt(ProtoContext *context, int index, ProtoObject* value);

	virtual ProtoTupleImplementation    *appendFirst(ProtoContext *context, ProtoTuple* otherTuple);
	virtual ProtoTupleImplementation 	*appendLast(ProtoContext *context, ProtoTuple* otherTuple);

	virtual ProtoTupleImplementation	*splitFirst(ProtoContext *context, int count = 1);
	virtual ProtoTupleImplementation    *splitLast(ProtoContext *context, int count = 1);

	virtual ProtoTupleImplementation    *removeFirst(ProtoContext *context, int count = 1);
	virtual ProtoTupleImplementation    *removeLast(ProtoContext *context, int count = 1);
	virtual ProtoTupleImplementation    *removeAt(ProtoContext *context, int index);
	virtual ProtoTupleImplementation    *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoList	  *asList(ProtoContext *context);
	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long  getHash(ProtoContext *context);
	virtual ProtoTupleIteratorImplementation *getIterator(ProtoContext *context);

	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	unsigned long elementCount:56;
	unsigned long height:8;
	union {
		ProtoObject   *data[TUPLE_SIZE];
		ProtoTupleImplementation    *indirect[TUPLE_SIZE]; 
	} pointers;

};

class ProtoStringImplementation;

class ProtoStringIteratorImplementation: public Cell, public ProtoStringIterator {
public:
	ProtoStringIteratorImplementation(
		ProtoContext *context,
		ProtoStringImplementation *base,
		unsigned long currentIndex = 0
	);
	virtual ~ProtoStringIteratorImplementation();

	virtual int hasNext(ProtoContext *context);
	virtual ProtoObject *next(ProtoContext *context);
	virtual ProtoStringIteratorImplementation *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoStringImplementation *base;
	unsigned long currentIndex;
};

class ProtoStringImplementation: public Cell, public ProtoString {
public:
	ProtoStringImplementation(
		ProtoContext *context,
		ProtoTupleImplementation *baseTuple
	);

	virtual ~ProtoStringImplementation();

	int	cmp_to_string(ProtoContext *context, ProtoString *otherString);

	virtual ProtoObject    *getAt(ProtoContext *context, int index);
	virtual ProtoStringImplementation    *setAt(ProtoContext *context, int index, ProtoObject* character);
	virtual ProtoStringImplementation    *insertAt(ProtoContext *context, int index, ProtoObject* character);
	unsigned long    	    getSize(ProtoContext *context);
	virtual ProtoStringImplementation	   *getSlice(ProtoContext *context, int from, int to);

	virtual ProtoStringImplementation    *setAtString(ProtoContext *context, int index, ProtoString* otherString);
	virtual ProtoStringImplementation    *insertAtString(ProtoContext *context, int index, ProtoString* otherString);

	virtual ProtoStringImplementation    *appendFirst(ProtoContext *context, ProtoString* otherString);
	virtual ProtoStringImplementation    *appendLast(ProtoContext *context, ProtoString* otherString);

	virtual ProtoStringImplementation	   *splitFirst(ProtoContext *context, int count = 1);
	virtual ProtoStringImplementation    *splitLast(ProtoContext *context, int count = 1);

	virtual ProtoStringImplementation	   *removeFirst(ProtoContext *context, int count = 1);
	virtual ProtoStringImplementation	   *removeLast(ProtoContext *context, int count = 1);
	virtual ProtoStringImplementation	   *removeAt(ProtoContext *context, int index);
	virtual ProtoStringImplementation    *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoObject	   *asObject(ProtoContext *context);
	virtual ProtoList	   *asList(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	virtual ProtoStringIteratorImplementation *getIterator(ProtoContext *context);

	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	ProtoTupleImplementation *baseTuple;

};


#define ITERATOR_NEXT_PREVIOUS 0
#define ITERATOR_NEXT_THIS 1
#define ITERATOR_NEXT_NEXT 2

class ProtoSparseListImplementation;

class ProtoSparseListIteratorImplementation: public Cell, public ProtoSparseListIterator {
public:
	ProtoSparseListIteratorImplementation(
		ProtoContext *context,
		int state,
		ProtoSparseListImplementation *current,
		ProtoSparseListIteratorImplementation *queue = NULL
	);
	virtual ~ProtoSparseListIteratorImplementation();

	virtual int hasNext(ProtoContext *context);
	virtual ProtoTuple *next(ProtoContext *context);
	virtual ProtoSparseListIteratorImplementation *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);

	virtual void finalize(ProtoContext *context);

	virtual void processReferences(
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			Cell *cell
		)
	);

	int state = ITERATOR_NEXT_PREVIOUS;
	ProtoSparseListImplementation *current;
	ProtoSparseListIteratorImplementation *queue = NULL;

};

class ProtoSparseListImplementation: public Cell, public ProtoSparseList {
public:
	ProtoSparseListImplementation(
		ProtoContext *context,
		unsigned long index = 0,
		ProtoObject *value = PROTO_NONE,
		ProtoSparseListImplementation *previous = NULL,
		ProtoSparseListImplementation *next = NULL
	);
	virtual ~ProtoSparseListImplementation();

	virtual BOOLEAN			has(ProtoContext *context, unsigned long index);
	virtual ProtoObject     *getAt(ProtoContext *context, unsigned long index);
	virtual ProtoSparseListImplementation *setAt(ProtoContext *context, unsigned long index, ProtoObject *value = PROTO_NONE);
	virtual ProtoSparseListImplementation *removeAt(ProtoContext *context, unsigned long index);
	virtual int				isEqual(ProtoContext *context, ProtoSparseList *otherDict);
	virtual ProtoObject     *getAtOffset(ProtoContext *context, int offset);

	unsigned long 	getSize(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	virtual ProtoSparseListIteratorImplementation *getIterator(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

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

	virtual void processValues (
		ProtoContext *context,
		void *self,
		void (*method) (
			ProtoContext *context,
			void *self,
			ProtoObject *value
		)
	);

	ProtoSparseListImplementation *previous;
	ProtoSparseListImplementation *next;

	unsigned long 	index;
	ProtoObject 	*value;
	unsigned long   hash;

	unsigned long count:52;
	unsigned long height:8;
	unsigned long type:4;
};

class ProtoByteBufferImplementation: public Cell, public ProtoByteBuffer {
public:
	ProtoByteBufferImplementation(
		ProtoContext *context,
		unsigned long size,
		char 		  *buffer = (char *) NULL	
	);
	virtual ~ProtoByteBufferImplementation();

	virtual char getAt(ProtoContext *context, int index);
	virtual void setAt(ProtoContext *context, int index, char value);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

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
	int				freeOnExit;
};

class ProtoExternalPointerImplementation: public Cell, public ProtoExternalPointer {
public:
	ProtoExternalPointerImplementation(
		ProtoContext *context,
		void		*pointer
	);
	virtual ~ProtoExternalPointerImplementation();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

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


class ProtoMethodCellImplementation: public Cell, public ProtoMethodCell {
public:
	ProtoMethodCellImplementation(
		ProtoContext *context,
		ProtoObject  *self,
		ProtoMethod	 method
	);
	virtual ~ProtoMethodCellImplementation();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

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

class ProtoObjectCellImplementation: public Cell, public ProtoObjectCell {
public:
	ProtoObjectCellImplementation(
		ProtoContext *context,
		ParentLinkImplementation	*parent = NULL,
		unsigned long mutable_ref = 0,
		ProtoSparseListImplementation  *attributes = NULL
	);
	virtual ~ProtoObjectCellImplementation();

	virtual ProtoObjectCellImplementation *addParent(ProtoContext *context, ProtoObjectCellImplementation *object);

	virtual ProtoObject	*asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

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

	unsigned long mutable_ref;
	ParentLinkImplementation	*parent;
	ProtoSparseListImplementation  *attributes;
};

class ProtoThreadImplementation: public Cell, public ProtoThread {
public:
	ProtoThreadImplementation(
		ProtoContext *context,
		ProtoString *name,
		ProtoSpace	*space,
		ProtoMethod code = (ProtoMethod) NULL,
		ProtoList *args = (ProtoList *) NULL,
		ProtoSparseList *kwargs = (ProtoSparseList *) NULL
	);
	virtual ~ProtoThreadImplementation();

	void		detach(ProtoContext *context);
	void 		join(ProtoContext *context);
	void		exit(ProtoContext *context); // ONLY for current thread!!!

	void		setManaged();
	void		setUnmanaged();
	void		synchToGC();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	virtual void finalize(ProtoContext *context);

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
	ProtoContext		*currentContext;
	long				state:4;
	long				unmanagedCount:60;
};

};

#endif /* PROTO_H_ */
