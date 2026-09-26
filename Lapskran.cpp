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

	btree::Tree tree;
	btree::use(tree);
	for (int i = 0; i < tokens.size();i++) {
		btree::Token t = { btree::gen_id(),tokens[i] };
		btree::push_back(t);
	}


	tokens.clear();	tokens.shrink_to_fit();
	//for (int i = 0; i < tree.total; i++) {
	//	btree::Token& tok = btree::get(i);
	//	std::cout << tok.name;

	//}
	//cout << endl;

	for (int i = 0; i < mengine::lex_pattern.size();i++) {
		mengine::lex::match(tree, i);
		//cout << "stage" << endl;

		//for (int i = 0; i < tree.total; i++) {
		//	btree::Token& tok = btree::get(i);
		//	std::cout << tok.name;

		//}

	}



	//for (int i = 0; i < tree.total; i++) {
	//	btree::Token& tok = btree::get(i);
	//	std::cout << i << ": id=" << tok.id << " name=\"" << tok.name << "\"\n";

	//}
	//std::cout << "result code petchataet kak on budiet vigliadet" << endl;
	for (int i = 0; i < tree.total; i++) {
		btree::Token& tok = btree::get(i);
		std::cout << tok.name;

	}

	return 0;
}