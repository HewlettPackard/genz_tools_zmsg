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

#include "test_genz_sys.h"
#include "gdata.h"
#include "sport.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <stdio.h>
#include "hdwr_service.h"
#include "sim_time.h"
#include "indirect_intf.h"

using namespace std;

// used to send the message
union user_msg_data
{
	int seqno;  
	char buffer[100];
};


/*  ****** Test module for immediate mode data test ******* */
static volatile unsigned num_urcv_init = 0;  //count user receive port initializations
static volatile unsigned num_usnd_init = 0;  //count user send port initializations
static volatile unsigned num_user_initialized = 0;  //count user initializations

void user_rcv_thread(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;

	int threadno = arg->thread_id;
	int rem_thread = (threadno + NUM_SENDERS - 1) % NUM_SENDERS;  //set up circular communication test
	string this_hport_name = "P" + IntToStr(threadno); //names the physical port
	string rem_hport_name = "P" + IntToStr(rem_thread);
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name);  // in this test, snd and rcv threads share a common port

	int error_number;
	rcvport* rcv_port_ptr = port_ptr->request_add_rcv_port("RCV", &error_number /*, arg->pdsc*/); //add a new receive port to this physical port

	fetch_add(num_urcv_init, 1);	//signal that local ports on receivers are initialized

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_thread no: " + IntToStr(threadno) + " added port name: " + "RCV");
	}

	while (num_usnd_init < NUM_SENDERS){} //wait until senders have initialized their local ports

	fetch_add(num_user_initialized, 1);  //signal that remote ports on receivers are initialized
	int prior_seqno = -1;
	int cum_errors = 0;
	user_msg_data message;
	int sample_size = 1000;
	print_clock timer;

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_thread no:" + IntToStr(threadno) + " starting rcvr test loop");
	}

	//this is the receiver's test loop
	while (true){
		for (int i = 0; i < sample_size; i++){
			unsigned length;
			unsigned source;
			bool done = false;
			while (!done){
				done = rcv_port_ptr->rcv_imm_message(message.buffer, &length, &source);
			}

			if (LOG_DEBUG == 1) {
				locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_thread: got packet no:" + IntToStr(message.seqno));
			}

			int seqno = message.seqno;
			if (seqno != (prior_seqno + 1)) cum_errors++;
			prior_seqno = seqno;

			// Insert data rcv delay
			if (enable_delay == 1) {
				global_time.sleep((double)delay_rcv_client);
			}
		}
		if (LOG_TEST_RESULTS == 1) {
			gdata.print_lock.lock();
			double durr_seconds = timer.print_diff("time_difference: ", *logfile);
			*logfile << "T:" << threadno << "  " << (sample_size / durr_seconds) << " msgs/sec" << " cum_errors=" << cum_errors << endl;
			gdata.print_lock.unlock();
		}
	}
}

