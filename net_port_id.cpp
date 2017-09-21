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

#include "net_port_id.h"
#include "sport_intf.h"

// routines used to manipulate (hport, sport) id and hash  
unsigned make_nodeportid(unsigned hport, unsigned sport){
	unsigned result;
	result = sport << 16;
	result = result | (hport & 0xffff);
	return result;
}

unsigned make_nodeportid(net_port_id input){
	unsigned result = make_nodeportid(input.get_hport_inx(), input.get_sport_inx());
	return result;
}

unsigned get_sport_nodeportid(unsigned input){
	return input >> 16;
}

unsigned get_hport_nodeportid(unsigned input){
	return input & 0xffff;
}

net_port_id::net_port_id(){
	hport_index = -1;
	sport_index = -1;
}
net_port_id::net_port_id(int hport, int sport){
	hport_index = hport;
	sport_index = sport;
}

net_port_id::net_port_id(unsigned input){
	sport_index = get_sport_nodeportid(input);
	hport_index = get_hport_nodeportid(input);
}

void net_port_id::set_hport_inx(int hport){
	hport_index = hport;
}

void net_port_id::set_sport_inx(int sport){
	sport_index = sport;
}

int net_port_id::get_hport_inx(){
	return hport_index;
}
int net_port_id::get_sport_inx(){
	return sport_index;
}

net_port_id::operator unsigned(){
	return make_nodeportid(hport_index, sport_index);
}

unsigned  net_port_id::get_unsigned(){  //convert to unsigned notation
	return make_nodeportid(hport_index, sport_index);
}

bool operator==(const net_port_id& a, const net_port_id& b){
	if ((a.hport_index == b.hport_index) && (a.sport_index == b.sport_index)) return true;
	return false;
}

bool operator<(const net_port_id& a, const net_port_id& b){
	if (a.hport_index < b.hport_index) return true;
	if ((a.hport_index == b.hport_index) && (a.sport_index < b.sport_index)) return true;
	return false;
}