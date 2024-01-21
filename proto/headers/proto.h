/* 
 * proto
 *
 *  Created on: November, 2017 - Redesign January, 2024
 *      Author: Gustavo Adrian Marino <gamarino@numaes.com>
 */

#include <atomic>
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

// Usefull constants.
// ATENTION: They should be kept on synch with proto_internal.h!

#define PROTO_TRUE ((ProtoObject *)  0x010FL)
#define PROTO_FALSE ((ProtoObject *) 0x000FL)
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

// Forward declarations

class Cell;
class BigCell;
class ProtoContext;
class ProtoList;
class ProtoSparseList;
class ProtoTuple;
class ProtoString;
class TupleDictionary;
class ProtoObject;
class ParentLink;
class ProtoObjectCell;
class ProtoSpace;
class DirtySegment;

typedef ProtoObject *(*ProtoMethod) (
	ProtoContext *, 		// context
	ProtoObject *, 			// self
	ParentLink *, 			// parentLink
	ProtoList *, 			// positionalParameters
	ProtoSparseList *		// keywordParameters
);

class ProtoObject {
public:
	ProtoObject *clone(ProtoContext *c, BOOLEAN isMutable = FALSE);
	ProtoObject *newChild(ProtoContext *c, BOOLEAN isMutable = FALSE);

	ProtoObject *getAttribute(ProtoContext *c, ProtoString *name);
	ProtoObject *hasAttribute(ProtoContext *c, ProtoString *name);
	ProtoObject *hasOwnAttribute(ProtoContext *c, ProtoString *name);
	ProtoObject *setAttribute(ProtoContext *c, ProtoString *name, ProtoObject *value);

	ProtoSparseList   *getAttributes(ProtoContext *c);
	ProtoSparseList   *getOwnAttributes(ProtoContext *c);
	ProtoList   	  *getParents(ProtoContext *c);

	ProtoObject *addParent(ProtoContext *c, ProtoObjectCell *newParent);
	ProtoObject *isInstanceOf(ProtoContext *c, ProtoObject *prototype);

	ProtoObject *call(ProtoContext *c,
	                  ParentLink *nextParent,
					  ProtoString *method,
					  ProtoObject *self,
					  ProtoList *unnamedParametersList = NULL,
			          ProtoSparseList *keywordParametersDict = NULL);

	unsigned long getHash(ProtoContext *context);
	int isCell(ProtoContext *context);
	Cell *asCell(ProtoContext *context);

	void finalize(ProtoContext *context);

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


// ParentPointers are chains of parent classes used to solve attribute access
class ParentLink {
public:
	ProtoObject *getObject(ProtoContext *context);
	ParentLink *getParent(ProtoContext *context);
};

class ProtoListIterator {
public:
	virtual int hasNext(ProtoContext *context);
	virtual ProtoObject *next(ProtoContext *context);
	virtual ProtoListIterator *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
};

class ProtoList {
public:
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
	virtual ProtoListIterator *getIterator(ProtoContext *context);

};

class ProtoTupleIterator {
public:
	virtual int hasNext(ProtoContext *context);
	virtual ProtoObject *next(ProtoContext *context);
	virtual ProtoTupleIterator *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
};

class ProtoTuple {
public:
	virtual ProtoObject   *getAt(ProtoContext *context, int index);
	virtual ProtoObject   *getFirst(ProtoContext *context);
	virtual ProtoObject   *getLast(ProtoContext *context);
	virtual ProtoTuple	  *getSlice(ProtoContext *context, int from, int to);
	virtual unsigned long  getSize(ProtoContext *context);

	virtual BOOLEAN		   has(ProtoContext *context, ProtoObject* value);
	virtual ProtoTuple    *setAt(ProtoContext *context, int index, ProtoObject* value);
	virtual ProtoTuple    *insertAt(ProtoContext *context, int index, ProtoObject* value);

	virtual ProtoTuple 	  *appendFirst(ProtoContext *context, ProtoTuple* otherTuple);
	virtual ProtoTuple 	  *appendLast(ProtoContext *context, ProtoTuple* otherTuple);

	virtual ProtoTuple	  *splitFirst(ProtoContext *context, int count = 1);
	virtual ProtoTuple    *splitLast(ProtoContext *context, int count = 1);

	virtual ProtoTuple	  *removeFirst(ProtoContext *context, int count = 1);
	virtual ProtoTuple	  *removeLast(ProtoContext *context, int count = 1);
	virtual ProtoTuple	  *removeAt(ProtoContext *context, int index);
	virtual ProtoTuple 	  *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoList	  *asList(ProtoContext *context);
	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long  getHash(ProtoContext *context);
	virtual ProtoTupleIterator *getIterator(ProtoContext *context);

};

class ProtoStringIterator {
public:
	virtual int hasNext(ProtoContext *context);
	virtual ProtoObject *next(ProtoContext *context);
	virtual ProtoStringIterator *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);

};

class ProtoString {
public:
	int	cmp_to_string(ProtoContext *context, ProtoString *otherString);

