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

#include "network.h"
#include "sport.h"
#include "gdata.h"

hport::hport(int inx, string name, net_clique* parent_in)
{
	hport_id = inx;
	hport_name = name;
	parent_clique = parent_in;
	num_cmd_ports = 0;
	num_rcv_ports = 0;
	descriptor_pointer = &hsvc_pdsc[hport_id]; 
}

string hport::get_name(){
	return hport_name;
}

int hport::get_cmd_port_index(string name){  //get the command port index
	if (cmdport_name_to_index_map.find(name) == cmdport_name_to_index_map.end()){
		return -1;  //name not defined
	}
	int result = cmdport_name_to_index_map[name];
	return result;
}

int hport::get_rcv_port_index(string name){  //get the sport index
	if (rcv_port_name_to_index_map.find(name) == rcv_port_name_to_index_map.end()){
		return -1;  //name not defined
	}
	int result = rcv_port_name_to_index_map[name];
	return result;
}

rcvport* hport::get_rcv_port_pointer(string name){  //retrieve the receive port pointer
	return map_rcv_name_to_pointer[name];
}

rcvport* hport::get_rcv_port_pointer(int short_id){  //retrieve the receive port pointer
	return map_rcv_index_to_pointer[short_id];
}

net_clique* hport::get_parent_clique_ptr(){
	return parent_clique;
}

bool  hport::sport_is_defined(string sport_name){
	if (cmdport_name_to_index_map.find(sport_name) == cmdport_name_to_index_map.end()) return false;
	return true;
}

cmdport* hport::request_add_cmd_port( string sport_name, int* result_errorno /*, hport_descr* p*/){
	
	port_lock.lock();
	if (cmdport_name_to_index_map.find(sport_name) != cmdport_name_to_index_map.end()) {
		port_lock.unlock();
		return  0; //fail if name is already in map
	}
	while ((num_cmd_ports + 1) >= MAX_CMD_PORTS){} //adding sport to full vector

	int sport_short_id = num_cmd_ports++;//get internal index for new port
	cmdport * cmdportptr = (new cmdport(this, sport_name, sport_short_id, NUM_RCV_BUFFER_ENTRIES));
	cmdport_name_to_index_map[sport_name] = sport_short_id;
	port_lock.unlock();

	cmdport * result = cmdportptr;
	cmd_port_interface* interface_ptr = &cmdportptr->intf;
	cmd_port_interface_init(interface_ptr, NUM_RCV_BUFFER_ENTRIES, make_nodeportid(hport_id, sport_short_id));

	hport_cmd_record* cmd_ptr;

	unsigned cmd_index;
	while (true){ //wait for an available add port record (only NUM_SPORT_ADD_SLOTS are added in parallel)
		avail_hport_cmd_lock.lock();
		int success = pop_head_lfqueue_s(&descriptor_pointer->avail_hport_cmd_records_s, &cmd_index);
		avail_hport_cmd_lock.unlock();
		if (success == 1) break;
	}

	cmd_ptr = &descriptor_pointer->hport_cmd_vect[cmd_index];
	cmd_ptr->type = HPORT_CMD_ADD_CMD_PORT;
	cmd_ptr->cmd_interface_ptr = &cmdportptr->intf;
	cmd_ptr->source = make_nodeportid(hport_id, sport_short_id);
	cmd_ptr->num_cmd_ports = num_cmd_ports;
	cmd_ptr->conn_index = sport_short_id;  //pass the sport short id to xmit & rcv poll 
	cmd_ptr->done = 0;  //mark record as not yet processed;

	while (true){
		hport_cmd_lock_xmit.lock();

		//submit the request for processing
		int success = push_tail_lfqueue_s(&descriptor_pointer->hport_cmd_to_process_cmd_s, cmd_index); // the xmit server will process, pass to rcv server, then mark this record as done

		hport_cmd_lock_xmit.unlock(); //allow others to append into the shaared queue
		if (success==1)  break;  //this should always succeed on the first trial
	}
	while (cmd_ptr->done == 0){}; //wait xmit processing and for for the rcv server to mark the record as done
	while (true){  //release the record for reuse
		avail_hport_cmd_lock.lock();
		int success = push_tail_lfqueue_s(&descriptor_pointer->avail_hport_cmd_records_s, cmd_index); //insert the last element 
		avail_hport_cmd_lock.unlock();
		if (success) break;
	} 
	return result;
}

