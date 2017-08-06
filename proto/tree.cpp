/*
 * tree.cpp
 *
 *  Created on: 5 de ago. de 2017
 *      Author: gamarino
 */

#include "proto.h"
#include "proto_internal.h"

void setTreeStatistics(TreeNode *self) {
	if (!self->previous && !self->next) {
		self->count = 1;
		self->height = 1;
	}
	else if (!self->next) {
		self->count = self->previous->count + 1;
		self->height = self->previous->height + 1;
	}
	else if (!self->previous) {
		self->count = self->next->count + 1;
		self->height = self->next->height + 1;
	}
	else {
		self->count = self->previous->count + self->next->count + 1;
		self->height = (self->previous->height > self->next->height) ?
							self->previous->height + 1 :
							self->next->height + 1;
	}
}

void int getBalance(TreeNode *self) {
	if (self->next && self->previous)
		return self->previous->height - self->next->height;
	else if (self->previous)
		return self->previous->height;
	else if (self->next)
		return -self->next->height;
	else
		return 0;
}

// A utility function to right rotate subtree rooted with y
// See the diagram given above.
TreeNode *rightRotate(TreeNode *n)
{
	TreeNode *y = n->clone();
    TreeNode *x = y->previous->clone();
    TreeNode *T2 = x->next;

    // Perform rotation
    x->previous = y;
    y->next = T2;

    //  Update statistics
    setTreeStatistics(x);
    setTreeStatistics(y);

    // Return new root
    return x;
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
TreeNode *leftRotate(TreeNode *n)
{
	TreeNode *x = n->clone();
    TreeNode *y = x->next->clone();
    TreeNode *T2 = y->previous;

    // Perform rotation
    y->previous = x;
    x->next = T2;

    //  Update statistics
    setTreeStatistics(x);
    setTreeStatistics(y);

    // Return new root
    return y;
}


TreeNode::TreeNode(ProtoObject *key, TreeNode *previous = NULL, TreeNode *next = NULL) {
	if (key) {
		this->key = key;
		this->previous = previous;
		this->next = next;
		setTreeStatistics(this);
	}
	else {
		// Empty tree case

		this->key = NULL;
		this->previous = NULL;
		this->next = NULL;
	}
}

void TreeNode::processReferences(void *callback(Cell *)) {
	if (this->previous)
		(*callback)(this->previous);
	if (this->next)
		(*callback)(this->next);
}

void TreeNode::beforeDeleting() {

}

TreeNode *TreeNode::clone() {
	return new TreeNode(this->key, this->previous, this->next);
}

TreeNode *TreeNode::get(ProtoObject *key) {
	TreeNode *node = this;

	if (!this->key)
		return NULL;

	while (node) {
		if (node->key == key)
			return this;
		if (((int) node->key) < ((int) key))
			node = node->next;
		else
			node = node->previous;
	}
	return NULL;
}

int TreeNode::has(ProtoObject *key) {
	TreeNode *node = this;

	if (!this->key)
		return FALSE;

	while (node) {
		if (node->key == key)
			return TRUE;
		if (((int) node->key) < ((int) key))
			node = node->next;
		else
			node = node->previous;
	}
	return FALSE;
}

TreeNode *TreeNode::add(ProtoObject *key) {
	TreeNode *newNode;
	TreeNode *newAux;
	int cmp;

	newNode = this->clone();

	// Empty tree case
	if (!this->key) {
		newNode->key = key;
		newNode->previous = NULL;
		newNode->next = NULL;
		return newNode;
	}

	if (this->key != key) {
		cmp = ((int) this->key) < ((int) key);
		if (cmp && this->next) {
			newNode->next = this->next->add(key);
		}
		else if (!cmp && this->previous) {
			newNode->previous = this->previous->add(key);
		}
		else {
			newAux = this->clone();
			newAux->previous = (!cmp) ? newNode->previous:
										NULL;
			newAux->next = (cmp) ? newNode->previous:
							       NULL;
			newAux->key = key;
		}
		setTreeStatistics(newNode);
		int balance = getBalance(newNode);

		// If this node becomes unbalanced, then
		// there are 4 cases

		// Left Left Case
		if (balance > 1 && key < newNode->previous->key)
			return rightRotate(newNode);

		// Right Right Case
		if (balance < -1 && key > newNode->next->key)
			return leftRotate(newNode);

		// Left Right Case
		if (balance > 1 && key > newNode->previous->key)
		{
			newNode->previous = leftRotate(newNode->previous);
			return rightRotate(newNode);
		}

		// Right Left Case
		if (balance < -1 && key < newNode->next->key)
		{
			newNode->next = rightRotate(newNode->next);
			return leftRotate(newNode);
		}
	}

	return newNode;
}

TreeNode *TreeNode::remove(ProtoObject *key) {
	// STEP 1: PERFORM STANDARD BST DELETE

	TreeNode *newRoot = this;

	// Empty tree?
	if (!this->key)
		return this;

	// If the key to be deleted is smaller than the
	// root's key, then it lies in left subtree

	if (key < this->key && this->previous) {
		newRoot = this->clone();
		newRoot->previous = this->previous->remove(key);
	}

	// If the key to be deleted is greater than the
	// root's key, then it lies in right subtree
	else if (key > this->key && this->next) {
		newRoot = this->clone();
		newRoot->next = this->next(key);
	}

	// if key is same as root's key, then This is
	// the node to be deleted
	else
	{
		// node with only one child or no child
		if (!this->previous || !this->next)
		{
			TreeNode *temp = this->previous ? this->previous :
											  this->next;
			// No child case
			if (!temp) {
				TreeNode *emptyTree = this->clone();
				emptyTree->key = NULL;
				emptyTree->previous = NULL;
				emptyTree->next = NULL;
				return emptyTree;
			}
			else // One child case
				return temp;
		}
		else
		{
			// node with two children: Get the inorder
			// successor (smallest in the right subtree)
			TreeNode *s = this->next;
			while (s->previous)
				s = s->previous;
			newRoot = s->clone();

			newRoot->previous = this->previous;
			newRoot->next = this->next->remove(key);
		}
	}

	// STEP 2: Update statistics of new root
	setTreeStatistics(newRoot);

	// STEP 3: GET THE BALANCE FACTOR OF THIS NODE (to
	// check whether this node became unbalanced)
	int balance = getBalance(newRoot);

	// If this node becomes unbalanced, then there are 4 cases

	// Left Left Case
	if (balance > 1 && getBalance(newRoot->previous) >= 0)
		return rightRotate(newRoot);

	// Left Right Case
	if (balance > 1 && getBalance(newRoot->previous) < 0) {
		newRoot->previous = leftRotate(newRoot->previous);
		return rightRotate(newRoot);
	}

	// Right Right Case
	if (balance < -1 && getBalance(newRoot->next) <= 0)
		return leftRotate(newRoot);

	// Right Left Case
	if (balance < -1 && getBalance(newRoot->next) > 0){
		newRoot->next = rightRotate(newRoot->next);
		return leftRotate(newRoot);
	}

	return newRoot;
}

