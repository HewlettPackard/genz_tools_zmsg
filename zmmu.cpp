#include "zmmu.h"
#include "tools.h"


int zmmu_table_entry::is_overlap(zmmu_table_entry in ) {
	unsigned start1 = in_addr;
	unsigned end1 = start1 + length -1; //last byte in region
	unsigned start2 = in.in_addr;
	unsigned end2 = in.in_addr + in.length - 1; //last byte in region
	if (start1 <= start2 && end1 >= end2) return true;  //1 encloses 2
	if (start2 <= start1 && end2 >= end1) return true;  //2 encloses 1
	if (start1 <= start2 && end1 >= start2) return true; //1 ends inside 2
	if (start2 <= start1 && end2 >= end1) return true; // 2 ends inside 1
	return false;  //there is no overlap
}

unsigned zmmu_table_entry::get_target_address(unsigned src_addr) {
	unsigned gen_z_start_addr = src_addr + (out_addr -in_addr);
	return gen_z_start_addr;
}

unsigned zmmu_table_entry::get_copy_length(unsigned src_addr, unsigned requested_len) {
	unsigned end_of_window = in_addr + length;
	unsigned copy_length = MIN(requested_len, end_of_window - src_addr); //do not copy past end of window
	return copy_length;
}

int zmmu::insert_entry(zmmu_table_entry in) {
	unsigned start = in.in_addr;
	unsigned end = in.in_addr + in.length;
	
	map<unsigned, zmmu_table_entry>::iterator it = zmmu_table.upper_bound(start); //get smallest strictly > entry
	if (it != zmmu_table.begin()){
		it--;  //get largest <= entry
		while (true) {
			if (it == zmmu_table.end()) break;
			zmmu_table_entry* current = &it->second;
			if (current->in_addr > end) break;
			

			if (in.is_overlap(*current)) {
				return false;
			}
			it++;
		}
	}
	zmmu_table[start] = in;
	return true;
}

 zmmu_table_entry* zmmu::find_matching_entry(unsigned address){//unsigned* length, unsigned* security){ //zmmu_table_entry* result) {

	map<unsigned, zmmu_table_entry>::iterator it = zmmu_table.upper_bound(address); //get smallest strictly > entry 
	if (it == zmmu_table.begin()) return 0;
	it--;  //get largest <= entry
	unsigned window_start = it->first;
	zmmu_table_entry value = it->second;
	if (address > window_start + value.length - 1) return 0;  //address is outside next smaller region
	return &(it->second);
}

int zmmu::delete_entry(unsigned start_address) {
	auto it = zmmu_table.find(start_address);
	if (it == zmmu_table.end()) return false;  //could not find start address in the map
	zmmu_table.erase(it); 
	return true; //successful erasure
}