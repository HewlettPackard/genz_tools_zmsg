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

#include "hdwr_loops.h"
#include "gdata.h"
#include <cstdio>
#include "zengine.h"
#include "sport.h"
#include "sim_time.h"
#include "sim_signal.h"

bool add_rcv_port(hport_descr* p, hport_cmd_record* add_rec_ptr){
	rcv_port_interface* rcv_interface_ptr = add_rec_ptr->rcv_interface_ptr;
	int rcv_port_short_id = add_rec_ptr->conn_index;//get internal index for new port
	p->rcv_port_interface_vect[rcv_port_short_id] = rcv_interface_ptr;
	p->rcv_port_in_use_vect[rcv_port_short_id] = true;
	p->num_cmd_ports = add_rec_ptr->num_rcv_ports;
	return true;
}

void process_hport_commands_rcv(hport_descr* p){

	while (is_empty_lfqueue_s(&p->hport_cmd_to_process_rcv_s)==0){
		unsigned cmd_index;
		int success = pop_head_lfqueue_s( &p->hport_cmd_to_process_rcv_s, &cmd_index); 
		hport_cmd_record* cmd_ptr = &p->hport_cmd_vect[cmd_index];

		int command_type = cmd_ptr->type;
		switch (command_type){  
			// add more cases: such as reset the interfaces
		case HPORT_CMD_ADD_CMD_PORT:
			cmd_ptr->done = true;  
			if (LOG_HRDWR_DEBUG == 1) {
				locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " hport_commands_rcv: Added sport to hport :" + IntToStr(p->hport_id));
			}
			break;
		case HPORT_CMD_ADD_RCV_PORT:  // ******************* adding receiver is not right yet
			add_rcv_port(p, cmd_ptr);
			cmd_ptr->done = true;
			if (LOG_HRDWR_DEBUG == 1) {
				locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " hport_commands_rcv: Added rcv_port to hport :" + IntToStr(p->hport_id));
			}
			break;
		}
	}
}

//receive loop polls physical merge queue for incoming traffic
void poll_rcv(hport_descr* p){
	merge_queue* mq_ptr = &gdata.mq_vect[p->hport_id];
	while (true){
		for (int trips = 0; trips < 40; trips++){
			if (is_empty_merge_queue(mq_ptr) == 1) continue;
			mq_record* record_ptr = peek_head_merge_queue(mq_ptr);
			while (record_ptr->ready == 0) {			}; //message not yet complete
			int msg_type = record_ptr->msg.header.type; //extract the command type

			if (LOG_RCV_THREAD == 1) {
				locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_rcv: got message at merge queue of type :" + IntToStr(msg_type));
			}

			// add rcvr processing delay
			if (enable_delay == 1) {
				global_time.sleep((double)delay_rcvr);
			}

			unsigned dest = record_ptr->msg.header.dest_interface_id; //get the message destination tuple
			unsigned dest_sport = get_sport_nodeportid(dest);  //extract the sport index
			if (p->sport_in_use_vect[dest_sport] == false){ //if target interface is disabled  //MSS   fix -- decouple receiver from sender in_use
				if (LOG_RCV_THREAD == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_rcv: packet to unknown sport:" + IntToStr(dest_sport));
				}
				remove_head_merge_queue(mq_ptr); //discard this message
				continue;  
			}
			
			rcv_port_interface* interface = p->rcv_port_interface_vect[dest_sport];
			
			int rcv_index = interface->rcv_buff.alloc();  //get an unused receive buffer index
			while (rcv_index < 0){} // need to add a NACK back to zmsg engine when not enough receive buffers

			int success = 0;  
			switch (msg_type){  //decode the message type
			case MSG_ENQA: //process immediate
				{
					zrecord* new_buff = (zrecord*) interface->rcv_buff.get_buff_pointer(rcv_index);

					new_buff->header = record_ptr->msg.header;
					int body_len = record_ptr->msg.header.len;
					memcpy(&new_buff->data, &record_ptr->msg.data, body_len);  //copy the immediate message

					if (LOG_RCV_THREAD == 1) {
						locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_rcv: copy the immediate data");
					}
					success = interface->rcv_buff.push_tail(rcv_index);  //full queue pushback is not yet right???
					signal_status* rev_signal = record_ptr->reverse_signal;

					if (success){ rev_signal->success = CMDPORT_SUCCESS; }
					 else{ rev_signal->success = CMDPORT_ENQA_FAIL; }
					rev_signal->s.notify();  //notify originating zeng sender of success or fail outcome at receiver
					break;
				}
			}
			if (LOG_RCV_THREAD == 1) {
				locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_rcv: finished processing mque head");
			}
			remove_head_merge_queue(mq_ptr);
		}  //drop out of this loop to check intf settings

		process_hport_commands_rcv(p);
	}
}

