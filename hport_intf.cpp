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


#include "hport_intf.h"

void init_hport_descr(hport_descr* p, unsigned hport_id_in){
	p->num_cmd_ports = 0;
	p->hport_id = hport_id_in;

	//initiate queues - used for processing add new ports - or other instructions from the client
	init_lfqueue_s(&p->avail_hport_cmd_records_s);
	init_lfqueue_s(&p->hport_cmd_to_process_cmd_s);
	init_lfqueue_s(&p->hport_cmd_to_process_rcv_s);

	for (unsigned i = 0; i < NUM_CMDS; i++){
		int success = push_tail_lfqueue_s(&p->avail_hport_cmd_records_s, i);
		if (success == 0) break;  // no more space
	}

	for (int i = 0; i < MAX_CMD_PORTS; i++){
		p->sport_in_use_vect[i] = 0;
	}

	for (int i = 0; i < MAX_RCV_PORTS; i++){
		p->rcv_port_in_use_vect[i] = 0;
	}
}
