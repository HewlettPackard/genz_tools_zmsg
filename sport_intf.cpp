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

#include "sport_intf.h"
#include "tools.h"
#include "hport_intf.h"

void cmd_port_interface_init(cmd_port_interface* intf, int buff_size, unsigned port_tuple){

	intf->this_nodeportid = port_tuple;
	intf->fence_bit = 0; //fence is inactive, activated upon seeing a fence message
	intf->completed_requests = 0;
	intf->issued_requests = 0;
	intf->snd_cmd_buff.init(NUM_SND_BUFFER_ENTRIES, ZRECORD_SIZE);
	intf->rcv_buff.init(NUM_RCV_BUFFER_ENTRIES, ZRECORD_SIZE);
	init_lfqueue_s(&intf->rcv_comp_buffer_w);
	intf->snd_comp_buffer.init(NUM_SLOTS);
}

int cmd_port_interface_send_imm_message(cmd_port_interface* intf, char* data, unsigned len, unsigned destination, unsigned seqno, int fence){  //change to operation on intf*********
	int rec_index = intf->snd_cmd_buff.alloc();
	if (rec_index == -1) return CMDPORT_NO_BUFFERS;  //ran out of command buffers
	zrecord* send_buffer_ptr = (zrecord*)intf->snd_cmd_buff.get_buff_pointer(rec_index);
	send_buffer_ptr->header.src_interface_id = intf->this_nodeportid;
	send_buffer_ptr->header.dest_interface_id = destination;

	int body_len = MIN(Z_RECORD_MTU, len);
	send_buffer_ptr->header.len = body_len;
	send_buffer_ptr->header.type = MSG_ENQA;
	send_buffer_ptr->seq_num = seqno;
	send_buffer_ptr->fence = fence;

	memcpy(&send_buffer_ptr->data, data, body_len);  //copy both header and body

	intf->snd_cmd_buff.push_tail(rec_index);
	return CMDPORT_SUCCESS;
}

int cmd_port_interface_get_comp_status(cmd_port_interface* intf, zcompletion_record* result) {
	if (intf->snd_comp_buffer.is_empty()) return 0;
	*result = intf->snd_comp_buffer.pop_head();
	return  1;
}

int  cmd_port_interface_put(cmd_port_interface* intf, unsigned rem_intf, void* lcl_src_ptr, unsigned rem_region_index,
	unsigned rem_region_offset, int len, int seqno, int fence){

	int rec_index = intf->snd_cmd_buff.alloc();
	if (rec_index == -1) return CMDPORT_NO_BUFFERS;  //ran out of send buffers
	zrecord* send_buffer_ptr = (zrecord*)intf->snd_cmd_buff.get_buff_pointer(rec_index);

	send_buffer_ptr->header.dest_interface_id = rem_intf;
	send_buffer_ptr->header.type = MSG_PUT;
	send_buffer_ptr->header.local_address = (unsigned) lcl_src_ptr;
	send_buffer_ptr->header.remote_region_index = rem_region_index;
	send_buffer_ptr->header.remote_region_offset = rem_region_offset;
	send_buffer_ptr->header.len = len;
	send_buffer_ptr->seq_num = seqno;
	send_buffer_ptr->fence = fence;
	intf->snd_cmd_buff.push_tail(rec_index);
	return CMDPORT_SUCCESS;
}

int  cmd_port_interface_get(cmd_port_interface* intf, unsigned rem_intf, void* lcl_dst_ptr, unsigned rem_region_index,
	unsigned rem_region_offset, int len, int seqno, int fence){

	int rec_index = intf->snd_cmd_buff.alloc();
	if (rec_index == -1) return CMDPORT_NO_BUFFERS;  //ran out of send buffers
	zrecord* send_buffer_ptr = (zrecord*)intf->snd_cmd_buff.get_buff_pointer(rec_index);
	send_buffer_ptr->header.dest_interface_id = rem_intf;

	send_buffer_ptr->header.type = MSG_GET;
	send_buffer_ptr->header.local_address = (unsigned) lcl_dst_ptr;
	send_buffer_ptr->header.remote_region_index = rem_region_index;
	send_buffer_ptr->header.remote_region_offset = rem_region_offset;
	send_buffer_ptr->header.len = len;
	send_buffer_ptr->seq_num = seqno;
	send_buffer_ptr->fence = fence;
	intf->snd_cmd_buff.push_tail(rec_index);
	return CMDPORT_SUCCESS;
}

int  cmd_port_interface_put_wzmmu(cmd_port_interface* intf, void* lcl_src_ptr, void* remote_dest_ptr, int length, int seqno, int fence) {
	int rec_index = intf->snd_cmd_buff.alloc();
	if (rec_index == -1) return CMDPORT_NO_BUFFERS;  //ran out of send buffers
	zrecord* send_buffer_ptr = (zrecord*)intf->snd_cmd_buff.get_buff_pointer(rec_index);

	send_buffer_ptr->header.local_address = (unsigned) lcl_src_ptr;
	send_buffer_ptr->header.remote_address = (unsigned) remote_dest_ptr;
	send_buffer_ptr->header.len = length;
	send_buffer_ptr->seq_num = seqno;
	send_buffer_ptr->fence = fence;
	intf->snd_cmd_buff.push_tail(rec_index);
	return CMDPORT_SUCCESS;

}


void rcv_port_interface_init(rcv_port_interface* intf, int buff_size, unsigned port_tuple){

	intf->this_nodeportid = port_tuple;
	intf->rcv_buff.init(NUM_RCV_BUFFER_ENTRIES, ZRECORD_SIZE);

	for (int i = 0; i < MAX_REG_BUFFERS; i++){
		intf->reg_buff[i].in_use = 0;
	}
}

int  rcv_port_interface_register_mem_region(rcv_port_interface* intf,  void* base_ptr, unsigned length){ //register a memory region

	for (int i = 0; i < MAX_REG_BUFFERS; i++){
		volatile reg_base_and_len* entry_ptr = &(intf->reg_buff[i]);
		if (entry_ptr->in_use == 0){ //find an unused entry
			entry_ptr->base = (unsigned) base_ptr;
			entry_ptr->length = length;
			entry_ptr->in_use = 1;  //mark as active
			return i;  //return index of the new entry
		}
	}
	return -1;  //returns index -1 if fail
}



int  rcv_port_interface_register_mem_region_respzmmu(rcv_port_interface* intf, void* base_ptr, unsigned length) { //register a memory region
	//hport_intf* this_bridge = intf->
	//not done
	return 0;


}


int rcv_port_interface_rcv_imm_message(rcv_port_interface* intf, char* result_data, unsigned* len, unsigned* source){
	int rcv_index = intf->rcv_buff.pop_head();
	if (rcv_index < 0) return 0; //fail
	zrecord* rcv_buffer_ptr = (zrecord*)intf->rcv_buff.get_buff_pointer(rcv_index);

	int rem_intf = rcv_buffer_ptr->header.dest_interface_id;
	int msg_len = rcv_buffer_ptr->header.len;
	memcpy(result_data, &rcv_buffer_ptr->data, msg_len);
	*source = rem_intf;  //return remote interface id
	*len = msg_len; //return message length
	intf->rcv_buff.free(rcv_index);
	return 1; //success
}