//add a new sport (to xmitter)
bool add_cmd_port(hport_descr* p, hport_cmd_record* add_rec_ptr){ 
	cmd_port_interface* cmd_interface_ptr = add_rec_ptr->cmd_interface_ptr;
	int sport_short_id = add_rec_ptr->conn_index;//get internal index for new port
	p->cmd_port_interface_vect[sport_short_id] = cmd_interface_ptr;
	p->sport_in_use_vect[sport_short_id] = true;
	p->num_cmd_ports = add_rec_ptr->num_cmd_ports;
	return true;
}

void process_hport_commands_xmit(hport_descr* p){
	while (is_empty_lfqueue_s(&p->hport_cmd_to_process_cmd_s) == 0){
		unsigned cmd_index;
		int success = pop_head_lfqueue_s(&p->hport_cmd_to_process_cmd_s, &cmd_index); //get value and remove first elem of queue 
		hport_cmd_record* cmd_ptr = &p->hport_cmd_vect[cmd_index];

		int command_type = cmd_ptr->type;
		switch (command_type){
		case HPORT_CMD_ADD_CMD_PORT:
			{
				bool result = add_cmd_port(p, cmd_ptr); //add a new logical command port 
				if (!result) {  
					cmd_ptr->done = true;  //mark as done and return error
					break;
				}
				if (LOG_HRDWR_DEBUG == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " hport_commands_xmit: Added sport to hport :" + IntToStr(p->hport_id));
				}
				// add port at rcvr too
				int success = push_tail_lfqueue_s(&p->hport_cmd_to_process_rcv_s, cmd_index); //insert the last element 
				while (success != TRUE){}
				break;
			}
			case HPORT_CMD_ADD_RCV_PORT:{
				int success = push_tail_lfqueue_s(&p->hport_cmd_to_process_rcv_s, cmd_index); //insert the last element 
				while (success != TRUE){}
				break;
			}
		}
	}
} 