void user_snd_thread(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
	int rem_thread = (threadno + 1) % NUM_SENDERS;

	string this_hport_name = "P" + IntToStr(threadno); //names the physical port
	string rem_hport_name = "P" + IntToStr(rem_thread);
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name); //get a handle to this physical port

	int error_return;
	cmdport* cmdport_ptr = port_ptr->request_add_cmd_port("CMD", &error_return /*, arg->pdsc*/); //ask to create a logical port
	fetch_add(num_usnd_init, 1);  //indicate that user send port initialization is complete
	while (num_urcv_init < NUM_SENDERS){}  //wait until receivers define all remote user ports
	std::cout << "time " << DoubleToStr(global_time.get_time()) << "added cmd port " << "SND" << endl;

	while (gdata.ccmgr.hport_is_defined("main_clique", rem_hport_name) == FALSE){};  //remote node name not defined
	int remote_hport_index = gdata.ccmgr.get_port_inx("main_clique", rem_hport_name);
	hport* remote_bridge = gdata.ccmgr.get_port_ptr("main_clique", rem_hport_name);
	int remote_receive_port_index = remote_bridge->get_rcv_port_index("RCV");  //get the remote receive interface index out of the remote h_port
	while (remote_bridge<0){};  //loop if receiver interface not defined
	unsigned rem_id = make_nodeportid(remote_hport_index, remote_receive_port_index); //make a proper target address for send

	while (rem_id < 0){};  //this better be successful
	int seqno = 0;  // 
	user_msg_data message;
	int num_msgs = 100;
	while (num_user_initialized < NUM_SENDERS){}

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_snd_thread no:" + IntToStr(threadno) + " starting sndr test loop");
	}

	int num_sent = 0;
	int num_completed = 0;
	int fence = 1;

	//this is the sender's test loop
	for (int i = 0; i < num_msgs; i++){ //loop to send the actual messages
		int length = 4;
		message.seqno = seqno;
		int done = 0;
		while (done <= 0){
			done = cmdport_ptr->send_imm_message(message.buffer, length, rem_id, message.seqno, fence); //wait until command can be placed into the command queue
		}
		num_sent++;

		if (LOG_DEBUG == 1) {
			locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_snd_thread: sent packet seq_num: " + IntToStr(seqno));
		}

		while (num_sent - num_completed > 5){  //allow at most 5 outstanding sends
			zcompletion_record result;  //Location for Completion Result
			done = cmdport_ptr->get_comp_status(&result); //wait until a completion can be retrieved
			if (done == 1){
				num_completed++;
				//process any errors here
				while (result.status != CMDPORT_SUCCESS){};  //assert message send succeeded
			}
		}

		seqno++;
		// Insert command insertion delay
		if (enable_delay == 1) {
			global_time.sleep((double)delay_cmnd_insert);
		}
	}

	while (num_sent - num_completed > 0){  //allow 5 outstanding sends
		zcompletion_record result;  //Location for Completion Result
		int done = cmdport_ptr->get_comp_status(&result); //wait until a completion can be retrieved
		if (done == 1){
			num_completed++;
			//process any errors here
			while (result.status != CMDPORT_SUCCESS){};  //assert message send succeeded
		}
	}
}

void test_genz_sys_sndrcv(){	//main thread launches all services
	thread_arg temp_arg;
	user_arg_vect.resize(NUM_SENDERS, temp_arg);  //separate input argument for each thread
	start_hdwr_svcs(); // Start hardware service
	vector<std::thread> threads; //container for all SOC threads
	unsigned thread_id = 0;
	// Start test using NUM_CLIENTS user receive and transmit threads per SOC
	for (unsigned socid = 0; socid < NUM_SOCS; socid++){
		for (unsigned ind = 0; ind < NUM_CLIENTS; ind++) {
			thread_id = socid * NUM_CLIENTS + ind;
			user_arg_vect[thread_id].thread_id = thread_id;
			user_arg_vect[thread_id].pdsc = &hsvc_pdsc[socid];
			threads.push_back(std::thread(user_rcv_thread, &user_arg_vect[thread_id]));
			threads.push_back(std::thread(user_snd_thread, &user_arg_vect[thread_id]));
		}
	}

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " test_genz_sys created clients: " + IntToStr(NUM_CLIENTS));
	}

	// Wait for the user threads to finish (add termination condition)
	int inx = 0;
	for (unsigned socid = 0; socid < NUM_SENDERS; socid++){ //join on user threads
		threads[inx++].join();
	}
}

static volatile unsigned pri_thread = 0;
static volatile unsigned sec_thread = 0;

/**************** Ping pong test *******************/

