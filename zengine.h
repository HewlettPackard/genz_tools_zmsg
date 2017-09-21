#ifndef _ZENG_H
#define _ZENG_H

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

#include "zrecord.h"
//these are the three distinct states for the z-engine
#define ZENG_IDLE  0  //ready for new request
#define ZENG_RUNNING  1  //request submitted
#define ZENG_DONE  2   //operation comnpleted - engine needs completion proessing

struct genz_engine {
public:
	volatile int pid;  //physical id of the engine
	volatile unsigned state; //engine is in in one of three states: IDLE, RUNING, DONE
	volatile int halt; // will use later (to halt/freeze the interface)
	volatile int cmd_status; // status of the ZCMD command execution  (success==1)
	volatile int s_intf_id; // local logical interface id to process command
	volatile int cmd_index; //index of command in sport_intf command array
	volatile int seq_no;  //sequence number needed to identify completions (mss)
	zrecord command;  //holds a copy of the currently executing command (mss)
};

void init_genz_engine(genz_engine*, int mtu, int index);
int is_busy_genz_engine(genz_engine* q);
void set_idle_genz_engine(genz_engine* q);  //retire the head element when done processing
unsigned set_running_genz_engine(genz_engine* q); //get next entry at tail of queue
int is_idle_genz_engine(genz_engine* q);
int is_done_genz_engine(genz_engine* q); // finished processing the zengine
int get_s_intf(genz_engine* q);
int get_zeng_cmd_status(genz_engine* q);
void set_zeng_done(genz_engine* q);

#endif