	virtual ProtoObject    *getAt(ProtoContext *context, int index);
	virtual ProtoString    *setAt(ProtoContext *context, int index, ProtoObject* character);
	virtual ProtoString    *insertAt(ProtoContext *context, int index, ProtoObject* character);
	unsigned long    	    getSize(ProtoContext *context);
	virtual ProtoString	   *getSlice(ProtoContext *context, int from, int to);

	virtual ProtoString    *setAtString(ProtoContext *context, int index, ProtoString* otherString);
	virtual ProtoString    *insertAtString(ProtoContext *context, int index, ProtoString* otherString);

	virtual ProtoString    *appendFirst(ProtoContext *context, ProtoString* otherString);
	virtual ProtoString    *appendLast(ProtoContext *context, ProtoString* otherString);

	virtual ProtoString	   *splitFirst(ProtoContext *context, int count = 1);
	virtual ProtoString    *splitLast(ProtoContext *context, int count = 1);

	virtual ProtoString	   *removeFirst(ProtoContext *context, int count = 1);
	virtual ProtoString	   *removeLast(ProtoContext *context, int count = 1);
	virtual ProtoString	   *removeAt(ProtoContext *context, int index);
	virtual ProtoString    *removeSlice(ProtoContext *context, int from, int to);

	virtual ProtoObject	   *asObject(ProtoContext *context);
	virtual ProtoList	   *asList(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	virtual ProtoStringIterator *getIterator(ProtoContext *context);

};

class ProtoSparseListIterator {
public:
	virtual int hasNext(ProtoContext *context);
	virtual ProtoTuple *next(ProtoContext *context);
	virtual ProtoSparseListIterator *advance(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual void finalize(ProtoContext *context);

};

class ProtoSparseList {
public:
	virtual BOOLEAN			has(ProtoContext *context, unsigned long index);
	virtual ProtoObject     *getAt(ProtoContext *context, unsigned long index);
	virtual ProtoSparseList *setAt(ProtoContext *context, unsigned long index, ProtoObject *value = PROTO_NONE);
	virtual ProtoSparseList *removeAt(ProtoContext *context, unsigned long index);
	virtual int				isEqual(ProtoContext *context, ProtoSparseList *otherDict);
	virtual ProtoObject     *getAtOffset(ProtoContext *context, int offset);

	virtual unsigned long 	getSize(ProtoContext *context);
	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	virtual ProtoSparseListIterator *getIterator(ProtoContext *context);
	
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

};

class ProtoByteBuffer {
public:
	virtual unsigned long getSize(ProtoContext *context);
	virtual char *getBuffer(ProtoContext *context);
	virtual char getAt(ProtoContext *context, int index);
	virtual void setAt(ProtoContext *context, int index, char value);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
};

class ProtoExternalPointer {
public:
	virtual void *getPointer(ProtoContext *context);
	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
};

// In order to compile a method the folling structure is recommended:
// method (self, parent, p1, p2, p3, p4=init4, p5=init5) {
//	   super().method()
// }
//     
// ProtoObject *literalForP4, *literalForP5;
// ProtoObject *constantForInit4, *constantForInit5;
// 
// ProtoObject *method(ProtoContext *previousContext, ProtoParent *parent, ProtoList *positionalParameters, ProtoSparseList *keywordParameters)
//      // Parameters + locals
//		struct {
//   		ProtoObject *p1, *p2, *p3, *p4, *p5, *l1, *l2, *l3;
//		} locals;
//		ProtoContext context(previousContext, &locals, sizeof(locals) / sizeof(ProtoObject *));
//
//		locals.p4 = alreadyInitializedConstantForInit4;
//		locals.p5 = alreadyInitializedConstantForInit5;
//
//      if (positionalParameters) {
//     		int unnamedSize = positionalParameters->getSize(&context);
//
//          if (unnamedSize < 3)
//    			raise "Too few parameters. At least 3 positional parameters are expected"
//
//	    	locals.p1 = positionalParameters->getAt(&context, 0);
//	    	locals.p2 = positionalParameters->getAt(&context, 1);
//	    	locals.p3 = positionalParameters->getAt(&context, 2);
//
//          if (unnamedSize > 3)
//			    locals.p4 = positionalParameters->getAt(&context, 3);
//
//          if (unnamedSize > 4)
//	    		locals.p5 = positionalParameters->getAt(&context, 4);
//
//		    if (unnamedSize > 5)
//			    raise "Too many parameters"
//      }
//      else
//          raise "At least 3 positional parameters expected"
//
//      if (keywordParameters) {
//          if (keywordParameters->has(&context, literalForP4)) {
//	    	    if (unnamedSize > 4)
//		    	    raise "Double assignment on p4"
//
//			    locals.p4 = keywordParameters->getAt(&context, literalForP4);
//          }
//
//          if (keywordParameters->has(&context, literalForP5)) {
//	    	    if (unnamedSize > 5)
//		    	    raise "Double assignment on p5"
//
//			    locals.p5 = keywordParameters->getAt(&context, literalForP5);
//          }
//      }
//
//		ProtoParent *nextParent;
//      if (parent)
//		    nextParent = parent;
// 
//      if (nextParent)
//          nextParent->object->call(this, nextParent->parent, positionalParameters, keywordParameters);
//      else
//          raise "There is no super!!"
//
//
//
// Not used keywordParameters are not detected
// This provides a similar behaviour to Python, and it can be automatically generated based on compilation time info
//
// You can use try ... catch to handle exceptions or not, it's up to you

class ProtoMethodCell {
public:
	virtual ProtoObject *getSelf(ProtoContext *context);
	virtual ProtoMethod getMethod(ProtoContext *context);

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	ProtoMethod	method;
	ProtoObject *self;
};

class ProtoObjectCell {
public:
	virtual ProtoObjectCell *addParent(ProtoContext *context, ProtoObjectCell *object);