// process messages from logical port queues and assign it to a free zengine
void poll_schd(hport_descr* p){  

	int sport_inx = 0; //logical port index used to poll ports
	while (true){ 
		for (unsigned trips = 0; trips < 40; trips++){
			for (int i = 0; i < p->num_cmd_ports; i++) {
				if (p->sport_in_use_vect[i] == false) continue;  //skip disabled sports  

				cmd_port_interface* interface = p->cmd_port_interface_vect[i];
				unsigned zsrc = interface->this_nodeportid;  //get tuple for this sport interface
				if (interface->snd_cmd_buff.is_empty()){ continue; }
				int cmd_index = interface->snd_cmd_buff.pop_head();
				
				if (enable_delay == 1) { //command peeking delay
					global_time.sleep((double)delay_cmnd_retrieve);
				}
				zrecord* next_record = (zrecord*) interface->snd_cmd_buff.get_buff_pointer(cmd_index);
				// fence command, put interface on fence
	
				if (interface->fence_bit == 1) {
					if (interface->completed_requests != interface->issued_requests) continue;  //remain in fence mode & poll next sport intf
					// if interface is in fence state... make sure all messages are delivered
					else interface->fence_bit = 0;  //drop out of fence mode and continue processing
				}

				//its a new record, send message to zeng
				int found = 0;
				while (found == 0) { // keep scanning until find a free engine
					int count = 0;
					for (int zid = 0; zid < MAX_NUM_ZENG; zid++) { //scan all the engines

						int id = zid + p->hport_id * MAX_NUM_ZENG; // find the engine on this hport
						genz_engine* zeng_ptr = &gdata.zeng_vect[id];
						if (!is_idle_genz_engine(zeng_ptr)) continue;  //skip each non-idle engine

						// pass interface pointer to engine and mark zeng as "RUNNING"
						zeng_ptr->s_intf_id = i;  //passes the interface index to the engine
						zeng_ptr->command = *next_record; //copy the command to the engine
						zeng_ptr->cmd_index = cmd_index;
						set_running_genz_engine(zeng_ptr);

						found = 1; // found a free zeng -- exit and process next cmnd queue
						if (LOG_SCHEDULER == 1) {
							locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_schd found free zeng num: " + IntToStr(id));
						}

						interface->snd_cmd_buff.free(cmd_index);
						interface->issued_requests++;
						break; // resume scanning at the next sport
					}
					count++;
					if (LOG_SCHEDULER == 1) {
						locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_schd exited for loop and found=" + IntToStr(count) + "  " + IntToStr(found));
					}
				}		
			}
		} 
		process_hport_commands_xmit(p);  //check, for any port change commands
	}
}

