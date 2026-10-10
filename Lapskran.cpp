#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <utility>
#include <unordered_map>
#include <map>
#include <charconv>
#include <algorithm>

#pragma once

enum Reserved {
	NOT_FOUND,
	PIF,
	PWHILE,
	pINT,
	pSTRING,
	PLABEL,
	PGOTO,
	PEND,

	LEVEL,
	LEX,
	AST,
	TAC,
	HIR,
	LIR,
	ASM,
	ELSE,
	OR,
	AND,
	REP,
	END_COND,
	TOK,
	TOKWN,
	NEWLINE,
	PNEWLINE,

	LT,
	GT,
	EQ,
	BANG,
	PLUS,
	MINUS,
	DOT,
	COMMA,
	LPAREN,
	RPAREN,
	LBRACK,
	RBRACK,

	TOKEN,
	wORD,
	ANY,

	ENUM_LEN
};
struct InternEntry {
	uint32_t id;
	std::string text;
	bool is_int = false;
	int64_t num = 0;
};

InternEntry id_table[1000] = {
	{ NOT_FOUND, "" },
	{ PIF,       "pif" },
	{ PWHILE,    "pwhile" },
	{ pINT,      "pint" },
	{ pSTRING,   "pstring" },
	{ PLABEL,    "plabel" },
	{ PGOTO,     "pgoto" },
	{ PEND,      "pend" },
	{ LEVEL,     "level" },
	{ LEX,       "lex" },
	{ AST,       "ast" },
	{ TAC,       "tac" },
	{ HIR,       "hir" },
	{ LIR,       "lir" },
	{ ASM,       "asm" },
	{ ELSE,      "else" },
	{ OR,        "or" },
	{ AND,       "and" },
	{ REP,       "rep" },
	{ END_COND,  "end_cond" },
	{ TOK,       "tok" },
	{ TOKWN,     "tokwn" },
	{ NEWLINE,   "\n" },
	{ PNEWLINE,  "pnewline" },
	{ LT,        "<" },
	{ GT,        ">" },
	{ EQ,        "=" },
	{ BANG,      "!" },
	{ PLUS,      "+" },
	{ MINUS,     "-" },
	{ DOT,       "." },
	{ COMMA,     "," },
	{ LPAREN,    "(" },
	{ RPAREN,    ")" },
	{ LBRACK,    "[" },
	{ RBRACK,    "]" },
	{ TOKEN,     "token" },
	{ wORD,      "word" },
	{ ANY,       "any" },
};


#include "interning.h"
#include "properties.h"
#include "token.h"
#include "macro_engine.h"
#include "orig.h"
#include "lex.h"

int main(int argc, char* argv[]) {

	auto start = std::chrono::steady_clock::now();

	std::string path = "C:\\Users\\admin\\Desktop\\lapskran\\Lapskran\\test.lps";

	std::vector<std::string> words = gen_words_array(path);
	std::vector<std::string> tokens = gen_tokens_array(words);

	std::vector<uint32_t> ntokens = intern_all(tokens);

	ntokens = mengine::init(ntokens);

	btree::create_tree("main");
	btree::use("main");

	for (int i = 0; i < (int)ntokens.size(); i++) {
		btree::Token t = { btree::gen_id(), ntokens[i] };
		btree::push_back(t);
	}

	tokens.clear();
	tokens.shrink_to_fit();

	for (int i = 0; i < (int)mengine::lex_pattern.size(); i++) {
		mengine::lex::match(i);
	}

	auto end = std::chrono::steady_clock::now();

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "elapsed: " << ms << " ms" << std::endl;

	int iii = 0;
	for (int i = 0; i < btree::get_length(); i++) {
		btree::Token& tok = btree::get(i);
		auto abc = get_str(tok.name);
		std::cout << abc;
		if (abc != "\n") {
			std::cout << " ";


		}

		//if (tok.name == get_str(tok.name)) {
		//}
	}
	for (int i = 0; i < btree::get_length(); i++) {
		btree::Token& tok = btree::get(i);
		if (tok.name == get_id("hello")) {
			iii++;
		}
	}
	//	std::cout << iii;
	return 0;
}