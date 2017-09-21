#ifndef _ZRECORD_H
#define _ZRECORD_H

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

#define Z_RECORD_MTU 1600  //bytes -- immediate mode Z message is limited to this length

#define MAX_NUM_INDIRECT_FRAMES 20  //number of frame ids that are carried in a message

#define ZRECORD_SIZE sizeof(zrecord)

struct zheader{
	unsigned type;  //the message type
	unsigned src_interface_id;   // identifies requesting (sending) context ID  -- (REQCTXID)
	unsigned dest_interface_id;  //identifies destination (responding) context ID for a network -- (RSPCTXID)
	unsigned len; //length of message in bytes
	unsigned local_address; //source address for PUT or target address for GET
	unsigned remote_address; //dest address for PUT_WZMMU or source address for GET_WZMMU
	unsigned remote_region_index; //index of selected memory region for indirect command
	unsigned remote_region_offset; //offset into selected memory region for indirect command
};

struct zrecord{  // format for input and output messages
	// these are not carried in packet
	unsigned fence;	//inidicates this command has fence bit set
	int seq_num; //sequence number used to identify command completions

	//these are carried in packet
	zheader header;
	unsigned data[(Z_RECORD_MTU+3)/4];  //byte count rounded up into word count
};

struct zcompletion_record{
public:
	unsigned status;
	unsigned seq_num;
};

#define IND_OP_REQ_CREDIT 1
#define IND_OP_RET_REM_CREDIT 2
#define IND_OP_RET_LOCAL_CREDIT 3
#define IND_OP_SEND_SEQ_MSGS 4

struct indirect_message_header{
	int op_code;
	int length;  //number of indirect messages
};

struct indirect_message{
	indirect_message_header header;
	int index_vector[MAX_NUM_INDIRECT_FRAMES];
};


#endif