void user_pri_ping_pong_thread(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
	int rem_thread = (threadno + 1) % NUM_SOCS;
	string this_hport_name = "P" + IntToStr(threadno);
	string rem_hport_name = "P" + IntToStr(rem_thread);
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name);

	int error_reply;
	cmdport* cmdport_ptr = port_ptr->request_add_cmd_port("CPRI", &error_reply /*, arg->pdsc*/); //add a new logical port to this physical port
	while (cmdport_ptr == 0){};
	rcvport* rcvport_ptr = port_ptr->request_add_rcv_port("RPRI", &error_reply /*, arg->pdsc*/ ); //add a new logical port to this physical port
	while (cmdport_ptr == 0){};

	fetch_add(pri_thread, 1); //signal that primary thread has defined its local port
	while (sec_thread < NUM_SOCS){}  //must wait until secondary ports are defined

	fetch_add(num_user_initialized, 1);
	user_msg_data message;

	int seqno = 0;
	int num_msgs = 100000000;
	int sample_size = 100;

	int prior_seqno = 0;
	int cum_errors = 0;
	user_msg_data rcv_buf;
	while (sec_thread == 0){};  //wait until scondary's remote ports are defined

	while (gdata.ccmgr.hport_is_defined("main_clique", rem_hport_name) == FALSE){};  //remote node name not defined
	int remote_hport_index = gdata.ccmgr.get_port_inx("main_clique", rem_hport_name);
	hport* remote_bridge = gdata.ccmgr.get_port_ptr("main_clique", rem_hport_name);
	int remote_receive_port_index = remote_bridge->get_rcv_port_index("RSEC");  //get the remote receive interface index out of the remote h_port
	while (remote_bridge<0){};  //receiver interface not defined
	unsigned rem_id = make_nodeportid(remote_hport_index, remote_receive_port_index); //make a proper target address
	int fence = 0;
	print_clock timer;
	for (int i = 0; i < num_msgs; i++){ //loop to send the actual messages
		for (int i = 0; i < sample_size; i++){

			unsigned send_length = 4;
			message.seqno = seqno;
			int send_done = 0;
			while (send_done <= 0){
				send_done = cmdport_ptr->send_imm_message(message.buffer, send_length, rem_id, message.seqno, fence); //wait until a message can be sent
			}

			zcompletion_record result;
			int comp_done = 0;
			while (comp_done <= 0){
				comp_done = cmdport_ptr->get_comp_status(&result); //remove completion
			}

			unsigned rcv_length;
			unsigned source;
			bool rcv_done = false;
			while (!rcv_done){  //wait until read is ready
				rcv_done = rcvport_ptr->rcv_imm_message(rcv_buf.buffer, &rcv_length, &source);
			}
			seqno = rcv_buf.seqno;
			if (seqno != (prior_seqno + 1)) 
				cum_errors++;
			prior_seqno = seqno;
		}

		if (LOG_TEST_RESULTS == 1) {
			gdata.print_lock.lock();
			double durr_seconds = timer.print_diff("time_difference: ", *logfile);
			*logfile << " latency=" << (durr_seconds / (2 * sample_size)) << " cum_errors=" << cum_errors << " seq" << seqno << endl;
			gdata.print_lock.unlock();
		}
	}
}

// secondary pong thread
void user_sec_ping_pong_thread(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
	int dest_thread = (threadno + NUM_SOCS - 1) % NUM_SOCS;

	string this_hport_name = "P" + IntToStr(threadno); //names the physical port
	string rem_hport_name = "P" + IntToStr(dest_thread);

	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name); //get a handle to this physical port
	while (pri_thread < NUM_SOCS){};  //must wait until all primary ports are defined

	int error_number;
	cmdport* cmdport_ptr = port_ptr->request_add_cmd_port("CSEC", &error_number /*, arg->pdsc */); //ask to create a logical port
	rcvport* rcvport_ptr = port_ptr->request_add_rcv_port("RSEC", &error_number /*, arg->pdsc*/); //ask to create a logical port

	while (gdata.ccmgr.hport_is_defined("main_clique", rem_hport_name) == FALSE){};  //remote node name not defined
	int remote_hport_index = gdata.ccmgr.get_port_inx("main_clique", rem_hport_name);
	hport* remote_bridge = gdata.ccmgr.get_port_ptr("main_clique", rem_hport_name);
	int remote_receive_port_index = remote_bridge->get_rcv_port_index("RPRI");  //get the remote receive interface index out of the remote h_port
	while (remote_bridge<0){};  //receiver interface not defined
	int rem_id = make_nodeportid(remote_hport_index, remote_receive_port_index); //make a proper target address

	fetch_add(sec_thread, 1);  //signal that secondary thread has defined its local port

	user_msg_data rcv_buf;
	print_clock timer;
	user_msg_data message;
	while (num_user_initialized < NUM_SOCS){};

	unsigned remote_id;
	int fence = 0;
	while (true){
		unsigned rcv_length;
		bool rcv_done = false;
		while (!rcv_done){  //wait until read is ready
			rcv_done = rcvport_ptr->rcv_imm_message(rcv_buf.buffer, &rcv_length, &remote_id);
		}
		unsigned seqno = rcv_buf.seqno;
		int snd_length = 4;
		message.seqno = seqno + 1;
		int snd_done = 0;
		while (snd_done <= 0){
			snd_done = cmdport_ptr->send_imm_message(message.buffer, snd_length, rem_id, message.seqno, fence); //wait until a message can be sent
		}

		zcompletion_record result;
		int comp_done = 0;
		while (comp_done <= 0){
			comp_done = cmdport_ptr->get_comp_status(&result); //remove completion
		}
	}
}

