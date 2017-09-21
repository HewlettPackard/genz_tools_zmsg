#ifndef _LFQUEUE_H
#define _LFQUEUE_H

/*
© Copyright 2017 Hewlett Packard Enterprise Development LP

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
in the documentation and / or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

*/

//Mike Schlansker and Ali Munir


#include <vector>
using namespace std;


//lock free queue of type T between single sender and single listner
template <class T>
class lfqueue{
	volatile unsigned head; //oldest entry
	volatile unsigned tail; //next available slot
	volatile unsigned total_slots; //max size+1 in number of entries
	vector<T> array;
	int inc(int); //private function for rotating advance of head or tail pointer
public:
	lfqueue();  //create lock free queue of elements of type T with nill maximum size

	lfqueue(const int max_size);  //create lock free queue of elements of type T with given maximum size
	void init(int max_size);
	bool push_tail(const T& input); //insert a newest member
	T* peek_tail(); //get pointer to peek at the oldest member
	bool push_tail(); //insert a  member modified by peek
	T* peek_head(); //get pointer to peek at the oldest member
	void remove_head(); //remove the oldest member
	T pop_head(); //get value and remove oldest element
	int curr_size(); //current number of members
	bool is_full();
	bool is_empty();
};



template <class T>
int lfqueue<T>::inc(int in){
	int result = (in + 1);
	if (result >= (int)total_slots) result = result - (int)total_slots;
	return result;
}

template <class T>
lfqueue<T>::lfqueue(){  //default constructor for useless queue of nill size
	head = 0; //head points to oldest entry in the queue
	tail = 0; //tail points to the next available entry
	array.resize(0);
}


template <class T>
lfqueue<T>::lfqueue(const int max_queue_size_in){
	total_slots = max_queue_size_in + 1; //one wasted slot is needed to hold the max_queue_size_in entries
	head = 0; //head points to oldest entry in the queue
	tail = 0; //tail points to the next available entry
	array.resize(total_slots);
}

template <class T>

void lfqueue<T>::init(int max_size){
	total_slots = max_size + 1; //one wasted slot is needed to hold the max_queue_size_in entries
	head = 0; //head points to oldest entry in the queue
	tail = 0; //tail points to the next available entry
	array.resize(total_slots);
}

template <class T>
bool lfqueue<T>::push_tail(const T& input){ //push a new value
	if (is_full()) return false;
	array[tail] = input;
	tail = inc(tail);
	return true;
}

template <class T>
T* lfqueue<T>::peek_tail(){ //get pointer to peek at the oldest member
	if (is_full()) return 0;
	return &array[tail];
}

template <class T>
bool lfqueue<T>::push_tail(){ //insert the tail member that was previously updated using the peek_tail pointer
	if (is_full()) return false;
	tail = inc(tail);
	return true;
}

template <class T>
T* lfqueue<T>::peek_head() { //get pointer to allow a peek at the head element
	if (is_empty()) return 0;
	T* result = &array[head];
	return result;
}

template <class T>
void  lfqueue<T>::remove_head() { //removes the head element
	if (is_empty()) return;
	head = inc(head);
	return;
}

template <class T>
T lfqueue<T>::pop_head() { //returns the value and removes the head element
	T result = array[head];  //result is undefined for empty queue
	remove_head();
	return result;
}

template <class T>
int  lfqueue<T>::curr_size(){
	int num = (tail - head);
	if (num < 0) num = num + total_slots;
	return num;
}

template <class T>
bool lfqueue<T>::is_full(){
	int temp_tail = inc(tail);
	if (temp_tail == head) return true;
	return false;
}

template <class T>
bool lfqueue<T>::is_empty(){
	if (tail == head) return true;
	return false;
}

//lock free queue of pointers to type T between single sender and single listener

template <class T>
class lfqueuep{

	volatile unsigned head; //oldest entry
	volatile unsigned tail; //next available slot
	volatile unsigned total_slots; //size(+1) in number of entries
	vector<T*> array;
	int inc(int);
public:
	lfqueuep();//create an empty queue of pointers to "T"
	lfqueuep(const int max_size);//create an empty queue of pointers to "T" having a given largest size
	void fill(); //fill the queue with pointers to new null elements
	bool push_tail(T* input); //insert a newest member
	T* peek_head(); //peek through the head pointer at the oldest member without removing
	void remove_head(); //remove the oldest member
	T* pop_head(); //get oldest member and remove

	int curr_size(); //current number of members
	bool is_full();
	bool is_empty();
};

template <class T>
int lfqueuep<T>::inc(int in){
	int result = (in + 1);
	if (result >= (int)total_slots) result = result - (int)total_slots;
	return result;
}

template <class T>
lfqueuep<T>::lfqueuep(){
	total_slots = 1;
	head = 0; //head points to oldest entry in the queue
	tail = 0; //tail points to the next available entry
	array.resize(1);
}

template <class T>
lfqueuep<T>::lfqueuep(const int size_in){
	total_slots = size_in+1;
	head = 0; //head points to oldest entry in the queue
	tail = 0; //tail points to the next available entry
	array.resize(total_slots);
}

template <class T>
void lfqueuep<T>::fill(){  //fill the queue with null elements
	for (unsigned int i = 0; i < total_slots; i++){
		T* elem = new T;
		push_tail(elem);
	}
}

template <class T>
bool lfqueuep<T>::push_tail(T* input){ //add a new element
	if (is_full()) return false;
	array[tail] = input;
	tail = inc(tail);
	return true;
}

template <class T>
T* lfqueuep<T>::peek_head() { //allow peek at the head element without removing
	if (is_empty()) return 0;
	T* result = array[head];
	return result;
}

template <class T>
void  lfqueuep<T>::remove_head() { //removes the head element of non-empty queue
	if (is_empty()) return;
	head = inc(head);
	return;
}

template <class T>
T* lfqueuep<T>::pop_head() { //returns and removes the head element
	if (is_empty()) return 0;//result is 0 for empty queue
	T* result = array[head];
	remove_head();
	return result;
}

template <class T>
int  lfqueuep<T>::curr_size(){
	int num = (tail - head);
	if (num < 0) num = num + total_slots;
	return num;
}

template <class T>
bool lfqueuep<T>::is_full(){
	int temp_tail = inc(tail);
	if (temp_tail == head) return true;
	return false;
}

template <class T>
bool lfqueuep<T>::is_empty(){
	if (tail == head) return true;
	return false;
}

#endif