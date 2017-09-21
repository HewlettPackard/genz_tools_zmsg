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


#include "sim_resource.h"
#include "sim_time.h"
#include "tools.h"

chan_res_map::chan_res_map(){
	next_unused = 0;
}

void chan_res_map::insert(int id, int amount){  //insert a new entry to the map
	res_map[id] = amount;
}

void chan_res_map::remove(int id){  //removes an entry form the map
	res_map.erase(id);
	unused_id_list.push_back(id);  //save the id for recycling
}

chan_res_map chan_res_map::trim(){  //returns a copy of the res_map, without entries having non-positive value field
	chan_res_map result;
	for (map<int, int>::iterator it = res_map.begin(); it != res_map.end(); it++){
		if (it->second > 0){
			result.insert(it->first, it->second); //copy entry forward
		}
	}
	return result;
}

int chan_res_map::find_small(){  //find an entry having the smallest positive value
	int result = MAXINT;
	for (map<int, int>::iterator it = res_map.begin(); it != res_map.end(); it++){
		int value = it->second;
		if (value > 0) result = MIN(result, value);
	}
	return result;
}

void chan_res_map::subtract_scalar(int bits_sent){ //move bits through channel for all active senders
	chan_res_map result;
	for (map<int, int>::iterator it = res_map.begin(); it != res_map.end(); it++){
		it->second = MAX(0, ((it->second) - bits_sent));
	}
}

int chan_res_map::get_unused_id(){
	int id;

	if (!unused_id_list.empty()){  //look to reuse a previously released id
		id = unused_id_list.front();
		unused_id_list.pop_front();
		return id;
	}

	id = next_unused; //get a brand new id
	next_unused++;
	return id;
}

double chan_res_map::get_val(int id){
	return res_map[id];
}

bool chan_res_map::done(int id){  //tests a communication reqeust for completeness
	map<int, int>::iterator it = res_map.find(id);
	if (it == res_map.end()) return TRUE; //not present entry is considered as done
	return (it->second <= 0);  //non-positive bits-to-move means this is done
}

int chan_res_map::size(){
	return res_map.size();
}

channel_resource::channel_resource(double bandwidth){
	channel_bw = bandwidth;  //initialize the bandwidth of the3 channel
	prev_time = global_time.get_time();  //prev time is time of creation
}

double channel_resource::calc_min_delay(int id){//calculated delay for client "id" assuming no new requests
	double delay = 0.;
	chan_res_map temp_map = rem_requests.trim();
	while (!temp_map.done(id)){ //is the remaining request for id retired (<= 0) ?
		int bits_to_move = temp_map.find_small(); //find smallest remaining rqeust
		int num_sending = temp_map.size();  //how many positive senders?
		double svc_rate = channel_bw / num_sending;  //service rate for each requestor
		delay += bits_to_move / svc_rate;  //advance delay time across these bits
		temp_map.subtract_scalar(bits_to_move); //scalar subtraction from all entries in map
		temp_map = temp_map.trim();  //copy zero-trimmed map
	}
	return delay;
}

int channel_resource::insert(int bits_requested){//adds a new communication request returns the id of the request
	update();  //update request map and set prev_time to now
	int id = rem_requests.get_unused_id();
	rem_requests.insert(id, bits_requested); //insert a new request starting now
	return id;
}

void channel_resource::remove(int id){ //removes a completed communication request
	rem_requests.remove(id);
}

void channel_resource::channel_delay(int bits){  //bits specifies number of bits to move across channel
	mut.lock();  //lock resource before insertion
	int id = insert(bits);  //insert a new request for a transmission of size bits
	while (TRUE){
		if (rem_requests.done(id)){//resource locked during done test
			remove(id); //locked during delete
			mut.unlock();
			return;
		}
		update();  //update to current time
		double dt = calc_min_delay(id);
		mut.unlock();
		global_time.sleep(dt);  //resource is unlocked during sleep
		mut.lock();
	}
}

void channel_resource::update(){ //update send across channel progress to current time
	double curr_time = global_time.get_time();
	double rem_time = curr_time - prev_time;  //this defines how far the update must proceed
	prev_time = curr_time; //advance prev_time to now
	while (rem_time > 0){  //update up-to current time
		chan_res_map temp_map = rem_requests.trim();  //get remaining positive requests
		int bits_to_move = temp_map.find_small();   //find a smallest remaining sending request
		int num_sending = temp_map.size();  //how many concurrent senders?
		if (num_sending == 0) return; // we are done sending  -- no more bits to send
		double svc_rate = channel_bw / num_sending;  //per sender service rate
		double time_increment = MIN(rem_time, (bits_to_move / svc_rate)); //time increment for this update
		bits_to_move = (int)((time_increment * svc_rate) + .5); //bits moved for each sender during time increment
		rem_requests.subtract_scalar(bits_to_move); //perform scalar subtraction of bits_to_move
		rem_time -= time_increment;  //decrement remaining time
	}
}

//use dedicated exclusion resource for a fixed time period
void exclusion_resource::use_resource(double time_delay){
	mut.lock(); //block waiting for free resource
	global_time.sleep(time_delay); //hold the resource for a fixed time period
	mut.unlock(); //release the resource
}