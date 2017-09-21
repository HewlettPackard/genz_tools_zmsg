#ifndef _HPORT_INTF_H
#define _HPORT_INTF_H

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
#include "zmmu.h"

#define MAX_CMD_PORTS 20  //maximum number of virtual command ports at each hport
#define MAX_RCV_PORTS 20 //maximum number of virtual receive ports at each hport
#define NUM_CMDS 10
#define HPORT_CMD_ADD_CMD_PORT 1
#define HPORT_CMD_ADD_RCV_PORT 2

struct hport_cmd_record{ //this structure is local to a single SOC memory
	int type;
	unsigned process_handle;  //placeholder for future use
	cmd_port_interface* cmd_interface_ptr;
	rcv_port_interface* rcv_interface_ptr;
	int conn_index;
	int num_cmd_ports;  // upper limit for sport polling
	int num_rcv_ports;  // upper limit for sport polling
	unsigned source; 
	unsigned dest;
	int remote_conn_number;
	volatile int done;	//result of service call
};

struct hport_mgt_cmd_record{  //new command 
	int cmd;
	int errorno; //identifies the error
};

struct hport_descr{  //structure local to a single Gen-Z bridge

public:

	int hport_id;  //this should track with Gen-Z component ID
	int num_cmd_ports;  //num of ports to poll by the scheduler loop
	cmd_port_interface* cmd_port_interface_vect[MAX_CMD_PORTS];
	bool sport_in_use_vect[MAX_CMD_PORTS]; //is this port used? 

	zmmu requesting_zmmu;  //performs outbound adddress processing
	zmmu responding_zmmu;  //performs inbound address processing

	rcv_port_interface* rcv_port_interface_vect[MAX_RCV_PORTS];
	bool rcv_port_in_use_vect[MAX_RCV_PORTS]; //is this port used? 


	// TODO: add zeng initiation here and change code accordingly

	lfqueue_s avail_hport_cmd_records_s; //static queues maintain indices into command vector
	lfqueue_s hport_cmd_to_process_cmd_s;
	lfqueue_s hport_cmd_to_process_rcv_s;
	hport_cmd_record hport_cmd_vect[NUM_CMDS];

};

void init_hport_descr(hport_descr* p, unsigned hport_id);

#endif
