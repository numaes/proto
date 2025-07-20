/*
 *  templates.cpp
 *  See Copyright Notice in proto.h
 */

#include "../headers/proto_internal.h"

namespace proto {

template class ProtoListImplementation<ProtoObject>;
template class ProtoListIteratorImplementation<ProtoObject>;
template class ProtoSparseList<ProtoObject>;
template class ProtoSparseListIterator<ProtoObject>;

}
