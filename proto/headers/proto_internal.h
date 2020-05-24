/*
 * proto_internal.h
 *
 *  Created on: 20 de jun. de 2016
 *      Author: gamarino
 */

#ifndef PROTO_INTERNAL_H_
#define PROTO_INTERNAL_H_

#include "proto.h"

#define TAG_MASK				  ((unsigned long) 0x07)
#define TYPE_MASK				  ((unsigned long) 0x0f)
#define TYPE_SHIFT				  ((unsigned long) 0x03)

#define POINTER_GET_TAG(p)		  (((unsigned long) p) & TAG_MASK)
#define POINTER_GET_TYPE(p)	  	  ((((unsigned long) p >> TYPE_SHIFT) & TYPE_MASK))	

#define POINTER_TAG_CELL 	      ((unsigned long) 0x00)
#define POINTER_TAG_MUTABLEOBJECT ((unsigned long) 0x01)
#define POINTER_TAG_SMALLINT      ((unsigned long) 0x02)
#define POINTER_TAG_SMALLDOUBLE   ((unsigned long) 0x03)
#define POINTER_TAG_EMBEDEDVALUE  ((unsigned long) 0x04)
#define POINTER_TAG_UNASSIGNED_5  ((unsigned long) 0x05)
#define POINTER_TAG_UNASSIGNED_6  ((unsigned long) 0x06)
#define POINTER_TAG_UNASSIGNED_7  ((unsigned long) 0x07)

#define EMBEDED_TYPE_BOOLEAN      ((unsigned long) 0x00)
#define EMBEDED_TYPE_UNICODECHAR  ((unsigned long) 0x01)
#define EMBEDED_TYPE_BYTE         ((unsigned long) 0x02)
#define EMBEDED_TYPE_TIMESTAMP    ((unsigned long) 0x03)
#define EMBEDED_TYPE_DATE         ((unsigned long) 0x04)
#define EMBEDED_TYPE_TIMEDELTA    ((unsigned long) 0x05)
#define EMBEDED_TYPE_UNASSIGNED_6 ((unsigned long) 0x06)
#define EMBEDED_TYPE_UNASSIGNED_7 ((unsigned long) 0x07)
#define EMBEDED_TYPE_UNASSIGNED_8 ((unsigned long) 0x08)
#define EMBEDED_TYPE_UNASSIGNED_9 ((unsigned long) 0x09)
#define EMBEDED_TYPE_UNASSIGNED_A ((unsigned long) 0x0A)
#define EMBEDED_TYPE_UNASSIGNED_B ((unsigned long) 0x0B)
#define EMBEDED_TYPE_UNASSIGNED_C ((unsigned long) 0x0C)
#define EMBEDED_TYPE_UNASSIGNED_D ((unsigned long) 0x0D)
#define EMBEDED_TYPE_UNASSIGNED_E ((unsigned long) 0x0E)
#define EMBEDED_TYPE_UNASSIGNED_F ((unsigned long) 0x0F)

#define CELL_TYPE_UNASSIGNED	  	((unsigned long) 0x00)
#define CELL_TYPE_IDENTITY_DICT   	((unsigned long) 0x01)
#define CELL_TYPE_PROTO_LIST	  	((unsigned long) 0x02)
#define CELL_TYPE_PROTO_SET		  	((unsigned long) 0x03)
#define CELL_TYPE_BYTE_BUFFER     	((unsigned long) 0x04)
#define CELL_TYPE_PROTO_THREAD    	((unsigned long) 0x05)
#define CELL_TYPE_PROTO_OBJECT    	((unsigned long) 0x06)
#define CELL_TYPE_EXTERNAL_POINTER 	((unsigned long) 0x07)
#define CELL_TYPE_METHOD	      	((unsigned long) 0x08)
#define CELL_TYPE_PARENT_LINK     	((unsigned long) 0x09)
#define CELL_TYPE_LITERAL_DICT    	((unsigned long) 0x0A)
#define CELL_TYPE_UNASSIGNED_B    	((unsigned long) 0x0B)
#define CELL_TYPE_UNASSIGNED_C    	((unsigned long) 0x0C)
#define CELL_TYPE_UNASSIGNED_D    	((unsigned long) 0x0D)
#define CELL_TYPE_UNASSIGNED_E    	((unsigned long) 0x0E)
#define CELL_TYPE_UNASSIGNED_F    	((unsigned long) 0x0F)

class Cell {
public:
	Cell(
		ProtoContext *context, 
		unsigned long type = CELL_TYPE_UNASSIGNED,
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
		ProtoObject *object
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

	ParentLink *parent;
	ProtoObject *object;
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
		char		*buffer,
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


#endif /* PROTO_INTERNAL_H_ */
