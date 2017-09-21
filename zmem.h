#ifndef _MEM_H
#define  _MEM_H

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

#include <list>
#include <vector>
#include <string>
using namespace std;
class memlist;

//trivial class to support zmalloc of shared regions

struct mem_region{
	int index;
	memlist* parent;  //allow easy return to common global pool
	vector<int> array;
	unsigned size; //total size of this region in words
	int char_count;  //character count for message

public:
	mem_region(int size, memlist* parent); //size in bytes
	void return_region();
	void put_string(string);
	void put_bytes(char* input, int len);
	string get_string();
	char* get_ptr(); //get pointer to beginning of the region
};

class memlist{ //trivial memory manager for global pool of buffers
	vector< list<mem_region*> > data_ptrs;
	vector<int> size_vect;
	vector<int> outstanding;
	int vec_len;

public:
	memlist();
	memlist( list< pair<int, int> > invect);
	mem_region* zmalloc_region(int size);
	void zfree_region(mem_region* input);
};

#endif
