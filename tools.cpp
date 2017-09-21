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


#include "tools.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>

double waste_time(int num_trips){
	double result = 0;
	for (int i = 0; i < num_trips; i++){
		for (int j = 0; j < 1000; j++){
			result += i;
		}
	}
	return result;
}

std::string IntToStr(int n)
{
	std::ostringstream result;
	result << n;
	return result.str();
}

std::string DoubleToStr(double d){
	std::ostringstream result;
	result << d;
	return result.str();
}

int StrToInt(const std::string& s)
{
	int result;
	std::istringstream ss(s);
	ss >> result;
	if (!ss) throw std::invalid_argument("StrToInt");
	return result;
}

mutex printlock;
void locking_print(ofstream& file, string output){
	printlock.lock();
	file << output << endl;
	printlock.unlock();
}

string hexify(int i)
{
	std::ostringstream os;
	os << std::hex << i;
	std::string s = os.str();
	return s;
}

void zassert(int i){
	if (!i){
		while (1){ // infinite loop on assertion violation
		}
	}
}

string get_chars(string& input, string chars){
	//return string with matching chars and drop matched chars from input
	int pos = input.find_first_not_of(chars);
	int len = input.size();
	string result;
	if (pos == string::npos){ //did not find any non-matching chars
		result = input;
		input = "";
		return result;
	}
	result = input.substr(0, pos);
	string replace_input = input.substr(pos, len - pos); //get remaining sring
	input = replace_input;
	return result;
}

string get_not_chars(string& input, string chars){
	size_t pos = input.find_first_of(chars);
	string result;
	if (pos == std::string::npos) {
		result = input;
		input = "";  //remove returned characters from input
		return result; //did not find a match
	}
	int len = input.size();
	result = input.substr(0, pos);
	string replace_input = input.substr(pos, len - pos); //get remaining string
	input = replace_input;
	return result;
}

void get_char(string& input, char parse_char){
	get_chars(input, " "); //drop leading spaces
	int length = input.size();
	zassert(input[0] == parse_char);
	string return_string = input.substr(1, length - 1);
	input = return_string;
}

void get_final_char(string& input, char parse_char){
	string remaining_input = input;
	int length = remaining_input.size();
	while (remaining_input[length - 1] == ' '){ //strip trailing spaces
		remaining_input = remaining_input.substr(0, length - 1);
		length = remaining_input.size();
		zassert(length>0);
	}
	zassert(remaining_input[length - 1] == parse_char);  //better match the parse char
	remaining_input = remaining_input.substr(0, length - 1);
	input = remaining_input;
}

int parse_value(string& input){
	string digits = "0123456789";
	get_chars(input, " "); //ingore  leading spaces
	string return_digits = get_chars(input, digits);
	get_chars(input, " ");
	return StrToInt(return_digits);
}

string strip_leading(string input, string ch){  //result returns input strriped of any leading characters contained in ch
	int input_size = input.size();

	string result;
	int dropped_chars = 0;
	while (dropped_chars < (int) input.size()){
		int found = 0;
		for (int i = 0; i< (int) ch.size(); i++){ //test current char for match in string ch
			if (input[dropped_chars] == ch[i]) {
				found = 1;
				break;
			}
		}
		if (found == 0) break;  //exit on first non-matching char
		dropped_chars++; //found a matching character - advance to next character
	}
	result = input.substr(dropped_chars, input.size() - dropped_chars);
	return result;
}

string strip_trailing(string input, string ch){  //result return input striped  of any trailing characters contained in ch
	int input_size = input.size();
	string result;
	while (input_size>0){
		int found = 0;
		for (int i = 0; i< (int) ch.size(); i++){ //test last char for match in string ch
			if (input[input_size - 1] == ch[i]) {
				found = 1;
				break;
			}
		}
		if (found == 0) break;  //exit on first non-matching char
		input_size--; //decrease llength for each matching char
	}
	result = input.substr(0, input_size);
	return result;
}

string strip_leading_trailing(string input, string ch){
	string striplead = strip_leading(input, ch);
	string result = strip_trailing(striplead, ch);
	return result;
}

bool parse_keyword(string& input, string& result){ //each keyword is followed by a ":"
	input = strip_leading(input, " ");
	string keyword = get_not_chars(input, ":");
	if (input.size() == 0) return false;
	if (input[0] == ':'){ //we found the keyword (should always be true)
		input = input.substr(1, input.size() - 1); //remove the ":" from the input stream
		result = strip_leading_trailing(keyword, " ");
		return true;
	}
	return false; //failed to recognize a keyword
}

print_clock::print_clock(){
	start = clock(); //set the start time
	previous = start;
}

void print_clock::print_total(string label, ostream& s){
	clock_t current = clock();
	double duration = (double)(current - start) / CLOCKS_PER_SEC;
	s << "PRINT_DIFF:" << label << "-  " << duration << " seconds   " << duration / 60. << " minutes" << endl;
	previous = current;
}

double print_clock::print_diff(string label, ostream& s){
	clock_t current = clock();
	double duration = (double)(current - previous) / CLOCKS_PER_SEC;
	s << "PRINT_DIFF:" << label << "-  " << duration << " seconds   " << duration / 60. << " minutes" << endl;
	previous = current;
	return duration; //return time difference in seconds
}


static mutex inc_lock;
unsigned fetch_add(volatile unsigned& var, unsigned inc){
	inc_lock.lock();
	int result = var;
	var += inc;
	inc_lock.unlock();
	return result;
}

static mutex dec_lock;
unsigned fetch_rem(volatile unsigned& var, unsigned rem) {
	dec_lock.lock();
	int result = var;
	var -= rem;
	dec_lock.unlock();
	return result;
}