/*
 * list.cpp
 *
 *  Created on: 6 de ago. de 2017
 *      Author: gamarino
 */

#include "proto.h"
#include "proto_internal.h"

ListNode::~ListNode() {
}

ListNode::ListNode() {
}

void        ListNode::processReferences(void *callback(Cell *subCell)) {
}

void        ListNode::beforeDeleting() {
}

TreeNode    *ListNode::clone() {
}


TreeNode    *ListNode::get(ProtoObject *key) {
}

TreeNode    *ListNode::has(ProtoObject *key) {
}

TreeNode    *ListNode::set(ProtoObject *key, ProtoObject *value) {
}

TreeNode    *ListNode::remove(ProtoObject *key) {
}


ProtoObject *ListNode::at(ProtoObject *index) {
}

ProtoObject *ListNode::atSet(ProtoObject *index, ProtoObject *value) {
}

ProtoObject *ListNode::keys() {
}

ProtoObject *ListNode::size() {
}

ProtoObject *ListNode::getFirst() {
}

ProtoObject *ListNode::getLast() {
}


ProtoObject *ListNode::removeFirst() {
}

ProtoObject *ListNode::removeLast() {
}

ProtoObject *ListNode::removeAt(ProtoObject *index) {
}

ProtoObject *ListNode::removeSlice(ProtoObject *indexFrom, ProtoObject *indexTo) {
}


ProtoObject *ListNode::addFirst() {
}

ProtoObject *ListNode::addLast() {
}

ProtoObject *ListNode::insertAt(ProtoObject *index, ProtoObject *value) {
}

ProtoObject *ListNode::insertTreeAt(ProtoObject *index, TreeNode *tree) {
}


ProtoObject	*ListNode::slice(ProtoObject *from, ProtoObject *to) {
}


ProtoObject	*ListNode::extendFirst(ProtoObject *otherTree) {
}

ProtoObject	*ListNode::extendLast(ProtoObject *otherTree) {
}

