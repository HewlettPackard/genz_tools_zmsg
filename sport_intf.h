#ifndef _SPORT_INTF_H
#define _SPORT_INTF_H

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
#include "lfqueue.h"
#include "zrecord.h"
#include "mque.h"
#include "net_port_id.h"
#include "zengine.h"
#include <string>
#include <iterator>

#define MAX_REG_BUFFERS 20 //allowed nubmer of registration buffers

//message commands that are interpreted by hardware
#define MSG_ENQA 1  //two-sided immediate message command (enqueue atomic)
#define MSG_NOOP  2  //noop command
#define MSG_PUT 3  //one-sided messaging semantics without ZMMU
#define MSG_GET 4  
#define MSG_PUT_WZMMU 5  //one-sided messaging semantics with ZMMU



#define CMDPORT_SUCCESS 1 //successful return
#define CMDPORT_FAIL 0 //error -- generic fail
#define CMDPORT_NO_BUFFERS -1
#define CMDPORT_ENQA_FAIL -2  //error -- no room in receive queue
#define CMDPORT_REMOTE_MEM_UNREGISTERED -3
#define CMDPORT_REMOTE_MEM_OUTOFBOUNDS -4

unsigned make_nodeportid(unsigned hport, unsigned sport);
unsigned get_sport_nodeportid(unsigned input);
unsigned get_hport_nodeportid(unsigned input);
unsigned make_nodeportid(net_port_id input);

#define CONN_TABLE_SIZE 0x00000020  //up to about (1/2) * 32 connections per sport 
#define CONN_TABLE_MASK 0x0000001F

#define MAX_CONNS 20
#define NUM_RCV_BUFFER_ENTRIES 20 // the number of entries in each of the receive buffers
#define NUM_SND_BUFFER_ENTRIES 20
#define IND_FRAME_SIZE 512*4  //size of eager indirect frames (each frame maintains full-word alignment)
#define NUM_INDIRECT_FRAMES  10  //number of eager indirect frames

struct reg_base_and_len{
	unsigned in_use;  // 1 if region is active
	unsigned base;  // base address for the region 
	unsigned length; // length of the region in bytes
	unsigned process_id;  //not used in single_process emulation
};

// interface cmd_port to hport
struct cmd_port_interface{  
public:
	unsigned this_nodeportid;  
	int fence_bit;		//fence active bit
	unsigned issued_requests; //count of issued requests
	unsigned completed_requests;	// count of completed requests
	ring_buffer rcv_buff;
	lfqueue_s rcv_comp_buffer_w;
	ring_buffer snd_cmd_buff;
	lfqueue<zcompletion_record> snd_comp_buffer; //sender completion buffer
};

//supporting functions
void cmd_port_interface_init(cmd_port_interface* intf, int buff_size, unsigned port_tuple);
int cmd_port_interface_send_imm_message(cmd_port_interface* intf, char* data, unsigned len, unsigned destination, unsigned seqno, int fence);
int cmd_port_interface_get_comp_status(cmd_port_interface* intf, zcompletion_record* result);
int  cmd_port_interface_put(cmd_port_interface* intf, unsigned rem_intf, void* lcl_src_ptr, unsigned rem_region_index, unsigned remote_region_offset, int length, int seqno, int fence);
int  cmd_port_interface_get(cmd_port_interface* intf, unsigned rem_intf, void* lcl_dst_ptr, unsigned rem_region_index, unsigned remote_region_offset, int length, int seqno, int fence);
int  cmd_port_interface_put_wzmmu(cmd_port_interface* intf,  void* lcl_src_ptr, void* remote_dest_ptr, int length, int seqno, int fence);

struct rcv_port_interface{
	unsigned this_nodeportid;
	ring_buffer rcv_buff;
	reg_base_and_len  reg_buff[MAX_REG_BUFFERS];  //hardware table for registered buffers
};

void rcv_port_interface_init(rcv_port_interface* intf, int buff_size, unsigned port_tuple);
int rcv_port_interface_rcv_imm_message(rcv_port_interface* intf, char* data, unsigned* len, unsigned* source);
int  rcv_port_interface_register_mem_region(rcv_port_interface* intf, void* base_pointer, unsigned length); //register a memory region at responder

#else
#endif
