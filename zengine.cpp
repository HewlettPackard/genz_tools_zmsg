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

#include "zengine.h"
#include "tools.h"

void init_genz_engine(genz_engine* q, int mtu, int index) {
	q->pid = index;  //the local name 
	q->state = ZENG_IDLE;
	q->halt = 0;
}

int is_busy_genz_engine(genz_engine* q) {
	if (q->state == ZENG_IDLE) return 0;
	return 1;
}

void set_idle_genz_engine(genz_engine* q) { //set state to idle - show zeng is free to process next command
	q->state = ZENG_IDLE;
}

unsigned set_running_genz_engine(genz_engine* q) { //set state to running to indicate busy
	q->state = ZENG_RUNNING;
	return 0;
}

int is_idle_genz_engine(genz_engine* q) {
	if (q->state == ZENG_IDLE) return 1;
	return 0;
}

int is_done_genz_engine(genz_engine* q) {
	if (q->state == ZENG_DONE) return 1;
	return 0;
}

int get_s_intf(genz_engine* q)
{ // return sport intf id
	return q->s_intf_id;
}

int get_zeng_cmd_status(genz_engine * q) //get status of completed command
{ 
	return q->cmd_status;
}

void set_zeng_done(genz_engine * q)
{  
	q->state = ZENG_DONE;
}