//polls for a free z-engine and calls the processing function!
void poll_zeng(hport_descr* p, int zid) {

	genz_engine* zeng_ptr = &gdata.zeng_vect[zid];
	if (LOG_ZENG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_zeng thread no:" + IntToStr(zid) + " port_name:" + IntToStr(p->hport_id));
	}

	while (true) {
		if (zeng_ptr->state != ZENG_RUNNING) continue;  // wait until engine enters running state
		//process new command  in running state
		
		if (LOG_ZENG == 1) { locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_zenf: process message"); }
		cmd_port_interface* interface = p->cmd_port_interface_vect[zeng_ptr->s_intf_id];

		unsigned zsrc = interface->this_nodeportid;  //get tuple for this sport interface//		pop_head_lfqueue_s(&interface->snd_cmnd_buffer_w, &next_snd_index);  //remove index to next command from send queue
		zrecord* next_command = &zeng_ptr->command;  //get a pointer to the actual command
		if (LOG_ZENG == 1) {
			locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " genz_eng_proc: processing a message");
		}

		int msg_type = next_command->header.type;
		int dest_intf = next_command->header.dest_interface_id;

		int pdest = get_hport_nodeportid(dest_intf);		//extract physical port index
		int rcv_dest = get_sport_nodeportid(dest_intf);
		merge_queue* dest_mq = &gdata.mq_vect[pdest];	//get pointer to correct destination merge queue
		switch (msg_type) {
			// Insert channel + completion processing delay -- maybe split in to multiple functions?
			if (enable_delay == 1) { global_time.sleep((double)latency); }

			case MSG_ENQA: //process immediate
			{
				int llen = next_command->header.len;
				int len = MIN(llen, Z_RECORD_MTU);//length truncated at immediate mtu
				mq_record* rec_ptr = 0;
				while (rec_ptr==0){ //retry at physical layer until sufficient space to send data
					rec_ptr = insert_header_merge_queue(dest_mq, next_command->header);  //try to insert the header
				}  
				if (LOG_ZENG == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " genz_eng_proc: immediate");
				}

				signal_status sig;  //used to wait for a reply from the remote receiver
				rec_ptr->reverse_signal = &sig;  //send a pointer to this semaphore to the remote receiver
				insert_imm_msg_mq_record(rec_ptr, (char*) &(next_command->data), rec_ptr->msg.header); //data is copied into ring after ring tail is released

				sig.s.wait();  //wait for downstream receiver to reply
				//wait until remote receiver replies before reporing succcess

				zeng_ptr->cmd_status = sig.success; //status set to status outcome from rcv engine
				break;//process next message
			}

			case MSG_NOOP:
			{
				zeng_ptr->cmd_status = CMDPORT_SUCCESS; //status set to: SUCCEED
				break;
			}

			case MSG_PUT: //process put command
			{
				unsigned src_ptr = next_command->header.local_address; //extract fields from command
				unsigned dest_intf = next_command->header.dest_interface_id;  //tuple identifies remote bridge and  port 
				unsigned rem_region_index = next_command->header.remote_region_index;  //index to memory region at remote interface
				unsigned copy_len = next_command->header.len; //length of copy
				unsigned rem_region_offset = next_command->header.remote_region_offset; 
				unsigned src_addr = next_command->header.local_address;
				//need to add an RKEY along with its check

				unsigned bridge_id = get_hport_nodeportid(dest_intf);  //find the remote interface
				unsigned rcv_port_id = get_sport_nodeportid(dest_intf);
				hport*  remote_bridge_ptr = gdata.ccmgr.get_port_ptr("main_clique", bridge_id);
				rcvport* receiver = remote_bridge_ptr->get_rcv_port_pointer(rcv_port_id);

				while (rem_region_index >= MAX_REG_BUFFERS) {} // window index bounds check
				reg_base_and_len window_bounds = receiver->get_intf()->reg_buff[rem_region_index];  //get destination window base and bounds
				unsigned in_use = window_bounds.in_use;
				if (in_use==0){ //region is not registered return fail
					zeng_ptr->cmd_status = CMDPORT_REMOTE_MEM_UNREGISTERED;
					break;
					}
				unsigned window_base = window_bounds.base;
				unsigned window_length = window_bounds.length;
				if ( (rem_region_offset+copy_len) > window_length ) {  //copy extends past end of window
					zeng_ptr->cmd_status = CMDPORT_REMOTE_MEM_OUTOFBOUNDS;
					break;
				}
				unsigned dst_start_ptr = window_base + rem_region_offset;
				memcpy((void*)dst_start_ptr, (void*)src_ptr, copy_len);  //copy the put
				if (LOG_ZENG == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " genz_eng_proc: MSG_PUT");
				}
				zeng_ptr->cmd_status = CMDPORT_SUCCESS; //
				break;//process next message		  
			}
			case MSG_GET: //process put command
			{
				unsigned dst_ptr = next_command->header.local_address; //extract fields from command
				unsigned dest_intf = next_command->header.dest_interface_id;  //tuple identifies remote bridge and  port 
				unsigned rem_region_index = next_command->header.remote_region_index;  //index to memory region at remote interface
				unsigned copy_len = next_command->header.len; //length of copy
				unsigned rem_region_offset = next_command->header.remote_region_offset;
				unsigned src_addr = next_command->header.local_address;
				//need to add an RKEY along with its check

				unsigned bridge_id = get_hport_nodeportid(dest_intf);  //find the remote interface
				unsigned rcv_port_id = get_sport_nodeportid(dest_intf);
				hport*  remote_bridge_ptr = gdata.ccmgr.get_port_ptr("main_clique", bridge_id);
				rcvport* receiver = remote_bridge_ptr->get_rcv_port_pointer(rcv_port_id);

				while (rem_region_index >= MAX_REG_BUFFERS) {} // window index bounds check
				reg_base_and_len window_bounds = receiver->get_intf()->reg_buff[rem_region_index];  //get destination window base and bounds
				unsigned in_use = window_bounds.in_use;
				if (in_use == 0){ //region is not registered return fail
					zeng_ptr->cmd_status = CMDPORT_REMOTE_MEM_UNREGISTERED;
					break;
				}

				unsigned window_base = window_bounds.base;
				unsigned window_length = window_bounds.length;
				if ((rem_region_offset + copy_len) > window_length) {  //copy extends past end of window
					zeng_ptr->cmd_status = CMDPORT_REMOTE_MEM_OUTOFBOUNDS;
					break;
				}
				unsigned src_start_ptr = window_base + rem_region_offset;
				memcpy( (void*) dst_ptr, (void*)src_start_ptr, copy_len);  //copy the put
				if (LOG_ZENG == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " genz_eng_proc:  MSG_GET");
				}
				zeng_ptr->cmd_status = CMDPORT_SUCCESS; //
				break;//process next message		  
			}
			case MSG_PUT_WZMMU: //process put command using the ZMMU
			{
				//need to add an RKEY along with its check

				unsigned src_addr = next_command->header.local_address; //extract fields from command
				unsigned requested_len = next_command->header.len; //length of copy
				unsigned dest_addr = next_command->header.remote_address;

				//find the local ZMMU and perform a lookup
				zmmu* requesting_zmmu = &p->requesting_zmmu;
				zmmu_table_entry* req_table_entry = requesting_zmmu->find_matching_entry(dest_addr);
				while (req_table_entry == 0) {} //no legal translation -- need to add error processing

				//calculate a gen-z starting address and length
				unsigned gen_z_start_addr = req_table_entry->get_target_address(src_addr);
				unsigned src_copy_lengh = req_table_entry->get_copy_length(src_addr, requested_len); //do not copy past end of requesting window
				int remote_cid = req_table_entry->req.remote_cid;
				//find the remote ZMMU

				hport*  remote_bridge_ptr = gdata.ccmgr.get_port_ptr("main_clique", remote_cid);
				hport_descr* remote_hport_descr = remote_bridge_ptr->get_descriptor_pointer();
				zmmu* responding_zmmu = &remote_hport_descr->responding_zmmu;
				//calculate a destination starting address and length

				zmmu_table_entry* resp_table_entry = responding_zmmu->find_matching_entry(gen_z_start_addr);
				while (resp_table_entry == 0) {} //no legal translation -- need to add error processing
				unsigned dst_start_addr = resp_table_entry->get_target_address(gen_z_start_addr);
				unsigned dst_copy_length = resp_table_entry->get_copy_length(gen_z_start_addr,src_copy_lengh);

				memcpy((void*)dst_start_addr, (void*)src_addr, dst_copy_length);  //copy the put

					if (LOG_ZENG == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " genz_eng_proc: MSG_PUT_WZMMU");
				}
				zeng_ptr->cmd_status = CMDPORT_SUCCESS; //
				break;//process next message
			}
		}
		set_zeng_done(zeng_ptr); // z engine is finished processing job
	}	//end of while true
}

