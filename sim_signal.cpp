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

#include "sim_signal.h"

event_queue global_event_queue;  //declare the global event queue

semaphore::semaphore(){
	count = 0;
}

void semaphore::notify() {
	std::unique_lock<decltype(mut)> lock(mut);
	++count;
	cond.notify_one();
	//mutex_ unlocked in unique_lock destructor call
}

void semaphore::wait() {
	std::unique_lock<decltype(mut)> lock(mut);
	while (!count) // Handle early wake-ups.
		cond.wait(lock);
	--count;
}

bool semaphore::try_wait() {
	std::unique_lock<decltype(mut)> lock(mut);
	if (count) {
		--count;
		return true;
	}
	return false;
}

void semaphore::notify_after_delay(double delay) {
	double signal_time = delay + global_time.get_time();
	global_event_queue.insert(signal_time, this);
		//insert (time, ref-to-semaphore) on global event queue
}

void semaphore::notify_at_time(double absolute_time) {
	global_event_queue.insert(absolute_time, this);
	//insert (time, ref-to-semaphore) on global event queue
}

 event_queue::event_queue(){
	 stopped = 0;
}

void event_queue::insert(double signal_time, semaphore* s){
	pair<double, semaphore*> val;
	val.first = signal_time; val.second = s;
	mut.lock();
	event_map.insert(val);
	mut.unlock();
}

semaphore* event_queue::retrieve(double curr_time){  //retrieve next semaphore (if it is ready at curr_time) 
	mut.lock();
	map<double, semaphore*>::iterator it = event_map.begin(); //get iterator to earliest entry
	if (it == event_map.end()) {
		mut.unlock();
		return 0; //no semaphore event to return
	}
	double event_time = it->first;
	if (event_time <= curr_time) { //is this event ready at curr_time?
		semaphore* result= it->second;  
		event_map.erase(it); //remove entry
		mut.unlock();
		return result; //return semaphore result
	}
	mut.unlock();
	return 0; //semaphore event not ready at this time
}

void event_queue::start(){
	stopped = 0;
}

void event_queue::stop(){
	stopped = 1;
}

int event_queue::is_stopped(){
	return stopped;
}

void sim_start_watch_for_events(){  
	global_event_queue.start();  //make sure queue is not stopped
	while (!global_event_queue.is_stopped()){ //event handling polling loop
		double curr_time = global_time.get_time();
		semaphore* s = global_event_queue.retrieve(curr_time); //get the next ready signal
		if (s != 0){
			s->notify(); //send the notificaiton
		}
	}
}

void sim_stop_watch_for_events(){ 
	global_event_queue.stop(); //stop the event handling loop
}