	virtual ProtoObject	*asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
	unsigned long mutable_ref;
	ParentLink	*parent;
	ProtoSparseList  *attributes;
};

class ProtoThread {
public:
	void		detach(ProtoContext *context);
	void 		join(ProtoContext *context);
	void		exit(ProtoContext *context); // ONLY for current thread!!!

	void		setManaged();
	void		setUnmanaged();
	void		synchToGC();

	virtual ProtoObject	  *asObject(ProtoContext *context);
	virtual unsigned long getHash(ProtoContext *context);
	
};

class ProtoContext {
public:
	ProtoContext(
		ProtoContext *previous = NULL,
		ProtoObject **localsBase = NULL,
		unsigned int localsCount = 0, 
		ProtoThread *thread = NULL,
		ProtoSpace *space = NULL
	);

	virtual ~ProtoContext();

	ProtoContext	*previous;
	ProtoSpace		*space;
	ProtoThread		*thread;
	Cell			*lastAllocatedCell;
	ProtoObject     **localsBase;
	unsigned int	localsCount;
	unsigned int	allocatedCellsCount;
	
	Cell 			*allocCell();
	void			checkCellsCount();
	void			setReturnValue(ProtoContext *context, ProtoObject *returnValue);

	// Constructors for base types, here to get the right context on invoke
	ProtoObject 	*fromInteger(int value);
	ProtoObject 	*fromDouble(double value);
	ProtoTuple      *tupleFromList(ProtoList *list);
	ProtoObject 	*fromUTF8Char(const char *utf8OneCharString);
	ProtoString 	*fromUTF8String(const char *zeroTerminatedUtf8String);
	ProtoMethodCell		 	*fromMethod(ProtoObject *self, ProtoMethod method);
	ProtoExternalPointer 	*fromExternalPointer(void *pointer);
	ProtoByteBuffer 		*fromBuffer(unsigned long length, char* buffer);
	ProtoByteBuffer		 	*newBuffer(unsigned long length);
	ProtoObject 	*fromBoolean(BOOLEAN value);
	ProtoObject 	*fromByte(char c);
	ProtoObject     *fromDate(unsigned year, unsigned month, unsigned day);
	ProtoObject     *fromTimestamp(unsigned long timestamp);
	ProtoObject     *fromTimeDelta(long timedelta);

	ProtoList		*newList();
	ProtoTuple		*newTuple();
	ProtoSparseList *newSparseList();

};

class ProtoSpace {
public:
	ProtoSpace(
		ProtoMethod mainFunction,
		int argc = 0,
		char **argv = NULL
	);
	virtual ~ProtoSpace();

	ProtoObject *getThreads();

	ProtoObject	*objectPrototype;
	ProtoObject *smallIntegerPrototype;
	ProtoObject *smallFloatPrototype;
	ProtoObject *unicodeCharPrototype;
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
	ProtoObject *threadPrototype;
	ProtoObject *rootObject;

	ProtoObject *listPrototype;
	ProtoObject *listIteratorPrototype;

	ProtoObject *tuplePrototype;
	ProtoObject *tupleIteratorPrototype;

	ProtoObject *stringPrototype;
	ProtoObject *stringIteratorPrototype;

	ProtoObject *sparseListPrototype;
	ProtoObject *sparseListIteratorPrototype;

	ProtoString *literalGetAttribute;
	ProtoString *literalSetAttribute;
	ProtoString *literalCallMethod;

	Cell 		*getFreeCells(ProtoThread *currentThread);
	void 		analyzeUsedCells(Cell *cellsChain);
	void		triggerGC();

	ProtoSparseList	 	*threads;

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

	std::atomic<TupleDictionary *> tupleRoot;
	std::atomic<ProtoSparseList *>  mutableRoot;
	std::atomic<BOOLEAN> mutableLock;
	std::atomic<BOOLEAN> threadsLock;
	std::atomic<BOOLEAN> gcLock;
	std::thread::id		 mainThreadId;
	ProtoThread			*mainThread;
	std::thread			*gcThread;
	std::condition_variable stopTheWorldCV;
	std::condition_variable restartTheWorldCV;
	std::condition_variable gcCV;
	int					 gcStarted;

	static std::mutex	 globalMutex;

};

};

#endif /* PROTO_H_ */
