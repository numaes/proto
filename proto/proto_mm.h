/*
 * proto_mm.h
 *
 *  Created on: 5 de ago. de 2017
 *      Author: gamarino
 */

#ifndef PROTO_MM_H_
#define PROTO_MM_H_

#include "proto.h"
#include "proto_internal.h"

class AllocatedCellMarker;
class FreeCellMarker;

class AllocSegment {
	ProtoMM	*space;
	int		size;
	struct Cell *base;

	AllocatedCellMarker *allocatedMarker;
	FreeCellMarker *freeMarker;
};

class CellInfo {
public:
	void CellInfo();
	virtual void ~CellInfo();

	ProtoMM	*AllocSegment;

	virtual int isFreeCell();
};

class AllocatedCellMarker: public CellInfo {
public:
	virtual int isFreeCell() {return FALSE;}
};

class FreeCellMarker: public CellInfo {
public:
	virtual int isFreeCell() {return TRUE;}
};




#endif /* PROTO_MM_H_ */