// currently not extended for multiple clients per SOC
void test_genz_sys_ping_pong(){

	thread_arg temp_arg;
	user_arg_vect.resize(NUM_SOCS, temp_arg);  //separate input argument for each thread
	start_hdwr_svcs(); // Start hardware service
	vector<std::thread> threads; //container for all SOC threads

	// Start test using one user receive and transmit thread per SOC
	for (unsigned socid = 0; socid < NUM_SOCS; socid++){
		user_arg_vect[socid].thread_id = socid;
		user_arg_vect[socid].pdsc = &hsvc_pdsc[socid];
		threads.push_back(std::thread(user_pri_ping_pong_thread, &user_arg_vect[socid]));
		threads.push_back(std::thread(user_sec_ping_pong_thread, &user_arg_vect[socid]));
	}

	// Wait for the user threads to finish (they probably never will)
	int inx = 0;
	for (unsigned socid = 0; socid < NUM_SOCS; socid++){ //join on user threads
		threads[inx++].join();
	}
}


struct put_pong_message{
	volatile int seqno;
	volatile int ready;
};


void user_pri_putpong_thread(void * aArg) {
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
	int rem_thread = (threadno + 1) % NUM_SOCS;
	string this_hport_name = "P" + IntToStr(threadno);
	string rem_hport_name = "P" + IntToStr(rem_thread);
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name);

	int error_reply;
	// define local ports and memory regions
	cmdport* cmdport_ptr = port_ptr->request_add_cmd_port("CPRI", &error_reply /*, arg->pdsc*/); //add a new logical port to this physical port
	while (cmdport_ptr == 0){};
	rcvport* rcvport_ptr = port_ptr->request_add_rcv_port("RPRI", &error_reply /*, arg->pdsc*/); //add a new logical port to this physical port
	while (cmdport_ptr == 0){};  //was the port successfully added?

	put_pong_message rcv_buff;
	put_pong_message send_buff;

	int port_id = rcvport_ptr->register_mem_region("pri_window", &rcv_buff, sizeof(put_pong_message)); //register a local memory region
	while (port_id < 0) {};//was the port successfully added?
	fetch_add(pri_thread, 1); //signal that primary thread has defined its local ports
	while (sec_thread < NUM_SOCS){}  //must wait until secondary ports are defined

	int remote_hport_index = gdata.ccmgr.get_port_inx("main_clique", rem_hport_name);
	hport* remote_bridge_ptr = gdata.ccmgr.get_port_ptr("main_clique", rem_hport_name);
	int remote_receive_port_index = remote_bridge_ptr ->get_rcv_port_index("RSEC");  //get the remote receive interface index out of the remote h_port
	rcvport* remote_receive_pointer = remote_bridge_ptr->get_rcv_port_pointer("RSEC");
	int rem_region_index = remote_receive_pointer->get_mem_region_index("sec_window");
	while (rem_region_index<0)
		{};  //receiver interface not defined
	unsigned rem_id = make_nodeportid(remote_hport_index, remote_receive_port_index); //make a proper target address

	fetch_add(num_user_initialized, 1);

	unsigned app_seqno = 0; //application defines a sequence number to check results
	int num_msgs = 100000000;
	int sample_size = 10000;
	unsigned prior_seqno = 0;
	int cum_errors = 0;
	while (sec_thread < NUM_SOCS){};  //wait until secondary's remote ports are defined
	fetch_add(num_user_initialized, 1); 
	rcv_buff.ready = 0;  
	rcv_buff.seqno = 0;  //minitialize receive buffer to eliminate compiler warnings (ref before use)
	int fence = 0; //fence not used for pingpong

	print_clock timer;
	for (int i = 0; i < num_msgs; i++){ //loop to send the actual messages
		for (int i = 0; i < sample_size; i++){
			rcv_buff.ready = 0;  //mark receive as not-ready before sending
			send_buff.seqno = app_seqno;
			send_buff.ready = 1;   //mark the sent message as ready
			int send_done = 0;
			int local_sequence_number = 0;  //not needed here with only one parallel message (and completion) at a time
			while (send_done <= 0){
				send_done = cmdport_ptr->put(rem_id, &send_buff, rem_region_index, 0, sizeof(put_pong_message), local_sequence_number, fence); //wait until a message can be sent
			}

			zcompletion_record result;
			int comp_done = 0;
			while (comp_done <= 0){
				comp_done = cmdport_ptr->get_comp_status(&result); //remove completion
			}
			while (rcv_buff.ready == 0) {} //wait for the put
			app_seqno = rcv_buff.seqno;
			if (app_seqno != (prior_seqno + 1)) cum_errors++;
			prior_seqno = app_seqno;
		}

		if (LOG_TEST_RESULTS == 1) {
			gdata.print_lock.lock();
			double durr_seconds = timer.print_diff("time_difference: ", *logfile);
			*logfile << " latency=" << (durr_seconds / (2 * sample_size)) << " cum_errors=" << cum_errors << " seq=" << app_seqno << endl;
			gdata.print_lock.unlock();
		}
	}
}