rcvport* hport::request_add_rcv_port(string rcv_port_name, int* result_errorno /*, hport_descr* p*/){

	if (rcv_port_name_to_index_map.find(rcv_port_name) != rcv_port_name_to_index_map.end()) {
		port_lock.unlock();
		return  0; //fail if receiver name is already in map
	}
	while ((num_rcv_ports + 1) >= MAX_RCV_PORTS){} //adding sport to full vector

	port_lock.lock();

	int rcv_port_short_id = num_rcv_ports++;//get internal index for new port
	rcvport * rcv_portptr = (new rcvport(this, rcv_port_name, rcv_port_short_id, NUM_RCV_BUFFER_ENTRIES));
	rcv_port_name_to_index_map[rcv_port_name] = rcv_port_short_id;
	map_rcv_name_to_pointer[rcv_port_name] = rcv_portptr;
	map_rcv_index_to_pointer[rcv_port_short_id] = rcv_portptr;
	port_lock.unlock();

	rcvport * result = rcv_portptr;
	rcv_port_interface* interface_ptr = &rcv_portptr->intf;
	rcv_port_interface_init(interface_ptr, NUM_RCV_BUFFER_ENTRIES, make_nodeportid(hport_id, rcv_port_short_id));

	hport_cmd_record* cmd_ptr;
	unsigned cmd_index;
	while (true){ //wait for an available add port record (only NUM_SPORT_ADD_SLOTS are added in parallel)
		avail_hport_cmd_lock.lock();
		int success = pop_head_lfqueue_s(&descriptor_pointer->avail_hport_cmd_records_s, &cmd_index);
		avail_hport_cmd_lock.unlock();
		if (success == 1) break;
	}

	cmd_ptr = &descriptor_pointer->hport_cmd_vect[cmd_index];
	cmd_ptr->type = HPORT_CMD_ADD_RCV_PORT;

	cmd_ptr->rcv_interface_ptr = &rcv_portptr->intf;
	cmd_ptr->source = make_nodeportid(hport_id, rcv_port_short_id);
	cmd_ptr->num_rcv_ports = num_rcv_ports;
	cmd_ptr->conn_index = rcv_port_short_id;  //pass the rcv_port short id to rcv poll 
	cmd_ptr->done = 0;  //mark record as not yet processed;
	while (true){
		hport_cmd_lock_xmit.lock();

		//submit the request for processing
		int success = push_tail_lfqueue_s(&descriptor_pointer->hport_cmd_to_process_cmd_s, cmd_index); // the xmit server will process, pass to rcv server, then mark this record as done

		hport_cmd_lock_xmit.unlock(); //allow others to append into the shaared queue
		if (success == 1)  break;  //this should always succeed on the first trial
	}

	while (cmd_ptr->done == 0){}; //wait xmit processing and for for the rcv server to mark the record as done
	while (true){  //release the record for reuse
		avail_hport_cmd_lock.lock();
		int success = push_tail_lfqueue_s(&descriptor_pointer->avail_hport_cmd_records_s, cmd_index); //insert the last element 
		avail_hport_cmd_lock.unlock();
		if (success) break;
	}
	return result;
}


hport_descr* hport::get_descriptor_pointer(){
	return descriptor_pointer;
}

net_clique::net_clique(){
	total_slots = RING_SIZE;
	actual_slots = total_slots - 1;
	mtu = Z_RECORD_MTU;
}

//this adds matching send and receive physical ports
hport*  net_clique::add_port(string name /*, lfqueuep<mem_region>* buffer_queue*/, ofstream* out){
	net_clique_lock.lock(); // for thread safety, allow only one add_port at a time
	if (hport_is_defined(name)) {
		net_clique_lock.unlock();
		return 0; //fail if name is already in set
	}

	int p_index = hports.size(); //get index of new physical port
	while (p_index >= (MAX_NUM_HPORTS + 1)){}  //exceeded static declaration

	hports.push_back(new hport(p_index, name,  this));
	hport_name_to_inx_map[name] = p_index;
	hport_inx_to_name_map[p_index] = name;
	hport* result = hports[p_index];  //return port pointers
	net_clique_lock.unlock();
	return result; //return success
}

hport* net_clique::get_port_ptr(string port_name){
	net_clique_lock.lock(); //do not allow get xmit port during port add
	if (hport_name_to_inx_map.find(port_name) == hport_name_to_inx_map.end()){
		net_clique_lock.unlock();
		return 0;
	}
	int inx = hport_name_to_inx_map[port_name];
	net_clique_lock.unlock();
	return  hports[inx];
}

hport*   net_clique::get_port_ptr(int port_index){
	return  hports[port_index];
}

string net_clique::get_port_name(int inx){
	if (hport_inx_to_name_map.find(inx) == hport_inx_to_name_map.end()) return string(""); //could not find
	return hport_inx_to_name_map[inx];
}

int net_clique::get_port_inx(string name){
	if (hport_name_to_inx_map.find(name) == hport_name_to_inx_map.end()) return -1; //could not find
	return hport_name_to_inx_map[name];
}

bool net_clique::hport_is_defined(string name){
	if (hport_name_to_inx_map.find(name) != hport_name_to_inx_map.end()) return true;
	return false;
}

bool net_clique_mgr::create_clique(string name){
	if (global_network_dir.find(name) != global_network_dir.end()) return false; //already exists
	global_network_dir[name] = new net_clique();
	return true;
}

hport*   net_clique_mgr::add_port(string clique_name, string port_name, /*lfqueuep<mem_region>* buffer_queue ,*/ ofstream* out){
	return global_network_dir[clique_name]->add_port(port_name, /*buffer_queue,*/ out);
}

hport*   net_clique_mgr::get_port_ptr(string clique_name, string port_name){
	return global_network_dir[clique_name]->get_port_ptr(port_name);
}

hport*   net_clique_mgr::get_port_ptr(string clique_name, int port_index){
	return global_network_dir[clique_name]->get_port_ptr(port_index);
}

int   net_clique_mgr::get_port_inx(string clique_name, string port_name){
	return global_network_dir[clique_name]->get_port_inx(port_name);
}

bool net_clique_mgr::hport_is_defined(string clique_name, string port_name){
	return global_network_dir[clique_name]->hport_is_defined(port_name);
}