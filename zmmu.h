#ifndef ZMMU_H
#define ZMMU_H
#include <map>
using namespace std;


class req_entry{
public:
	int remote_cid;  //component id

};
class resp_entry{
public:
	int access_rights; //read-only=0, allow_write=1
	int proc_id; //used for physical to virtual translation (unused here)
};
class zmmu_table_entry{
public:
	//entries that are common to both requestoing and responding zmmu
	unsigned in_addr; //starting address for input window
	unsigned length;  //length of window in number of bytes
	unsigned security_key;  //key for security check
	unsigned out_addr;  //start address for output window 
	union{
		req_entry req;  //entries that are unique to requestor
		resp_entry resp;  //entries that are unique to responder
	};

	int is_overlap(zmmu_table_entry); //function checks for overlap between two entries
	unsigned get_target_address(unsigned souce_address);
	unsigned get_copy_length(unsigned source_address, unsigned length);
};

class zmmu{
private:
	map<unsigned, zmmu_table_entry> zmmu_table;

public:
	int insert_entry(zmmu_table_entry in);
	zmmu_table_entry*  find_matching_entry(unsigned key); 
	int delete_entry(unsigned key);

};


#endif