void user_sec_putpong_thread(void * aArg){
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
	int rem_thread = (threadno + 1) % NUM_SOCS;

	string this_hport_name = "P" + IntToStr(threadno);
	string rem_hport_name = "P" + IntToStr(rem_thread);
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name);

	int error_reply;
	// define local ports and memory regions
	cmdport* cmdport_ptr = port_ptr->request_add_cmd_port("CSEC", &error_reply /*, arg->pdsc*/); //add a new logical port to this physical port
	while (cmdport_ptr == 0){};
	rcvport* rcvport_ptr = port_ptr->request_add_rcv_port("RSEC", &error_reply /*, arg->pdsc*/ ); //add a new logical port to this physical port
	while (cmdport_ptr == 0){};

	put_pong_message rcv_buff;
	put_pong_message send_buff;

	int port_id = rcvport_ptr->register_mem_region("sec_window", &rcv_buff, sizeof(put_pong_message)); //register a local memory region
	while (port_id < 0)
		{ };
	rcv_buff.ready = 0;
	while (pri_thread < NUM_SOCS){}

	fetch_add(sec_thread, 1); //signal that secondary thread has defined its local ports

	int remote_hport_index = gdata.ccmgr.get_port_inx("main_clique", rem_hport_name);
	hport* remote_bridge_ptr = gdata.ccmgr.get_port_ptr("main_clique", rem_hport_name);
	int remote_receive_port_index = remote_bridge_ptr->get_rcv_port_index("RPRI");  //get the remote receive interface index out of the remote h_port
	rcvport* remote_receive_pointer = remote_bridge_ptr->get_rcv_port_pointer("RPRI");
	int rem_region_index = remote_receive_pointer->get_mem_region_index("pri_window");
	while (rem_region_index < 0) {};  //receiver interface not defined
	unsigned rem_id = make_nodeportid(remote_hport_index, remote_receive_port_index); //make a proper target address
	//initialize receive buffer 
	int fence = 0;
	while (true){
		while (rcv_buff.ready == 0){}
		int app_seqno = rcv_buff.seqno;
		rcv_buff.ready = 0;

		send_buff.seqno = app_seqno + 1;
		send_buff.ready = 1;

		int local_sequence_number = 0;  //not used here for only one parallel message (and one completion) at a time
		int send_done = 0;
		while (send_done <= 0){
			send_done = cmdport_ptr->put(rem_id, &send_buff, rem_region_index, 0, sizeof(put_pong_message), local_sequence_number, fence); //wait until a message can be sent
		}

		zcompletion_record result;
		int comp_done = 0;
		while (comp_done <= 0){
			comp_done = cmdport_ptr->get_comp_status(&result); //remove completion
		}
	}
}

