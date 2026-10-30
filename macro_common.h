#pragma once
using namespace std;

const int sentinel_before = 3;
const int sentinel_after = 50;



using TStream = CArray<Token, CArrayMode::TStream>;


enum CmdType {
	CMD_TOKWN = 1,
	CMD_TOK,
	CMD_END_COND,
	CMD_AND,
	CMD_OR,
	EQ_EQ,
	BANG_EQ,
	LT_EQ,
	GT_EQ,
};

struct Condition {
	uint32_t command_type = 0;
	std::vector<uint32_t> comand;
};

struct Pattern {
	int level = 0;
	uint32_t stage = 0;
	std::vector<Condition> condition;
	std::vector<Token> body;
};

std::vector <std::vector<Pattern>> patterns;

std::unordered_map<int32_t, int32_t> order;

std::vector<Pattern>* pattern = nullptr;

enum PropKey {
	PROP_IS_REF = 1,     
	PROP_IS_ARRAY,       
	PROP_IS_PROP         
};

int resize_ii = 0;

struct Macro_var {
	int64_t value = 0;
	std::string type = "";        
	std::string str_value = "";
	std::vector<uint32_t> array;  

	uint32_t size_var_id = 0;    
	uint32_t linked_array_id = 0; 

	int ref_frame_idx = -1;       
	uint32_t ref_var_name = 0;    
};


void MACRO_CRUSH(Token token,string err) {
	std::string fname = get_str(props::get(token, intern("file_name")));
	std::string line_in_f = get_str(props::get(token, intern("line_in_file")));

	std::cout << "Macro bastard: "+ fname + ", line " + line_in_f + " - " << err << endl;
	exit(-1);
}

