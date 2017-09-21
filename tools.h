#ifndef _TOOLS_H
#define _TOOLS_H

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


#include <string>
#include <stdlib.h>
#include "string.h"
#include <time.h>

using namespace std;

double waste_time(int num_trips);

std::string IntToStr(int n);
std::string DoubleToStr(double d);
int StrToInt(const std::string& s);

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)
#define ABS(a) (a > 0 ? a : -a )
#define MOD_INC(a, b)   ( (a+1) % b )  //increment a modulo b

#define MAXDOUBLE   1.79769313486231570e+308 
#define MINDOUBLE -MAXDOUBLE  //most negative double
#define MAXFLOAT ((float)3.40282346638528860e+38)
#define MAXINT 2147483647

#define TRUE 1
#define FALSE 0

struct eqstr
{
	bool operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) == 0;
	}
};

void locking_print(ofstream& file,  string output);
string hexify(int i);
void zassert(int i); //error trap
string get_chars(string& input, string chars);
string get_not_chars(string& input, string chars);
void get_char(string& input, char parse_char);
void get_final_char(string& input, char parse_char);

int parse_value(string& input);
string strip_leading(string input, string ch);
string strip_trailing(string input, string ch);
string strip_leading_trailing(string input, string ch);
bool parse_keyword(string& input, string& result);

class print_clock{
	clock_t start, previous; //timer used to collect experiment run time
public:
	print_clock();
	void print_total(string label, ostream& s);
	double print_diff(string label, ostream& s);
};

unsigned fetch_add(volatile unsigned& var, unsigned inc);
unsigned fetch_rem(volatile unsigned& var, unsigned dec);
#else
#endif
