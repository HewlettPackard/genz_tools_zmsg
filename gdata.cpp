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


#include "gdata.h"
#include "network.h"
#include <thread>

global_data gdata;

ofstream* logfile;

clock_t Start;

double sim_start_time;

//sim_time global_time(TIME_SCALE); //initialize the global_timer

global_data::global_data() 
{ }


struct hport_descr hsvc_pdsc[NUM_SOCS]; 

vector<thread_arg> user_arg_vect;
vector<thread_arg> hdwr_arg_vect; //contains arguments for each created threads
vector<std::thread> hdwr_threads; //container for SOC kernel threads
vector<thread_arg> zeng_arg_vect; //contains arguments for each zeng threads
vector<std::thread> zeng_threads; //container for zengine threads


//declarations for indirect experiment
list<int> remote_credit_table[NUM_SOCS];
indirect_intf* remote_receiver_table[NUM_SOCS];
indirect_intf* remote_sender_table[NUM_SOCS];


