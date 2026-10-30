#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <utility>
#include <charconv>
#include <algorithm>
#include <windows.h>
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_map>
#include <map>


struct Token {
	uint32_t id;
	uint32_t name;
};

inline uint32_t gen_id() {
	static uint32_t id_total = 1;
	return id_total++;
}
#include "command.h"
#include "importer.h"
#include "interning.h"
#include "properties.h"
#include "token.h"
#include "VStack.h"

#include "macro_engine.h"


#include "orig.h"
#include "lex.h"
using namespace mengine;

#define NEXT uint8_t next_op = static_cast<uint8_t>(bytecode[cur].op); if (next_op >= OP_COUNT) goto L_DEFAULT; goto *dispatch_table[next_op]; 


int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);   
	SetConsoleCP(CP_UTF8);         

#include "goto_table.inc"

	VMemory StackFrame;
	VMemory CallFrame;
	VMemory ArgFrame;


	ArgFrame.Alloc(65536);


	intptr_t cur = 0;




	//return 0;



		/*
		if (argc < 2 or (argc == 2 and std::string(argv[1]) == "help")) {
			print_help();
			return 0;
		}
		else if (std::string(argv[1]) == "add_path") {
			add_path();
			return 0;
		}
		else if (std::string(argv[1]) == "build") {
			if (argc < 3) {
				std::cout << "error: build requires a file\n";
				return 0;
			}
			goto build;
		}
		else {
			print_help();
			return 0;
		}


		build:
		*/


	auto start = std::chrono::steady_clock::now();





	//	std::string path = argv[2];
	std::string path = "C:\\Users\\admin\\Desktop\\lang_lapskran\\main.lps";
	vector<Word> words = expand(path);


	vector<Word> tokens = gen_tokens_array(words);

	vector<Token> ntokens = intern_all(tokens);



	//ntokens = mengine::init_pipeline(ntokens);
	ntokens = mengine::init_funs(ntokens);
	//ntokens = mengine::init_patterns(ntokens);



	NEXT;


L_ALLOC:
	{
		StackFrame.Alloc(bytecode[cur].mem1);
		cur++;
		NEXT;
	}
L_DEALLOC:
	{
		StackFrame.DeAlloc(bytecode[cur].mem1);
		cur++;
		NEXT;
	}
L_MOV_II_VV:
	{
		cur++;
		NEXT;
	}
L_MOV_II_VC:
	{
		const Command& cmd = bytecode[cur];

		StackFrame.Set(cmd.mem1, cmd.mem2);


		cur++;
		NEXT;
	}
L_MOV_IN_ARG:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(cmd.mem1, StackFrame.Get(cmd.mem2));
		cur++;
		NEXT;
	}
L_MOV_IN_ARG_CONST:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(cmd.mem1, cmd.mem2);
		cur++;
		NEXT;
	}
L_MOV_OUT_ARG:
	{
		const Command& cmd = bytecode[cur];
		StackFrame.Set(cmd.mem1, ArgFrame.Get(cmd.mem2));
		cur++;
		NEXT;
	}

L_IF_LE_II_VC:
	{


		const Command& cmd = bytecode[cur];
		bool cond = StackFrame.Get(cmd.mem2) <= cmd.mem3;

		if (cond) {
			cur = cur + 1;
		}
		else {
			cur = cmd.mem1;
		}

		NEXT;
	}

L_ADD_III_VVV:
	{

		const Command& cmd = bytecode[cur];
		StackFrame.Set(cmd.mem1, StackFrame.Get(cmd.mem2) + StackFrame.Get(cmd.mem3));
		cur++;

		NEXT;
	}
L_ADD_III_VVC:
	{

		const Command& cmd = bytecode[cur];
		StackFrame.Set(cmd.mem1, StackFrame.Get(cmd.mem2) + cmd.mem3);
		cur++;

		NEXT;
	}
L_SUB_III_VVC:
	{
		const Command& cmd = bytecode[cur];
		StackFrame.Set(cmd.mem1, StackFrame.Get(cmd.mem2) - cmd.mem3);
		cur++;
		NEXT;
	}
L_PRINT:
	{
		const Command& cmd = bytecode[cur];
		int i = cmd.mem1;
		while (true) {
			if (pushcode[i].op == VAR_I) {
				std::cout << StackFrame.Get(pushcode[i].mem1);

			}
			else if (pushcode[i].op == LIT_S) {
				std::cout << str[pushcode[i].mem1];

			}
			else {
				break;

			}
			i++;
		}

		cur++;
		NEXT;
	}
L_CALL:
	{
		const Command& cmd = bytecode[cur];
		CallFrame.Push(cur + 1);
		cur = cmd.mem1;
		StackFrame.Alloc(cmd.mem2);
		NEXT;
	}
L_CALL_C:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(1, cmd.mem3);
		CallFrame.Push(cur + 1);
		cur = cmd.mem1;
		StackFrame.Alloc(cmd.mem2);
		NEXT;
	}
L_CALL_V:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(1, StackFrame.Get(cmd.mem3));
		CallFrame.Push(cur + 1);
		cur = cmd.mem1;
		StackFrame.Alloc(cmd.mem2);
		NEXT;
	}
L_RET_I_V:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(1, StackFrame.Get(cmd.mem2));
		cur = CallFrame.Pop();
		StackFrame.DeAlloc(cmd.mem1);
		NEXT;
	}
L_RET_I_C:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(1, cmd.mem2);
		cur = CallFrame.Pop();
		StackFrame.DeAlloc(cmd.mem1);
		NEXT;
	}
L_RET_ADD_II_VV:
	{
		const Command& cmd = bytecode[cur];
		ArgFrame.Set(1, StackFrame.Get(cmd.mem2) + StackFrame.Get(cmd.mem3));
		cur = CallFrame.Pop();
		StackFrame.DeAlloc(cmd.mem1);
		NEXT;
	}
L_GOTO:
	{
		const Command& cmd = bytecode[cur];
		cur = cmd.mem1;
		NEXT;
	}
L_eOF:
	{
		goto eof;
	}
L_DEFAULT:
	{
		std::cout << "CRUSH: cur = " << cur << std::endl;
		goto eof;
	}


eof:










	mengine::TStream bmain;

	for (int i = 0; i < (int)ntokens.size(); i++) {
		bmain.push_back(ntokens[i]);
	}



	for (auto& v : mengine::patterns)
		for (std::size_t j = 0; j < v.size(); ++j)
			mengine::match(bmain, v, (int)j);

	auto end = std::chrono::steady_clock::now();





	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	//std::cout << "elapsed: " << ms << " ms" << std::endl;

	int iii = 0;
	

	return 0;
}