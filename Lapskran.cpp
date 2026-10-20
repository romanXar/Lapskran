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
#include <set>
#include <deque>


#include "command.h"
#include "importer.h"
#include "interning.h"
#include "properties.h"
#include "token.h"




#include "macro_engine.h"


#include "orig.h"
#include "lex.h"



int main(int argc, char* argv[]) {


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
	*/
	/*
	build:
	*/
	auto start = std::chrono::steady_clock::now();

//	std::string path = argv[2];
	std::string path = "C:\\Users\\admin\\Desktop\\lang_lapskran\\main.lps";
	std::string source = expand(path);   // ← вот здесь
	/*
	for (int i = 0; i < source.size(); i++) {

	std:: cout << source[i];

	}*/




	std::vector<std::string> words = gen_words_array(source);





	std::vector<std::string> tokens = gen_tokens_array(words);

	std::vector<uint32_t> ntokens = intern_all(tokens);

	ntokens = mengine::init_pipeline(ntokens);
	ntokens = mengine::init_funs(ntokens);
	ntokens = mengine::init_patterns(ntokens);

	btree::create_tree("main");
	btree::use("main");

	for (int i = 0; i < (int)ntokens.size(); i++) {
		btree::Token t = { btree::gen_id(), ntokens[i] };
		btree::push_back(t);
	}

	tokens.clear();
	tokens.shrink_to_fit();

	for (auto& v : mengine::patterns)
		for (std::size_t j = 0; j < v.size(); ++j)
			mengine::match(v, (int)j);

	auto end = std::chrono::steady_clock::now();





	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	//std::cout << "elapsed: " << ms << " ms" << std::endl;

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