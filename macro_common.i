#line 1 "C:\\Users\\admin\\Desktop\\lapskran\\Lapskran\\macro_common.h"
#pragma once


namespace mengine {

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
		std::vector<btree::Token> body;
	};

	std::vector<Pattern> lex_pattern;
	std::vector<Pattern> ast_pattern;
	std::vector<Pattern> tac_pattern;
	std::vector<Pattern> hir_pattern;
	std::vector<Pattern> lir_pattern;
	std::vector<Pattern> asm_pattern;
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
	};


}
