#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <iomanip>


#include "token.h"
#include "macro_engine.h"
#include "orig.h"
#include "lex.h"



using namespace std;
int main(int argc, char* argv[]) {

	std::string path = "C:\\Users\\admin\\Desktop\\lapskran\\Lapskran\\test.lps";

	vector<string>  words = gen_words_array(path);
	vector<string> tokens = gen_tokens_array(words);

	//words.clear();words.shrink_to_fit();

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