#ifndef _SIM_RESOURCE_H
#define _SIM_RESOURCE_H

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

using namespace std;
#include <map>
#include <mutex>
#include <list>

class chan_res_map{ //channel resource map
	map<int, int> res_map; //holds remaining number of bits to be moved for each requestor (from prev time)
	list<int> unused_id_list; //used to recycle unused ID's
	int next_unused;

public:
	chan_res_map();
	void insert(int id, int amount);  //insert a new entry to the map
	void remove(int id);  //removes the "id" entry from the map
	chan_res_map trim();  //returns copy omitting all entries having a non-positive value field
	int find_small();  //find  value of smallest > 0. entry
	void subtract_scalar(int bits_sent); //subtract scalar value from entries in map -- result >= 0
	int get_unused_id();  //get an  unused id to index into the res_map
	double get_val(int id);  //return value field for this id
	bool done(int id);  //test the "id" communication reqeust for completeness
	int size();  //number of elements in the resource map
	friend class channel_time;
};

class channel_resource{  //shared communications resource (equal share per unit time)
	mutex mut;  //used to lock this resource channel
	volatile double channel_bw; //channel bandwidth in bits per second
	volatile double prev_time; //time at which last update was performed
	chan_res_map rem_requests; //maps remaining requests to number of bits to move across channel
	double calc_min_delay(int id); //calculates delay for "id" assuming no additional resource requests
	int insert(int bits_requested); //add new communication request of size bits_requested - return its id 
	void remove(int id); //remove completed communication request
	void update();  //bring time up to date - move all "bits" through the channel and advance prev_time

public:
	channel_resource(double bandwidth);  //create a channel resource with given bandwidth
	void channel_delay(int bits); //sleep for time needed to send "bits" through shared channel 
};

class exclusion_resource{  //dedicated resource used with settable delay
	mutex mut;

public:
	void use_resource(double time_delay);
};

#else
#endif