#ifndef _SIM_TIME_H
#define _SIM_TIME_H

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



#include <time.h>
#include <chrono>

#define CHRONO_T  //use "chrono" or "clock_t"


//*************scaled time simulation system


class sim_time{
	double slow_down;  //read only variable after initialization

#ifndef CHRONO_T
	clock_t start_time; //read only variable after initialization
#else
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
#endif

public:
	sim_time(); //construct with default slow down (1)
	sim_time(const double slow_down); //constructor specifies slow down
	void init(const double slow_down); 
	double get_time();
	void sleep(double scaled_time);  //sleep for a given period of time
	void sleep_until(double scaled_time); //sleep until absolute time -- (not working yet)
	double get_slow_down();
};

extern sim_time global_time;  //this is the global timer

#else
#endif