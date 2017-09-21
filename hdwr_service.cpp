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
#include <thread>
#include "gdata.h"
#include "sim_time.h"

// variables to count num of services started
static volatile unsigned num_rcv_init;
static volatile unsigned num_sch_init;
static volatile unsigned num_zeng_init;
static volatile unsigned num_cmp_init;

void hdwr_comp_svc(void * aArg) {

	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;

	string port_name = "P" + IntToStr(threadno);
	num_cmp_init++;

	if (LOG_COMP_ENG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " Starting Completion Engine thread no:" + IntToStr(threadno) + " port_name:" + port_name);
	}
	poll_cmp(arg->pdsc);  //initiate completion service - replace with cond vars??
}

void hdwr_zeng_svc(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;

	num_zeng_init++;

	if (LOG_ZENG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " Starting ZENG thread no:" + IntToStr(threadno) + " zeng_num:" + IntToStr(num_zeng_init));
	}
	poll_zeng(arg->pdsc, threadno);  //initiate zeng service
}

void hdwr_rcv_svc(void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;
//    lfqueuep<mem_region>* buffer_queue = arg->buffer_queue_ptr;

	//create physical port for this service
	string port_name = "P" + IntToStr(threadno);
	hport* hport_ptr = gdata.ccmgr.add_port("main_clique", port_name, /*buffer_queue,*/ logfile);
	while (hport_ptr == 0) {}; //if can't add port, stop here -- add error handling here

	num_rcv_init++;

	if (LOG_RCV_THREAD == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " Starting Receive thread no:" + IntToStr(threadno) + " port_num:" + IntToStr(num_rcv_init));
	}
	
	poll_rcv(arg->pdsc);  //initiate receive service
}

void hdwr_schd_svc( void * aArg)
{
	thread_arg * arg = (thread_arg*)aArg;
	int threadno = arg->thread_id;

	while (num_rcv_init < NUM_SOCS){}; //wait until receivers initialize their SOC's hport
	num_sch_init++;  //this scheduler is initialized

	if (LOG_SCHEDULER == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " Starting Scheduler thread no:" + IntToStr(threadno) + " port_num:" + IntToStr(num_sch_init));
	}

	poll_schd(arg->pdsc);  //initiate send service
}

void start_hdwr_svcs(){

	thread_arg temp_arg;
	thread_arg temp_arg2;
	num_rcv_init = 0;
	num_sch_init = 0;
	int num_ind_regions = 100;  //cr
	int ind_mtu = 1500;//mtu for indirect buffers
	int slot_size_index = 0;  //index selects number of slots in kernel ring buffers
	int tot_num_engs = NUM_SOCS * MAX_NUM_ZENG; // total number of zengs in the network

	//define a new "main_clique" communication service in global data
	gdata.ccmgr.create_clique("main_clique");

	pair<int, int> region_size(ind_mtu, num_ind_regions);
	list< pair<int, int> > mem_arg;
	mem_arg.push_back(region_size);
//	gdata.shared_memory = memlist(mem_arg); //create indirect buffers in the shared memory region

	hdwr_arg_vect.resize(NUM_SOCS, temp_arg);  //separate input argument for each thread
	zeng_arg_vect.resize(tot_num_engs, temp_arg2);  //separate input argument for each thread

	// this looks like a problem area... threads are being overwritten??
	//int inx = 0; // for zeng threads
	//int indx = 0; // for hdwr threads
	for (unsigned socid = 0; socid < NUM_SOCS; socid++){
		init_merge_queue(&gdata.mq_vect[socid], Z_RECORD_MTU, socid);


		hdwr_arg_vect[socid].thread_id = socid;
		hdwr_arg_vect[socid].pdsc = &hsvc_pdsc[socid];

		init_hport_descr(&hsvc_pdsc[socid], socid);
		
		// start the thread for each zengine
		for (unsigned engid = 0; engid < MAX_NUM_ZENG; engid++) {
			int zid = engid + socid * MAX_NUM_ZENG;  // each engine has a unique id in the global table
			init_genz_engine(&gdata.zeng_vect[zid], Z_RECORD_MTU, zid); //initialize the engine

			zeng_arg_vect[zid].thread_id = zid;
			zeng_arg_vect[zid].pdsc = &hsvc_pdsc[socid];

			zeng_threads.push_back(std::thread(hdwr_zeng_svc, &zeng_arg_vect[zid]));
		}

		hdwr_threads.push_back(std::thread(hdwr_rcv_svc, &hdwr_arg_vect[socid]));
		hdwr_threads.push_back(std::thread(hdwr_comp_svc, &hdwr_arg_vect[socid]));
		hdwr_threads.push_back(std::thread(hdwr_schd_svc, &hdwr_arg_vect[socid]));
	}

	while ((int)num_zeng_init < tot_num_engs) {}; //wait until all zengines are started
	while (num_cmp_init < NUM_SOCS) {}; //wait until all completion loops are initialized
	while (num_sch_init < NUM_SOCS){}; //wait until all scheduler loops are initialized
	
	if (LOG_HRDWR_DEBUG == 1) {
		locking_print(*logfile, "time: " + DoubleToStr(global_time.get_time()) + " Finished setting up the hardware ports on SOCS:" + IntToStr(tot_num_engs));
	}
}