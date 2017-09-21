#ifndef _SIM_SIGNAL_H
#define _SIM_SIGNAL_H

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
#include <condition_variable>
#include "sim_time.h"

template <class T>
class ordered_event_buffer_t  { //this class carries time ordered operand stream of type <T> from producer to consumer

private:
	multimap<double, T> buff_map; //container for delayed & time-ordered parameter stream
	mutex mut;

public:
	void send_at_time(double signal_time, T input){  //send a new entry which arrives at signal_time
		mut.lock();
		pair<double, T> val;
		val.first = signal_time; val.second = input;
		buff_map.insert(val);  //insert the new parameter into the ordered multimap
		mut.unlock();
	};

	int receive_earliest(T* result){ //get a next entry having the earliest notification time
		mut.lock();
		map<double, T>::iterator it = buff_map.begin();  //get pointer to oldest notificaiton
		if (it == buff_map.end()) { //is the map empty?
			mut.unlock();
			return 0; //empty map -- return fail
		}
		*result = it->second;
		buff_map.erase(it);
		mut.unlock();
		return 1; //return success
	};
};

class semaphore  
{   //semaphore counts producer signals to prevent overrun when consuming thread is tardy
private:
	std::mutex mut;
	std::condition_variable cond;
	unsigned long count; // Initialized as locked = 0.

public:
	semaphore();
	void notify();  //producer notifies consumer of event
	void wait();  //consumer waits until producer notification has ocurred
	bool try_wait(); //non-blocking test for event notification
	void notify_after_delay(double delta_t); //produce a semaphore signal with a fixed time delay
	   //the "start_watch_for_events" thread must be running to implement notify_with_delay
	void notify_at_time(double time); //produce a semaphore signal with a fixed time delay
};

class event_queue
{ //the event map associates execution time with a notification semaphore  
private:
	std::mutex mut;
	multimap<double, semaphore*> event_map;  // event list maps time to semaphore pointers
	int stopped;  //stop polling to implement graceful termination

public:
	event_queue();
	void insert(double finish_time, semaphore* s);  //insert event at desired finish_time with a pointer to a semaphore
	semaphore* retrieve(double curr_time);  //retrieves the next ready event with semaphore
	int is_stopped();  //is the queue polling loop stopped?
	void start();  //start the polling loop
	void stop(); //stop the polling loop
};

void sim_start_watch_for_events(); //start event handling loop
void sim_stop_watch_for_events(); //stop event handling loop

template <class T>
class msg_delay_channel{  //time-delayed message delivery -- message of type <T>
private:
	semaphore sem;
	ordered_event_buffer_t<T> buff;
public:
	void send_at_time(T* input, double delay){
		double curr_time = global_time.get_time();
		double target_time = curr_time + delay;
		buff.send_at_time(target_time, *input);
		sem.notify_at_time(target_time);
	};

	void receive(T* result){  //wait for the next signal and extract earliest arriving message in sequence
		sem.wait();
		int success = buff.receive_earliest(result);
	};

	int try_receive(T* result){
		int succcess = sem->try_wait();
		if (!success return 0);
		buff.receive_earliest(result);
		return 1;
	};
};

#else
#endif