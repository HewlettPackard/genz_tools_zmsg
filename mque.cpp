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


#include "mque.h"
#include "tools.h"
#include "sport_intf.h"

void init_mq_record(mq_record* rec){
	rec->ready=false; //message not ready
}

void insert_imm_msg_mq_record(mq_record* rec_ptr, char* msg,  zheader header){
	int curr_len = MIN(header.len, Z_RECORD_MTU); //threshold message length at mtu
	char *data_loc = (char*) &rec_ptr->msg.data[0];
	int body_len = header.len;
	rec_ptr->msg.header = header;
	memcpy(data_loc, msg, body_len);

	rec_ptr->ready = 1;
}

void init_merge_queue(merge_queue* q, int mtu, int index){
	q->total_slots = RING_SIZE;
	q->actual_slots = q->total_slots - 1;  //useable slots are one less than total slots
	q->index_mask = RING_MASK;
	q->head = 0; //head points to oldest entry in the queue
	q->tail = 0; //tail points to the next available entry
	q->num_samples = 0;  //used for statistics
	q->cumulative_size = 0;
	q->cid = index;  //the component ID for this ring is its index into the array of rings
	for (int i = 0; i < RING_SIZE; i++){
		init_mq_record(&q->slots[i]);
	}
}

int size_merge_queue(merge_queue* q){
	int num = ((q->tail&q->index_mask) - (q->head&q->index_mask));
	if (num<0) num = num + q->total_slots;  //for modulo subtraction
	return num;
}

int too_full_merge_queue(merge_queue* q){
	if (size_merge_queue(q) > ((int)q->actual_slots - 8)) return 1; 
	//there better be more than 8 total_slots or we cannot send anything!!
	//do not try insert more than 8 physical records concurrently!! (8-way SOC merge)
	return 0;
}

mq_record* insert_header_merge_queue(merge_queue* q, zheader header){  //test for sufficient space
	if (too_full_merge_queue(q)){
		return 0; //signal failed return with physical layer (ring-level) congestion
	}
	unsigned temp_tail = get_tail_merge_queue(q); 
	//grab the next slot at the tail and move the (must be null) message into the send window
	//fill in the message header contents
	mq_record* record_ptr = &q->slots[temp_tail & q->index_mask];
	record_ptr->msg.header=header;
	record_ptr->ready = 0;
	return record_ptr; //pointer to acquired slot for further processing
}

mq_record* insert_header_wait_merge_queue(merge_queue* q, zheader header){ //wait for sufficient space
	while (1){ //wait until room in ring
		mq_record* result = insert_header_merge_queue(q, header);
		if (result != 0) return result;
	}
	return 0;
}
mq_record* peek_head_merge_queue(merge_queue* q){ //get pointer to inspect head element
	if (is_empty_merge_queue(q) == 1) return 0;
	unsigned temp_head = q->head & q->index_mask;
	return &q->slots[temp_head];
}

void remove_head_merge_queue(merge_queue* q){  //retire the head element when done processing
	init_mq_record(&q->slots[q->head & q->index_mask]);
	fetch_add(q->head, 1);
}

unsigned get_tail_merge_queue(merge_queue* q){ //get next entry at tail of queue
	return fetch_add(q->tail, 1);
}

int is_empty_merge_queue(merge_queue* q){
	if (q->head == q->tail) return 1;
	return 0;
}