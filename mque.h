#ifndef _MQ_H
#define _MQ_H

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

#include "zrecord.h"
#include "sim_signal.h"

/* DGMQ is used as a rcv port for each physical ZBRIDGE */

#define S_PORT_NAME_ALREADY_DEFINED 1 //receiver return a logical, port credit to the sender

//these constants define circular merge queues with power of two size & matching index mask
#define RING_SIZE 0x100 ////256 and 1024
#define RING_MASK 0xff

class signal_status{
public:
	semaphore s;
	int success;
};

struct mq_record{ //merge queue record
	volatile int ready; //useed for concurrent insertion
	signal_status* reverse_signal;  // for signaling completion back to sending zengine
	zrecord msg;
};

void init_mq_record(mq_record* rec);
void insert_imm_msg_mq_record(mq_record* rec_ptr, char* msg, zheader header);

struct merge_queue{ // one merge_queue is dedicated to each ZBridge
public:
	int cid;  //Component ID used to reach this ring
	unsigned head; //oldest entry
	unsigned tail; //next available slot
	unsigned total_slots; //total number of slots
	unsigned actual_slots; //actually holds one less
	mq_record slots[RING_SIZE]; //static declaration for actual buffer
	int num_samples; //statistics
	int cumulative_size; //statistics
	unsigned index_mask;  //used to mask indexing operations
};

void init_merge_queue(merge_queue*, int mtu, int index);
int size_merge_queue(merge_queue* q);
int too_full_merge_queue(merge_queue* q);
mq_record* insert_header_merge_queue(merge_queue* q, zheader header);  //test for sufficient space
mq_record* insert_header_wait_merge_queue(merge_queue* q, zheader header); //wait for sufficient space
mq_record* peek_head_merge_queue(merge_queue* q ); //get pointer to inspect head element
void remove_head_merge_queue(merge_queue* q);  //retire the head element when done processing
unsigned get_tail_merge_queue(merge_queue* q); //get next entry at tail of queue
int is_empty_merge_queue(merge_queue* q);

#endif