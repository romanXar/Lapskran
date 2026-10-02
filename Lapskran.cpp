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

#include "properties.h"
#include "token.h"
#include "macro_engine.h"
#include "orig.h"
#include "lex.h"



	int main(int argc, char* argv[]) {




	
		std::string path = "C:\\Users\\admin\\Desktop\\lapskran\\Lapskran\\test.lps";

		std::vector<std::string>  words = gen_words_array(path);
		std::vector<std::string> tokens = gen_tokens_array(words);


		tokens = mengine::init(tokens);


		btree::create_tree("main");
		btree::use("main");


		for (int i = 0; i < tokens.size();i++) {
			btree::Token t = { btree::gen_id(),tokens[i] };
			btree::push_back(t);
		}


		tokens.clear();	tokens.shrink_to_fit();



		for (int i = 0; i < mengine::lex_pattern.size();i++) {
			mengine::lex::match(i);
	

		}

		for (int i = 0; i < btree::get_length(); i++) {
			btree::Token& tok = btree::get(i);
			std::cout << tok.name;
			if (tok.name != "\n") {

				std::cout << " ";

			}

		}

		return 0;
		
	}