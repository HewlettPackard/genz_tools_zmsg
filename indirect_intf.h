#ifndef _INDIRECT_INTF
#define _INDIRECT_INTF

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

#include "sport.h"
#include "string.h"
#include <list>
#include "net_port_id.h"
#include "zrecord.h"

struct dual_address_tuple{  // address tuple for indirect interface
	net_port_id dest_address;  //receiver's address at remote interface
	net_port_id src_address;  //command interface at remote interface
};

#define MAX_IND_CONNECTIONS 5
class indirect_intf
	
{
private:
	mutex lock;
	string name;
	hport* this_bridge;
	string cmd_port_name;
	string rcv_port_name;

	cmdport* cmd_port;   //port to issue send/put/get commands
	rcvport* receive_port;  //port to receive data & register remote memory buffers

	net_port_id cmd_port_id; //sending address is the return address
	net_port_id receive_port_id;  //receive address is the target address

	ring_buffer indirect_frames;  //pool of indirect data frames held by this interface
	list<int> avail_local_credits;
	lfqueue<int> rcv_buffer_list;  //index list of delivered messages
	int num_connections = 0;

	int local_put_window_id;
	int put_window_frame_size;

	//connection state
	dual_address_tuple connection_table[MAX_IND_CONNECTIONS];
	bool connection_is_valid[MAX_IND_CONNECTIONS];
	list<int>  remote_credits[MAX_IND_CONNECTIONS];  //available send credit (buffer index) lists  for remote interfaces
	int remote_window_id[MAX_IND_CONNECTIONS];  //identify the remote window id for each connection
	int remote_window_frame_size[MAX_IND_CONNECTIONS];
	
	map<net_port_id, int> xlate_remote_send_address_to_conn_id;  
	map<net_port_id, int> xlate_remote_rcv_address_to_conn_id;
	unsigned seqno;
	void process_incoming_messages();

public:
	indirect_intf(string intf_name,  hport* bridge); //construct a new interface
	//void request_remote_credits(int conn_id, int num_credits);  //send a message asking for more credits
	int send_ind_message(char* data, int len, int conn_id);
	int send_seq_ind_messages(list<char*> data, list<int> len, int conn_id);

	int rcv_ind_message(char* data, unsigned* len, unsigned* source);
	int get_rcv_port_id();
	list<int> get_avail_local_credits(int num_frames);
	net_port_id get_return_address();
	net_port_id get_target_address();
	void add_remote_credits(int conn_id, list<int> credits);
	dual_address_tuple get_dual_address();
	int get_put_window_id();
	int get_put_window_frame_size();
	int  add_connection(dual_address_tuple, int remote_window_id, int frame_size);
	int send_credits_to_remote(int conn_id, list<int> indexes);  //add credits for a remote interface

};

#endif