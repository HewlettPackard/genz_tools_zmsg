#ifndef _SPORT_H
#define _SPORT_H

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

#include "net_port_id.h"
#include "sport_intf.h"
#include <mutex>
#include <map>
#include "network.h"

class cmdport{  //defines ports for issuing commands
private:
	mutex port_lock;
	string cmdport_name; 
	int sid; //internal id for this logical command port
	hport *hport_ptr;  //pointer to hport to which this cmdport is attached
	friend class hport;
	net_port_id port_id;;
	cmd_port_interface intf;

public:    //helping routines -- add more for each command type
	cmdport(hport* hport, string name, int inx, int rcv_buff_size);
	int send_imm_message(char* data, int len, unsigned destination, unsigned seqno, int fence);
	int get_comp_status(zcompletion_record* result);
	net_port_id get_port_id();
	int  put(unsigned destination, void* lcl_src_ptr, unsigned remote_region_index, unsigned remote_region_offset, int length, int seqno, int fence);
	int  get(unsigned destination, void* lcl_dst_ptr, unsigned remote_region_index, unsigned remote_region_offset, int length, int seqno, int fence);

};

class rcvport{  //defines receiver ports
private:
	mutex port_lock;
	string rcvport_name;
	int sid; //internal id for this logical receive port
	hport *hport_ptr;  //pointer to hport to which this rcvport is attached
	net_port_id port_id;
	map<string, int> mem_region_index_map;  //translates a long region name to a region index
	rcv_port_interface intf;


public:

	//helping routines -- add more for each command type
	rcvport(hport* hport, string name, int inx, int rcv_buff_size);
	int  register_mem_region(string window_name, void* base_pointer, unsigned length); //register a local memory region to this receive port
	bool rcv_imm_message(char* data, unsigned* len, unsigned* source);
	int get_mem_region_index(string name); //returns the short name index for a named memory region
	net_port_id get_port_id();
	rcv_port_interface* get_intf();
	int  rcv_port_interface_register_mem_region_respzmmu(rcv_port_interface* intf, void* base_pointer, unsigned length); //register a memory region at responder


	friend class hport;
};


#endif
