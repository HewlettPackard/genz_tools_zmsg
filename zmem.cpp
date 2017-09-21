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

#include "zmem.h"
#include "tools.h"
#include "gdata.h"

mem_region::mem_region(int size_in, memlist* parent_in){
	size = ((size_in-1)/4) + 1 ; //size in bytes converted to word array size
	parent = parent_in;
	array.resize(size, 0);
}

void mem_region::return_region(){ //return this region to its parent memlist
	parent->zfree_region(this);
}

void mem_region::put_string(string in_string){
	int length = MIN(size, in_string.size());
	in_string.copy( (char* ) &array[0], length, 0);
	char_count = length;
}

void mem_region::put_bytes(char* input, int len){
	memcpy((char*) &array[0], input, len);
}

string mem_region::get_string(){
	char* pos = (char*) &array[0];
	string result(pos, pos + char_count);
	return result;
}

char* mem_region::get_ptr(){
	return (char*)&array[0];
}

memlist::memlist(){
	vec_len = 0;
}

memlist::memlist(list< pair<int, int> > invect){
	vec_len = 0;
	size_vect.resize(invect.size());
	outstanding.resize(invect.size());
	data_ptrs.resize(invect.size());
	for (list< pair<int, int> >::iterator it = invect.begin(); it != invect.end(); it++){
		int size = it->first;
		int number = it->second;
		size_vect[vec_len] = size;
		outstanding[vec_len] = 0;
		for (int i = 0; i < number; i++){
			data_ptrs[vec_len].push_back(new mem_region(size, this));
		}
		vec_len++;
	}
}

mem_region* memlist::zmalloc_region(int size_in){
	gdata.mem_lock.lock();
	int inx = 0;
	mem_region* result = 0; // this is the result of a failed request
	while (inx<vec_len){
		if(data_ptrs[inx].empty()) continue;  //no remaining elements in this list
		if (size_vect[inx] >= size_in){ //we found a fit
			result = data_ptrs[inx].front();
			data_ptrs[inx].pop_front();
			outstanding[inx]++;
			gdata.mem_lock.unlock();
			return result;
		}
		inx++;
	}
	gdata.mem_lock.unlock();
	return result;
}

void memlist::zfree_region(mem_region* input){
	int index = input ->index;
	outstanding[index]--;
	data_ptrs[index].push_back(input);
	return;
}