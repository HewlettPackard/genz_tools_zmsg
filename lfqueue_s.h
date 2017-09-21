#ifndef _LFQUEUE_S_H
#define _LFQUEUE_S_H

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

#include "lfqueue.h"

/* lock free queues of unsigned ints with static size set by NUM_SLOTS */
#define NUM_SLOTS 25  //this should be greater or equal to than RCV_BUFF_ENTRIES+1 in "sport_intf.h"

typedef struct lfqueue_s {
	unsigned head;
	unsigned tail;
	unsigned total_slots;
	unsigned flags;
	unsigned data_array[NUM_SLOTS];  //each queue holds NUM_SLOTS-1 entries
}lfqueue_s;

void init_lfqueue_s(lfqueue_s *lfq_data);
int push_tail_lfqueue_s(lfqueue_s *lfqueue_main, unsigned input); //insert the last element 
void* peek_tail_lfqueue_s(lfqueue_s *lfqueue_main); //get pointer to peek at the last elem of queue
int peek_head_lfqueue_s(lfqueue_s *lfqueue_main, unsigned* resuolt); //get pointer to peek at the first elem of queue
void remove_head_lfqueue_s(lfqueue_s *lfqueue_main); //remove the first elem of queue 
int pop_head_lfqueue_s(lfqueue_s *lfqueue_main, unsigned *data); //get value and remove first elem of queue 
int curr_size_lfqueue_s(lfqueue_s *lfqueue_main); //current number of members
int is_full_lfqueue_s(lfqueue_s *lfqueue_main);
int is_empty_lfqueue_s(lfqueue_s *lfqueue_main);

class ring_buffer{
	lfqueue<int> active_buffer;
	lfqueue<int> free_buffer;
	void* data_ptr=0;
	int padded_rec_len = 0;
	int size;
public:
	ring_buffer();
	~ring_buffer();
	ring_buffer(int size, int buff_len);
	void init(int num_entries, int buff_len );
	int push_tail(int buff_index);
	int is_empty();

	int  alloc(); //get index of allocated record
	int free(int buff_inx); //returns index back into free pool
	char* get_buff_pointer(int buff_index); //turns index into actual pointer to data
	int get_length_buff(); //gets the length of the buffer for remote put access
	int get_frame_size();
	int pop_head();
};


#endif
