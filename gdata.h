#ifndef _GDATA_H
#define _GDATA_H

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



#include <mutex>
#include <map>
#include <thread>
#include "network.h"
#include "hdwr_loops.h"
#include "zengine.h"
#include "indirect_intf.h"


using namespace std;


//Debug - LOG flags 
#define LOG_TEST_RESULTS 1
#define LOG_SCHEDULER 0
#define LOG_COMP_ENG 0
#define LOG_ZENG 0
#define LOG_RCV_THREAD 0
#define LOG_HRDWR_DEBUG 0
#define LOG_CUSTOM_MASK 0
#define LOG_CUSTOM_MASK_VAL "test"
#define LOG_DEBUG 1

// Hardware params
#define NUM_SOCS 1						//number of system on chip devices that run independent OSs
#define NUM_CLIENTS 1					// number of clients per SOC
#define NUM_SENDERS NUM_SOCS*NUM_CLIENTS
#define MAX_NUM_HPORTS 20
#define MAX_NUM_ZENG 1					// per node number of zengines
#define NUM_ZENG MAX_NUM_ZENG*NUM_SOCS	//total zengs

// Delay parameters - can turn On and off 
// delay is in usec??
#define enable_delay 0	// enable/diable timing model
#define delay_cmnd_insert 0.0000   // client delay to insert in cmnd queue
#define delay_rcv_client 0.0000 // client delay at rcv queue
#define delay_cmnd_retrieve 0.0000 // scheduler delay to retreive cmnd from cmnd queue
#define delay_zeng_retrieve 0.0000 // zeng delay to retreive cmnd from cmnd queue
#define delay_channel 0.00000		// channel delay -- fixed value for now, maybe a function later
#define delay_comp 0.0000		// delay completion insertion/retrieval
#define latency delay_channel+delay_comp
#define delay_rcvr 0.0000
#define delay_schd 0.0000

struct thread_arg{  //argument to thread initiation
	int thread_id;
	struct hport_descr* pdsc;
};

class global_data{  //this data is shared by all SOCs
public:
	mutex print_lock;
	mutex mem_lock; //lock provides mutual exclusion for memlist buffer manager access
	net_clique_mgr ccmgr;
	merge_queue mq_vect[MAX_NUM_HPORTS]; //one merge queue per SOC
	genz_engine zeng_vect[NUM_ZENG]; // zeng_vect in system. move it under hport

public:
	global_data();

};

// user threads
extern struct hport_descr hsvc_pdsc[NUM_SOCS];
extern vector<thread_arg> user_arg_vect;


//indirect experiment globals
extern list<int> remote_credit_table[NUM_SOCS];  //used to carry remote credits from each receiving interface back to sender
extern indirect_intf* remote_receiver_table[NUM_SOCS];
extern indirect_intf* remote_sender_table[NUM_SOCS];


//global files for data output
extern ofstream* logfile;
extern global_data gdata;

//global simulation start time 
extern double sim_start_time;

// hrdware threads
extern vector<std::thread> hdwr_threads; //hardware threads list, per node
extern vector<thread_arg> hdwr_arg_vect; //contains arguments for each created threads

// zengine threads
extern vector<std::thread> zeng_threads; 
extern vector<thread_arg> zeng_arg_vect; 
#endif