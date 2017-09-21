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
#include "sport_intf.h"
#include "net_port_id.h"
	

cmdport::cmdport(hport* hport_in, string name, int inx, const int buff_size_in)
{
	buff_size_in;
	hport_ptr = hport_in;
	sid = inx;
	port_id = net_port_id(hport_ptr->hport_id, sid);
	cmdport_name = name;
}

int cmdport::send_imm_message(char* data, int len, unsigned destination, unsigned seqno, int fence){  
	return cmd_port_interface_send_imm_message(&intf, data, len, destination, seqno, fence);
}

int cmdport::get_comp_status(zcompletion_record* result){
	return cmd_port_interface_get_comp_status(&intf, result);
}


net_port_id cmdport::get_port_id(){
	return port_id;
}

int  cmdport::put(unsigned destination, void* lcl_src_ptr, unsigned remote_region_index, unsigned remote_region_offset, int length, int seqno, int fence){
	return cmd_port_interface_put(&intf, destination, lcl_src_ptr, remote_region_index, remote_region_offset, length, seqno, fence);
}

int  cmdport::get(unsigned destination, void* lcl_dst_ptr, unsigned remote_region_index, unsigned remote_region_offset, int length, int seqno, int fence){
	return cmd_port_interface_get(&intf, destination, lcl_dst_ptr, remote_region_index, remote_region_offset, length, seqno, fence);
}
rcvport::rcvport(hport* hport_in, string name, int inx, const int buff_size_in)
{
	buff_size_in;
	hport_ptr = hport_in;
	sid = inx;
	port_id = net_port_id(hport_ptr->hport_id, sid);
	rcvport_name = name;

}

int  rcvport::register_mem_region(string window_name, void* base_pointer, unsigned length){ //register a local memory region at a receiver
	port_lock.lock();
	rcv_port_interface* interface = &intf;
	net_clique* parent_clique = hport_ptr->get_parent_clique_ptr();
	unsigned this_port_id = port_id;
	
	map<string, int>::iterator it = mem_region_index_map.find(window_name);
	if (it != mem_region_index_map.end())
		{port_lock.unlock();  return -1;  }//name is already defined

	int  local_id = rcv_port_interface_register_mem_region(interface, base_pointer, length); //register a memory region
	if (local_id < 0) { port_lock.unlock(); return -1; } //not enough space for new region
	mem_region_index_map[window_name] = local_id;
	port_lock.unlock();
	return local_id;
}

bool rcvport::rcv_imm_message(char* data, unsigned* len, unsigned* source){
	int success = rcv_port_interface_rcv_imm_message(&intf, data, len, source);
	if (success == 0) return false;
	return true;
}

int rcvport::get_mem_region_index(string name){
	port_lock.lock();
	map<string, int>::iterator it = mem_region_index_map.find(name);
	if (it == mem_region_index_map.end()){
		port_lock.unlock();;
		return -1;  //name is not defined
	}
	return(mem_region_index_map[name]);
	port_lock.unlock();
}

net_port_id rcvport::get_port_id(){
	return port_id;
}

rcv_port_interface* rcvport::get_intf(){
	return &intf;
}
