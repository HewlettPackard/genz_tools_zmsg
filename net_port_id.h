#ifndef _NET_PORTID_H
#define _NET_PORTID_H

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

	


//this class is used in "maps" to lookup ports
class net_port_id{ 
	int hport_index;  //this is the Gen-Z component ID
	int sport_index; //this index selects the logical port at the target component
public:
	net_port_id();
	net_port_id(int hport, int sport);
	net_port_id(unsigned input);

	void set_hport_inx(int);
	void set_sport_inx(int);
	int get_hport_inx();
	int get_sport_inx();
	unsigned get_unsigned();
	void set_unsigned();
	operator unsigned();

	friend bool operator==(const net_port_id& a, const net_port_id& b);
	friend bool operator<(const net_port_id& a, const net_port_id& b);
};

bool operator==(const net_port_id& a, const net_port_id& b);
bool operator<(const net_port_id& a, const net_port_id& b);

unsigned make_nodeportid(unsigned hport, unsigned sport);
unsigned make_nodeportid(net_port_id input);
unsigned get_sport_nodeportid(unsigned input);
unsigned get_hport_nodeportid(unsigned input);


#endif

