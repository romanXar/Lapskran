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
	PIF,
	pINT,
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
	bool is_int;
	int64_t num;
};

InternEntry id_table[100000] = {
	{ PIF,      "pif",      false, 0 },
	{ pINT,     "pint",     false, 0 },
	{ PLABEL,   "plabel",   false, 0 },
	{ PGOTO,    "pgoto",    false, 0 },
	{ PEND,     "pend",     false, 0 },
	{ LEVEL,    "level",    false, 0 },
	{ LEX,      "lex",      false, 0 },
	{ AST,      "ast",      false, 0 },
	{ TAC,      "tac",      false, 0 },
	{ HIR,      "hir",      false, 0 },
	{ LIR,      "lir",      false, 0 },
	{ ASM,      "asm",      false, 0 },
	{ ELSE,     "else",     false, 0 },
	{ OR,       "or",       false, 0 },
	{ AND,      "and",      false, 0 },
	{ REP,      "rep",      false, 0 },
	{ END_COND, "end_cond", false, 0 },
	{ TOK,      "tok",      false, 0 },
	{ TOKWN,    "tokwn",    false, 0 },
	{ NEWLINE,  "\n",       false, 0 },
	{ LT,       "<",        false, 0 },
	{ GT,       ">",        false, 0 },
	{ EQ,       "=",        false, 0 },
	{ BANG,     "!",        false, 0 },
	{ PLUS,     "+",        false, 0 },
	{ MINUS,    "-",        false, 0 },
	{ DOT,      ".",        false, 0 },
	{ COMMA,    ",",        false, 0 },
	{ LPAREN,   "(",        false, 0 },
	{ RPAREN,   ")",        false, 0 },
	{ LBRACK,   "[",        false, 0 },
	{ RBRACK,   "]",        false, 0 },
	{ TOKEN,    "token",    false, 0 },
	{ wORD,     "word",     false, 0 },
	{ ANY,      "any",      false, 0 },
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

		std::cout << get_str(tok.name);

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