void test_genz_sys_put_pong(){
	thread_arg temp_arg;
	user_arg_vect.resize(NUM_SOCS, temp_arg);  //provide distinct input argument for each thread
	start_hdwr_svcs(); // Start hardware service
	vector<std::thread> threads; //container for all SOC threads

	// Start test using one user receive and transmit thread per SOC
	for (unsigned socid = 0; socid < NUM_SOCS; socid++){
		user_arg_vect[socid].thread_id = socid;
		user_arg_vect[socid].pdsc = &hsvc_pdsc[socid];

		threads.push_back(std::thread(user_pri_putpong_thread, &user_arg_vect[socid]));
		threads.push_back(std::thread(user_sec_putpong_thread, &user_arg_vect[socid]));
	}

	// Wait for the user threads to finish (they probably never will)
	int inx = 0;
	for (unsigned socid = 0; socid < NUM_SOCS; socid++){ //join on user threads
		threads[inx++].join();
	}
}


void user_rcv_ind_thread(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;

	int threadno = arg->thread_id;
	int rem_thread = (threadno + NUM_SENDERS - 1) % NUM_SENDERS;  //set up circular communication test
	string this_hport_name = "P" + IntToStr(threadno); //names the physical port
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name);  // in this test, snd and rcv threads share a common port

	indirect_intf  intf("this_rcv_intf", port_ptr);  //construct an indirect interface
	list<int> credit_list = intf.get_avail_local_credits(10);
	remote_credit_table[threadno] = credit_list;  //place the credit list in global credit table for sender access
	remote_receiver_table[threadno] = &intf;  //place a pointer to the interface in the global table
	fetch_add(num_urcv_init, 1);	//signal that ports on receivers are initialized

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_thread no: " + IntToStr(threadno) + " added port name: " + "this_rcv_intf");
	}


	while (num_usnd_init < NUM_SENDERS){} //wait until senders have initialized their local ports
	indirect_intf* remote_sender_address = remote_sender_table[rem_thread];
	int conn_id = intf.add_connection(remote_sender_address->get_dual_address(), remote_sender_address->get_put_window_id(), remote_sender_address->get_put_window_frame_size());
	while (conn_id < 0){};  //this better be successful
	intf.add_remote_credits(conn_id, remote_credit_table[rem_thread]);
	std::cout << "time " << DoubleToStr(global_time.get_time()) << "added rcv-side connection" << endl;
	fetch_add(num_urcv_init, 1);	//signal that local ports on receivers are initialized

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_ind_thread no: " + IntToStr(threadno) + " added port name: " + "RCV");
	}

	while (num_usnd_init < NUM_SENDERS){} //wait until senders have initialized their local ports
	fetch_add(num_user_initialized, 1);  //signal that remote ports on receivers are initialized

	int prior_seqno = -1;
	int cum_errors = 0;
	user_msg_data message;
	int sample_size = 1000;
	print_clock timer;

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_ind_thread no:" + IntToStr(threadno) + " starting rcvr test loop");
	}

	//this is the receiver's test loop
	while (true){
		for (int i = 0; i < sample_size; i++){
			unsigned length;
			unsigned source;
			int done = 0;
			while (!done){
				done = intf.rcv_ind_message(message.buffer, &length, &source);
			}

			if (LOG_DEBUG == 1) {
				locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_rcv_ind_thread: got packet no:" + IntToStr(message.seqno));
			}

			int seqno = message.seqno;
			if (seqno != (prior_seqno + 1)) cum_errors++;
			prior_seqno = seqno;

			// Insert data rcv delay
			if (enable_delay == 1) {
				global_time.sleep((double)delay_rcv_client);
			}
		}
		if (LOG_TEST_RESULTS == 1) {
			gdata.print_lock.lock();
			double durr_seconds = timer.print_diff("time_difference: ", *logfile);
			*logfile << "T:" << threadno << "  " << (sample_size / durr_seconds) << " msgs/sec" << " cum_errors=" << cum_errors << endl;
			gdata.print_lock.unlock();
		}
	}
}