void poll_cmp(hport_descr* p) {
	
	while (true) { // keep scanning until find a free engine
		int count = 0;
		for (int count = 0; count < 10000; count++){
			for (int zid = 0; zid < MAX_NUM_ZENG; zid++) { //scan all the engines on this port

				// find the engine on this hport, make it a function
				int id = zid + p->hport_id * MAX_NUM_ZENG;
				genz_engine* zeng_ptr = &gdata.zeng_vect[id];

				if (!is_done_genz_engine(zeng_ptr)) continue;

				// generate a completion in the appropritate completion queue
				int sport_id = get_s_intf(zeng_ptr);
				int cmd_status = get_zeng_cmd_status(zeng_ptr);
				cmd_port_interface* interface = p->cmd_port_interface_vect[sport_id];

				interface->completed_requests--;
				zcompletion_record rec;
				rec.seq_num = zeng_ptr->command.seq_num;
				rec.status = cmd_status; //  zeng_ptr->status;

				int success_flag = interface->snd_comp_buffer.push_tail(rec);
				zassert(success_flag == 1);

				if (enable_delay == 1) {
					global_time.sleep((double)delay_comp);
				}

				set_idle_genz_engine(zeng_ptr); //sets the ready bit to 1 again

				if (LOG_COMP_ENG == 1) {
					locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_cmp in for loop" + IntToStr(zeng_ptr->cmd_status) + "num" + IntToStr(id));
				}
				break; // break out of loop, finish scanning
			}
		}
		if (LOG_COMP_ENG == 1) {
			locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " poll_cmp exited for loop ");
		}
	}
}