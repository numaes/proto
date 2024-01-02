/*
 * parent_link.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "../headers/proto.h"

namespace proto {


ProtoMethodCell::~ProtoMethodCell() {

}

ProtoMethodCell::ProtoMethodCell(
    ProtoContext *context,
    ProtoObject  *self,
    ProtoMethod	 method
) : Cell(context) {
    this->self = self;
    this->method = method;
};

void ProtoMethodCell::processReferences(
	ProtoContext *context, 
	void *self,
	void (*method)(
		ProtoContext *context, 
		void *self,
		Cell *cell
	)
) {
	method(context, self, this);
}

};

