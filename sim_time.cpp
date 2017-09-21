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

#include "sim_time.h"
#include <thread>

/* Mike Schlansker Hewlett-Packard Enterprise 6-28-2017 */

#define TIME_SCALE 1000.  //time scale slowdown for simulation system


sim_time::sim_time(){
	slow_down = 1.;  //default slowdown
#ifndef CHRONO_T
	start_time = clock(); //set start time at construction
#else
	start_time = std::chrono::high_resolution_clock::now(); //set start time at construction 
#endif
}

sim_time::sim_time(const double slow_down_in){
	slow_down = slow_down_in;
//set the start time to the time at construction 
#ifndef CHRONO_T
	start_time = clock(); 
#else
	start_time = std::chrono::high_resolution_clock::now(); 
#endif
}

void sim_time::init(const double slow_down_in){  //reinitialize class
	slow_down = slow_down_in;
	//set the start time to the time at re-initialization 
#ifndef CHRONO_T
	start_time = clock(); 
#else
	start_time = std::chrono::high_resolution_clock::now();
#endif
}

double sim_time::get_time(){  //retrieve the curerent scaled time

#ifndef CHRONO_T
	clock_t curr_time = clock();
	int rel_ticks = curr_time - start_time;
	double rel_time = rel_ticks / (double)CLOCKS_PER_SEC;
#else
	std::chrono::time_point<std::chrono::high_resolution_clock> curr_time = std::chrono::high_resolution_clock::now();
	double rel_time = std::chrono::duration_cast<std::chrono::microseconds>(curr_time - start_time).count()*.000001;
#endif
	double result = rel_time / slow_down;
	return result;
}

void sim_time::sleep(double scaled_time){
	int micro_seconds_delay = (int)(scaled_time * slow_down * 1000000.);
	std::this_thread::sleep_for(std::chrono::microseconds(micro_seconds_delay));
}

void sim_time::sleep_until(double target_time){
	double time_delay = target_time - get_time();  //delay = (target tinme - get current time)
	if (time_delay <= 0.) return;
	sleep(time_delay);
	return;
}

double sim_time::get_slow_down(){
	return slow_down;
}

sim_time global_time(TIME_SCALE); //initialize the global_timer
