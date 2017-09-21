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

#include "lfqueue_s.h"

void init_lfqueue_s(lfqueue_s *lfq_data) {
		lfq_data->total_slots = NUM_SLOTS;
		lfq_data->head = 0;
		lfq_data->tail = 0;
        return;
}

int inc_w(lfqueue_s *lfqueue_main, int in){
	int result = (in + 1);
	if (result >= (int) lfqueue_main->total_slots) result = result - (int) lfqueue_main->total_slots;
	return result;
}

int push_tail_lfqueue_s(lfqueue_s *lfqueue_main, unsigned input){ //push a new value
        if (is_full_lfqueue_s(lfqueue_main)) return 0;
		lfqueue_main->data_array[lfqueue_main->tail] = input;
        lfqueue_main->tail = inc_w(lfqueue_main, lfqueue_main->tail);
        return 1;
}

void* peek_tail_lfqueue_s(lfqueue_s *lfqueue_main){ //get pointer to peek at the oldest member
        if (is_full_lfqueue_s(lfqueue_main)) return 0;
		unsigned* addr = &lfqueue_main->data_array[lfqueue_main->tail];
		return addr;
}

int peek_head_lfqueue_s(lfqueue_s *lfqueue_main, unsigned* result) { //get pointer to allow a peek at the head element
        if (is_empty_lfqueue_s(lfqueue_main)) return 0;
		*result = lfqueue_main->head;
		return  1;
}

void remove_head_lfqueue_s(lfqueue_s *lfqueue_main) { //removes the head element
        if (is_empty_lfqueue_s(lfqueue_main)) return;
        lfqueue_main->head = inc_w(lfqueue_main, lfqueue_main->head);
        return;
}

int pop_head_lfqueue_s(lfqueue_s *lfqueue_main, unsigned *data) { //returns the value and removes the head element
     if (is_empty_lfqueue_s(lfqueue_main)) {
		return 0;
	}
	 *data = lfqueue_main->data_array[lfqueue_main->head];
    remove_head_lfqueue_s(lfqueue_main);
    return 1;
}

int curr_size_lfqueue_s(lfqueue_s *lfqueue_main){
        int num = (lfqueue_main->tail - lfqueue_main->head);
        if (num < 0) num = num + lfqueue_main->total_slots;
        return num;
}

int is_full_lfqueue_s(lfqueue_s *lfqueue_main){
        int temp_tail = inc_w(lfqueue_main, lfqueue_main->tail);
        if (temp_tail == lfqueue_main->head) return 1;
        return 0;
}

int is_empty_lfqueue_s(lfqueue_s *lfqueue_main){
        if (lfqueue_main->tail == lfqueue_main->head) return 1;
        return 0;
}

ring_buffer::ring_buffer(){
}

ring_buffer::~ring_buffer(){
	::free(data_ptr);
}

ring_buffer::ring_buffer(int size, int buff_len){
	init(size, buff_len);
}



#define ALIGN_BOUNDARY 4

void ring_buffer::init(int num_entries, int rec_len){
	//alignment is maintained for each record pointer
	size = num_entries;
	padded_rec_len = ((rec_len + ALIGN_BOUNDARY - 1) / ALIGN_BOUNDARY) * ALIGN_BOUNDARY;
	active_buffer.init(num_entries);  //resize circular queues
	free_buffer.init(num_entries);
	data_ptr = malloc(num_entries*padded_rec_len);
	for (int i = 0; i < num_entries; i++){
		free_buffer.push_tail(i);
	}
}

int  ring_buffer::push_tail(int buff_index){
	return active_buffer.push_tail(buff_index);
}

int  ring_buffer::is_empty(){
	return active_buffer.is_empty();
}

int ring_buffer::alloc(){
	if (free_buffer.is_empty()) return -1;
	return free_buffer.pop_head();
}

int ring_buffer::free(int buff_id){
	int success = free_buffer.push_tail(buff_id);
	return success;
}

char*  ring_buffer::get_buff_pointer(int buff_index){
	return ((char *)data_ptr + (buff_index*padded_rec_len));
}

int ring_buffer::get_length_buff(){
	return size*padded_rec_len;
}

int ring_buffer::get_frame_size(){
	return padded_rec_len;
}

int  ring_buffer::pop_head(){
	if (active_buffer.is_empty()) return -1;
	return active_buffer.pop_head();
}

