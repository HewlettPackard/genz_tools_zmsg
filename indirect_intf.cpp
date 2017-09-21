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

#include "indirect_intf.h"
#include "zrecord.h"
#include "tools.h"

#define INDIRECT_BUFFER_COUNT 20  //number of indirect buffers initialized at each interface

indirect_intf::indirect_intf(string intf_name, hport* bridge) :rcv_buffer_list(100){ // bidirectional indirect interface uses a command port and a receive port
	name = intf_name;
	this_bridge = bridge;
	cmd_port_name = name + "_CMD";
	rcv_port_name = name + "_RCV";
	seqno = 0;

	int error_return;
	cmd_port = this_bridge->request_add_cmd_port(cmd_port_name, &error_return);
	while (cmd_port == 0){};
	cmd_port_id = cmd_port->get_port_id();

	receive_port = this_bridge->request_add_rcv_port(rcv_port_name, &error_return);
	while (receive_port == 0){};
	receive_port_id = receive_port->get_port_id();
	indirect_frames.init(INDIRECT_BUFFER_COUNT, ZRECORD_SIZE);
	for (int i = 0; i < INDIRECT_BUFFER_COUNT; i++){
		avail_local_credits.push_back(i);  // all credits are initially available
	}
	for (int i = 0; i < MAX_IND_CONNECTIONS; i++){
		connection_is_valid[i] = 0;
	}

	int window_id = receive_port->register_mem_region("put_winow", indirect_frames.get_buff_pointer(0), indirect_frames.get_length_buff());
	while (window_id < 0){}
	local_put_window_id = window_id;
	put_window_frame_size = indirect_frames.get_frame_size();
}

int indirect_intf::rcv_ind_message(char* data, unsigned* len, unsigned* source){
	process_incoming_messages();
	if (rcv_buffer_list.is_empty()) return 0; //nothing to deliver
	int rcv_index = *rcv_buffer_list.peek_head(); //get the head entry
	rcv_buffer_list.pop_head();  //remove it
	zrecord* msg_ptr = (zrecord*) indirect_frames.get_buff_pointer(rcv_index);
	*len = msg_ptr->header.len;
	unsigned source_id = msg_ptr->header.src_interface_id;
	int conn_id = xlate_remote_send_address_to_conn_id[source_id];
	*source = source_id;
	memcpy(data, &msg_ptr->data, msg_ptr->header.len);
	list<int> credits;
	credits.push_back(rcv_index);
	send_credits_to_remote(conn_id, credits);  //send credits back to remote interface
	return 1; //delivered a message
}

void indirect_intf::process_incoming_messages(){
	int done = 0;
	while (!done){
		indirect_message rcv_buf;
		unsigned rcv_length;
		int rcv_msg;
		unsigned remote_id;
		rcv_msg = receive_port->rcv_imm_message((char*)&rcv_buf, &rcv_length, &remote_id);
		if (!rcv_msg) return; //no messages to process yet
		net_port_id src_addr = net_port_id(remote_id);
		int conn_id = xlate_remote_send_address_to_conn_id[src_addr];

		indirect_message_header* header_ptr = &rcv_buf.header;
		int op_code = header_ptr->op_code;
		int length = header_ptr->length;

		switch(op_code){
		case IND_OP_RET_LOCAL_CREDIT:
			for (int i = 0; i < length; i++){
				int credit_index = rcv_buf.index_vector[i]; 
				avail_local_credits.push_back(credit_index);
			}
			break;
		case IND_OP_RET_REM_CREDIT:  //return credits into the per-connectio remote credit pool
			for (int i = 0; i < length; i++){
				int credit_index = rcv_buf.index_vector[i];
				remote_credits[conn_id].push_back(credit_index);
			}
			break;
		case IND_OP_REQ_CREDIT:
			{
			indirect_message msg;
			int desired_length = MIN(header_ptr->length, MAX_NUM_INDIRECT_FRAMES);
			int actual_length = 0;
			for (int i = 0; i < desired_length; i++){
				if (avail_local_credits.empty()) break;
				actual_length++;
				msg.index_vector[i] = avail_local_credits.front();
				avail_local_credits.pop_front();
				}
			msg.header.op_code = IND_OP_RET_REM_CREDIT;
			msg.header.length = actual_length;
			int seqno = -1;
			int snd_done = 0;
			while (snd_done <= 0){
				snd_done = cmd_port->send_imm_message((char*)&msg, sizeof(indirect_message), src_addr, seqno, 0); //wait until a message can be sent
				}
			}
			break;
		case IND_OP_SEND_SEQ_MSGS:
			for (int i = 0; i < length; i++){
				rcv_buffer_list.push_tail(rcv_buf.index_vector[i]);
				}
			break;
		}
	}
}

int indirect_intf::send_ind_message(char* data, int len, int conn_id){

	process_incoming_messages();  //gather any potential send credits
	unsigned dest_id = connection_table[conn_id].dest_address;
	if (remote_credits[conn_id].empty()) return 0;
	int frame_index = remote_credits[conn_id].front();
	remote_credits[conn_id].pop_front();

	indirect_message send_msg;
	int seqno = 0;

	zrecord msg;
	msg.header.len = len;
	msg.header.type = MSG_ENQA;
	msg.header.src_interface_id = connection_table[conn_id].src_address;
	msg.header.dest_interface_id = connection_table[conn_id].dest_address;
	int copy_len = MIN(len, Z_RECORD_MTU);
	memcpy(&msg.data, data, copy_len);

	int frame_len = sizeof(zrecord)-(Z_RECORD_MTU - len);
	char* snd_pointer = indirect_frames.get_buff_pointer(frame_index);
	int success = cmd_port->put(dest_id, &msg, remote_window_id[conn_id], remote_window_frame_size[conn_id] * frame_index, frame_len, seqno, 0);
	while (success <= 0) {}
	zcompletion_record compl;
	int result = 0;
	while (result == 0){
		result = cmd_port->get_comp_status(&compl);
	}
	while (compl.status != 1){}

	send_msg.index_vector[0] = frame_index;
	send_msg.header.length = 1;
	send_msg.header.op_code = IND_OP_SEND_SEQ_MSGS;

	success = cmd_port->send_imm_message((char*)&send_msg, sizeof(indirect_message), dest_id, seqno, 0);
	while (success <= 0){}

	result = 0;
	while (result == 0){
		result = cmd_port->get_comp_status(&compl);
	}
	return 1;
}

