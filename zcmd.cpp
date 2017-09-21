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
#include <iostream>
#include <fstream>
#include "test_genz_sys.h"
#include "sim_time.h"
#include "zmmu.h"

void test_zmmu() {

	zmmu zmm;  //create a table

	zmmu_table_entry new_entry; //create a table entry
	new_entry.length = 10;
	new_entry.security_key = 0;

	for (unsigned i = 0; i < 1000; i += 100){
		new_entry.in_addr = i;
		int success = zmm.insert_entry(new_entry);
		*logfile << "insertion success=" << success << endl;
	}
	*logfile << endl;

	for (unsigned i = 0; i < 1000; i += 100){
		unsigned key = i;
		zmmu_table_entry* result = zmm.find_matching_entry(key);
		*logfile << "find_matching for key =" << key << endl;

		if (result == 0){
			*logfile << "failed" << endl;
			}
		else {
			*logfile << "entry_in_addr=" << result->in_addr << " entry_len=" << result->length << endl;
			}

	}
	*logfile << "deletion test" << endl;
	int success = zmm.delete_entry(900);
	unsigned key = 900;
	zmmu_table_entry* result = zmm.find_matching_entry(key);
	*logfile << "find_matching for key =" << key << endl;

	if (result == 0){
		*logfile << "failed" << endl;
	}
	else {
		*logfile << "entry_in_addr=" << result->in_addr << " entry_len=" << result->length << endl;
	}

}


int main() {
	ofstream outfile("Output_log.txt", ios::out); //open primary output file
	logfile = &outfile; //set global pointer to log file
	sim_start_time = global_time.get_time();


//	test_genz_sys_sndrcv();
//	test_genz_sys_ping_pong();
	test_genz_sys_put_pong();

//	test_genz_sys_indirect();

//	test_zmmu();
	
}
