#pragma once
#include <charconv>
#include <algorithm>

namespace mengine {

	inline bool is_integer(const std::string& s) {
		if (s.empty()) return false;
		long long value;
		auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
		return ec == std::errc() && ptr == s.data() + s.size();
	}

	struct Condition {
		int vector_level = 0;
		std::string command_type;
		std::vector<std::string> comand;
	};

	struct Pattern {
		int level = 0;;
		std::string stage;

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

	void select_pattern(std::string name) {
		if (name == "lex") pattern = &lex_pattern;
		else if (name == "ast") pattern = &ast_pattern;
		else if (name == "tac") pattern = &tac_pattern;
		else if (name == "hir") pattern = &hir_pattern;
		else if (name == "lir") pattern = &lir_pattern;
		else if (name == "asm") pattern = &asm_pattern;
		else exit(-10);

	}

	std::vector<std::string> init(std::vector<std::string>& tk) {


		std::vector<std::string>  output_token;


		int nested = -1;
		int cond_level = 0;
		int old_cond_level = 0;

		bool is_cond = false;
		bool is_body = false;

		tk.push_back("");  // sentinel
		tk.push_back("");
		tk.push_back("");

		for (int i = 0; i < tk.size();i++) {
			if (is_integer(tk[i]) and tk[i + 1] == "level" and (tk[i + 2] == "lex" or tk[i + 2] == "ast" or tk[i + 2] == "tac" or tk[i + 2] == "hir" or tk[i + 2] == "lir" or tk[i + 2] == "asm") and tk[i + 3] == "pattern") {
				select_pattern(tk[i + 2]);
				Pattern p;
				p.level = std::stoi(tk[i]);
				p.stage = tk[i + 2];
				pattern->push_back(p);
				cond_level = 0;
				old_cond_level = 0;

				nested++;
			}
			else if (tk[i] == "pend") { nested--; }

			if (nested >= 0) {
				

				if (is_cond) {
					if (tk[i] == "<") {
						cond_level++;
					}
					else if (tk[i] == ">") {
						cond_level--;
					}

					else if (tk[i] == "[") {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().command_type = "beg_rep";
						cond_level++;
					}
					else if (tk[i] == "]") {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().command_type = "end_rep";
						cond_level--;
					}
					else {
						if ((tk[i] != "<" and tk[i] != "[") and (tk[i - 1] == "<" or tk[i - 1] == "[") and tk[i] != "or") {
							if (tk[i + 1] == ">") {
								pattern->back().condition.push_back({});
								pattern->back().condition.back().vector_level = cond_level - old_cond_level - 1;
								pattern->back().condition.back().command_type = "tokwn";
								pattern->back().condition.back().comand.push_back(tk[i]);
								old_cond_level = cond_level;
							}
							else if (tk[i + 2] == ">") {
								pattern->back().condition.push_back({});
								pattern->back().condition.back().vector_level = cond_level - old_cond_level - 1;
								pattern->back().condition.back().command_type = "tok";
								pattern->back().condition.back().comand.push_back(tk[i]);
								pattern->back().condition.back().comand.push_back(tk[i + 1]);
								old_cond_level = cond_level;
								i++;
							}
							else if (tk[i + 1] == "]") {
								pattern->back().condition.push_back({});
								pattern->back().condition.back().vector_level = cond_level - old_cond_level - 1;
								pattern->back().condition.back().command_type = "reptokwn";
								pattern->back().condition.back().comand.push_back(tk[i]);
								old_cond_level = cond_level;
							}
							else if (tk[i + 2] == "]") {
								pattern->back().condition.push_back({});
								pattern->back().condition.back().vector_level = cond_level - old_cond_level - 1;
								pattern->back().condition.back().command_type = "reptok";
								pattern->back().condition.back().comand.push_back(tk[i]);
								pattern->back().condition.back().comand.push_back(tk[i + 1]);
								old_cond_level = cond_level;
								i++;
							}

						}
						else if (tk[i] == "or") {
							pattern->back().condition.push_back({});
							pattern->back().condition.back().vector_level = cond_level - old_cond_level + 1;
							pattern->back().condition.back().command_type = tk[i];
							old_cond_level = cond_level;

						}
					}

					if ((tk[i] == ">" or tk[i] == "]") and (tk[i + 1] == "<" or tk[i + 1] == "[")) {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().vector_level = cond_level - old_cond_level + 1;
						pattern->back().condition.back().command_type = "and";
						old_cond_level = cond_level;

					}
				}
				else if (is_body) {
					pattern->back().body.push_back({ 0,tk[i] });

				}
				if (tk[i] == "pattern") {
					is_body = false;
					is_cond = true;
					i++;
				}
				else if (tk[i] == "\n") {
					is_cond = false;
					is_body = true;
				}

			}
			else if (nested == -1 and tk[i] == "pend") {
				is_body = false;
				is_cond = false;

			}
			else if (tk[i] == "\n" and tk[i - 1] == "pend" and nested == -1 and is_body == false and is_cond == false) {
				//skip
			}
			else if (nested == -1 and is_body == false and is_cond == false) {
				output_token.push_back(tk[i]);
			}

		}


		auto desc = [](const Pattern& a, const Pattern& b) {return a.level > b.level;};

		std::stable_sort(lex_pattern.begin(), lex_pattern.end(), desc);
		std::stable_sort(ast_pattern.begin(), ast_pattern.end(), desc);
		std::stable_sort(tac_pattern.begin(), tac_pattern.end(), desc);
		std::stable_sort(hir_pattern.begin(), hir_pattern.end(), desc);
		std::stable_sort(lir_pattern.begin(), lir_pattern.end(), desc);
		std::stable_sort(asm_pattern.begin(), asm_pattern.end(), desc);
		return output_token;

	}
	namespace lex {


		void match(btree::Tree& tokens,int i_pattern) {

			unsigned int itoken = 0;
			unsigned int itoken_end = tokens.total;

			unsigned int icond = 0;

			bool ismatch = true;

			for (int i = 0; i < tokens.total; i++) {

				int start_replace = i;
				int end_replace = i;

				bool ismatch = true;
				for (int j = 0; j < lex_pattern[i_pattern].condition.size();j++) {
					if (btree::get(i).name == lex_pattern[i_pattern].condition[0].comand[j]) {
						end_replace++;
					}
					else {

						ismatch = false;
						break;
					}
				}
				if (ismatch) {
					int bsize = lex_pattern[i_pattern].body.size();
					btree::set(start_replace, end_replace, lex_pattern[i_pattern].body.data(), bsize);
					i = start_replace + bsize;
				}
			}
		}
	}

	namespace ast {
		void match(btree::Tree& tokens) {



		}
	}

}