/*  send a sequence of indirect messages  (unfinished)

int indirect_intf::send_seq_ind_messages(list<char*> data, list<int> len, int conn_id){
	//send a sequence of indirect messages
	//returns the actual number sent which may be less than the number requested
	list<char*> src = data;

	process_incoming_messages();  //gather any potential send credits
	unsigned dest_id = connection_table[conn_id].dest_address;

	int num_sent = 0;

	indirect_message send_msg;
	list<int>::iterator src_len_it = len.begin();

	for (list<char*>::iterator it = src.begin(); it != src.end(); it++){
		char* src_ptr = *it;
		if (src_len_it == len.end())  break;
		int len = *src_len_it;
		src_len_it++;

		if (remote_credits[conn_id].empty() || num_sent >= MAX_NUM_INDIRECT_FRAMES) break;
		int frame_index = remote_credits[conn_id].front();
		remote_credits[conn_id].pop_front();

		char* snd_pointer = indirect_frames.get_buff_pointer(frame_index);
		int seqno = 0;
		cmd_port->put(dest_id, src_ptr, remote_window_id[conn_id], remote_window_frame_size[conn_id] * frame_index, len, seqno, 0);
		send_msg.index_vector[num_sent] = frame_index;
		num_sent ++;
	}

	for (int i = 0; i < num_sent; i++){
		zcompletion_record compl;
		int result = 0;
		while (result == 0){
			cmd_port->get_comp_status(&compl);
		}
	}

	send_msg.header.length = num_sent;
	send_msg.header.op_code = IND_OP_SEND_SEQ_MSGS;
//	send_msg.header.src_id = cmd_port_id;
	int seqno = 0;
	cmd_port->send_imm_message((char*)&send_msg, sizeof(indirect_message), dest_id, seqno, 0);
	zcompletion_record compl;
	int result = 0;
	while (result == 0){
		cmd_port->get_comp_status(&compl);
	}
	return num_sent;
}
*/

list<int> indirect_intf::get_avail_local_credits(int num_frames){
	list<int> result;
	for (int i = 0; i < num_frames; i++){
		if (avail_local_credits.empty()) break;
		int frame_index = avail_local_credits.front();
		result.push_back(frame_index);
		avail_local_credits.pop_front();
	}
	return result;
}

net_port_id indirect_intf::get_return_address(){
	return cmd_port_id;
}

net_port_id indirect_intf::get_target_address(){
	return receive_port_id;
}

void indirect_intf::add_remote_credits(int conn_id, list<int> in_credits){
	list<int>* credit_list = &remote_credits[conn_id];
	while (!in_credits.empty()){
		credit_list->push_back(in_credits.front()); //add  credits to remote credits pool
		in_credits.pop_front();
	}
}

dual_address_tuple  indirect_intf::get_dual_address(){
	dual_address_tuple result;
	result.dest_address = receive_port_id;
	result.src_address = cmd_port_id;
	return result;
}

int indirect_intf::get_put_window_id(){
	return local_put_window_id;
}

int indirect_intf::get_put_window_frame_size(){
	return put_window_frame_size;
}

int indirect_intf::add_connection(dual_address_tuple remote_address, int remote_window_id_in, int frame_size){
	lock.lock();
	for (int i = 0; i < MAX_IND_CONNECTIONS; i++){
		if (!connection_is_valid[i]){  //look for unused connection
			connection_table[i] = remote_address;
			xlate_remote_send_address_to_conn_id[remote_address.dest_address] = i;
			xlate_remote_rcv_address_to_conn_id[remote_address.src_address] = i;
			remote_credits[i].clear();
			remote_window_id[i] = remote_window_id_in;
			remote_window_frame_size[i] = frame_size;
			num_connections++;
			connection_is_valid[i] = true;
			lock.unlock();
			return i;
		}
	}
	lock.unlock();
	return -1;  //couldn't add connection
}

int indirect_intf::send_credits_to_remote(int conn_id, list<int> indexes){  //add credits for a remote interface
	indirect_message msg;
	int num_sent = 0;

	unsigned rem_id = connection_table[conn_id].dest_address;
	for (list<int>::iterator it = indexes.begin(); it != indexes.end(); it++){
		if (num_sent >= MAX_NUM_INDIRECT_FRAMES) break;
		int index = *it;
		msg.index_vector[num_sent] = index;
		num_sent++;
	}
	msg.header.op_code = IND_OP_RET_REM_CREDIT;
	msg.header.length = num_sent;
	int done = 0;
	int seqno = 0;
	int fence = 0;
	while (done <= 0){
		done = cmd_port->send_imm_message((char*)&msg, sizeof(indirect_message), rem_id, seqno, fence); //wait until command can be placed into the command queue
	}

	zcompletion_record result;  //Location for Completion Result
	done = 0;
	while( done <= 0){
		done = cmd_port->get_comp_status(&result); //wait until a completion can be retrieved
	}
	return num_sent;
}