void user_snd_ind_thread(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
	int rem_thread = (threadno + 1) % NUM_SENDERS;

	string this_hport_name = "P" + IntToStr(threadno); //names the physical port
	string rem_hport_name = "P" + IntToStr(rem_thread);
	hport* port_ptr = gdata.ccmgr.get_port_ptr("main_clique", this_hport_name); //get a handle to this physical port
	
	indirect_intf  intf("this_snd_intf", port_ptr);  //construct an indirect interface

	remote_sender_table[threadno] = &intf;

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_snd_ind_thread no: " + IntToStr(threadno) + " added port name: " + "this_snd_intf");
	}

	fetch_add(num_usnd_init, 1);  //indicate that user send port initialization is complete
	while (num_urcv_init < NUM_SENDERS){}  //wait until receivers define all remote user ports

	indirect_intf* remote_receiver_address = remote_receiver_table[rem_thread];
	int conn_id = intf.add_connection(remote_receiver_address->get_dual_address(), remote_receiver_address->get_put_window_id(), remote_receiver_address->get_put_window_frame_size());
	while (conn_id < 0){};  //this better be successful
	intf.add_remote_credits(conn_id, remote_credit_table[rem_thread]);

	std::cout << "time " << DoubleToStr(global_time.get_time()) << "added send-side connection" << endl;
	int seqno = 0;  // 
	user_msg_data message;
	int num_msgs = 100;
	while (num_user_initialized < NUM_SENDERS){}

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_snd_ind_thread no:" + IntToStr(threadno) + " starting sndr test loop");
	}

	int num_sent = 0;
	int num_completed = 0;
	int fence = 1;
	//this is the sender's test loop
	for (int i = 0; i < num_msgs; i++){ //loop to send the actual messages
		int length = 4;
		message.seqno = seqno;
		int done = 0;
		while (done <= 0){
			done = intf.send_ind_message(message.buffer, length, conn_id);
		}
		num_sent++;

		if (LOG_DEBUG == 1) {
			locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " usr_snd_ind_thread: sent packet seq_num: " + IntToStr(seqno));
		}
		seqno++;
		// Insert command insertion delay
		if (enable_delay == 1) {
			global_time.sleep((double)delay_cmnd_insert);
		}
	}
}

void test_genz_sys_indirect(){	//main thread launches all services
	thread_arg temp_arg;
	user_arg_vect.resize(NUM_SENDERS, temp_arg);  //separate input argument for each thread
	start_hdwr_svcs(); // Start hardware service
	vector<std::thread> threads; //container for all SOC threads
	unsigned thread_id = 0;

	for (unsigned socid = 0; socid < NUM_SOCS; socid++){
		thread_id = socid;
		user_arg_vect[thread_id].thread_id = thread_id;
		user_arg_vect[thread_id].pdsc = &hsvc_pdsc[socid];
		threads.push_back(std::thread(user_rcv_ind_thread, &user_arg_vect[thread_id]));
		threads.push_back(std::thread(user_snd_ind_thread, &user_arg_vect[thread_id]));
	}

	if (LOG_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " test_genz_sys created clients: " + IntToStr(NUM_CLIENTS));
	}

	// Wait for the user threads to finish (add termination condition)
	int inx = 0;
	for (unsigned socid = 0; socid < NUM_SENDERS; socid++){ //join on user threads
		threads[inx++].join();
	}

}