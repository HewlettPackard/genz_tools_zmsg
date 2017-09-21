#ifndef _NETWORK_H
#define _NETWORK_H


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
#include "lfqueue.h"
//#include "mem.h"
#include "sport_intf.h"
#include "hport_intf.h"

using namespace std;

class net_clique;
class cmdport;
class rcvport;

//Gen-Z bridge hardware class
class hport{ 
	string hport_name; 
	int hport_id; //Consider this also as the Gen-Z component ID
	net_clique* parent_clique;
	mutex port_lock;  //used by snd pool loop to lock sport_vect &  sport_name_to_index_map
	hport_descr* descriptor_pointer;

	int num_cmd_ports;
	map<string, int> cmdport_name_to_index_map;

	int num_rcv_ports;
	map<string, int> rcv_port_name_to_index_map;

	map<string, rcvport*> map_rcv_name_to_pointer;
	map<int, rcvport*> map_rcv_index_to_pointer;

	mutex hport_cmd_lock_xmit;  //used to lock the tail of the ptr cmd queue
	mutex avail_hport_cmd_lock;  //used to lock the head of the avail sport queue

public:
	hport(int index, string name, net_clique* parent);
	string get_name();
	int get_cmd_port_index(string name);  //retrieve the cmd  port index
	int get_rcv_port_index(string name);  //retrieve the receive port index
	rcvport* get_rcv_port_pointer(string name);  //retrieve the receive port pointer
	rcvport* get_rcv_port_pointer(int short_id);  //retrieve the receive port pointer

	net_clique* get_parent_clique_ptr();
	bool sport_is_defined(string);
	cmdport* request_add_cmd_port( string sport_name, int* errorno /*, struct hport_descr* p*/);//asynchronous request
	rcvport* request_add_rcv_port(string rcvport_name, int* errorno /*, struct hport_descr* p*/);//asynchronous request
	hport_descr* get_descriptor_pointer();

	friend class cmdport;
	friend class rcvport;
};

// clique is a global structure shared among all nodes
class net_clique{  
	mutex net_clique_lock;  //lock for mutually exclusive access to network clique
	vector<hport*> hports; //for each hport: zengines are coupled to all merge queues, rcv is coupled to one merge queue
	unsigned slot_size_index, mtu; //constants shared by all merge queues
	unsigned total_slots, actual_slots; //constants shared by all merge queues - for rcvr
	map<string, int> hport_name_to_inx_map; //map physical port names to port indices (index is carried through core network)
	map<int, string> hport_inx_to_name_map;
	map< int, map<string,int> > receiver_port_map; //for each receiving port, map receiver names to rcv_port indices

public:
	net_clique();
	hport*  add_port(string s /*, lfqueuep<mem_region>* buffer_queue */, ofstream* out); 
	hport* get_port_ptr(string port_name);
	hport* get_port_ptr(int port_index);

	string get_port_name(int inx);  
	int get_port_inx(string name);
	bool hport_is_defined(string name);

	friend class rcvport;
	friend class net_clique_mgr;
};

class net_clique_mgr{
	map<string, net_clique*>  global_network_dir;  // list of the net_cliques
public:
	bool create_clique(string name);
	hport*   add_port(string clique_name, string port_name /*, lfqueuep<mem_region>* buffer_queue */, ofstream* out);
	hport*   get_port_ptr(string clique_name, string port_name);
	hport*   get_port_ptr(string clique_name, int port_index);
	int get_port_inx(string clique_name, string port_name);
	bool hport_is_defined(string clique_name, string port_name);
};

#else
#endif