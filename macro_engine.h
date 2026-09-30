#pragma once
#include <charconv>
#include <algorithm>
#include <map>

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
			if (is_integer(tk[i]) and tk[i + 1] == "level" and (tk[i + 2] == "lex" or tk[i + 2] == "ast" or tk[i + 2] == "tac" or tk[i + 2] == "hir" or tk[i + 2] == "lir" or tk[i + 2] == "asm")) {
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
						pattern->back().condition.back().command_type = "rep";
						cond_level++;
					}
					else if (tk[i] == "]") {
						//pattern->back().condition.push_back({});
						//pattern->back().condition.back().command_type = "end_rep";
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
				if ((tk[i] == "lex" or tk[i] == "ast" or tk[i] == "tac" or tk[i] == "hir" or tk[i] == "lir" or tk[i] == "asm")) {
					is_body = false;
					is_cond = true;

				}
				else if (tk[i] == "\n" and is_cond) {
					pattern->back().condition.push_back({});
					pattern->back().condition.back().vector_level = -cond_level - 1;
					pattern->back().condition.back().command_type = "end_cond";
					is_cond = false;
					is_body = true;
				}

			}
			else if (nested == -1 and tk[i] == "pend") {
				is_body = false;
				is_cond = false;

				for (int m = 0; m < 50;m++) {
					pattern->back().body.push_back({ 0,"" });
				}


			}
			else if (tk[i] == "\n" and tk[i - 1] == "pend" and nested == -1 and is_body == false and is_cond == false) {
				//skip
			}
			else if (nested == -1 and is_body == false and is_cond == false and !tk[i].empty()) {
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
	inline bool is_macro_var(const std::string& s) {
		if (s.size() < 2) return false;
		unsigned char f = s[0];
		unsigned char b = s.back();
		return f >= '0' && f <= '9'
			&& ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z'));
	}
	namespace lex {



		/////////////////

		std::unordered_map<std::string, btree::Token> captured_tokens;

		void match(int i_pattern) {

			for (int i = 0; i < btree::get_length(); i++) {

				int start_replace = i;
				int csize = lex_pattern[i_pattern].condition.size();

				struct Level {
					std::string mode_level = "default";
					int icond_start = 0;
					int icond = 0;
					int icond_return = 0;
					int iter_i = 0;
					bool result_match = true;
					int repeats = 0;
				};

				std::vector<Level> level;



				level.push_back({});// нулевой уровень 

				int pending_repeats = 0;





				while (level.back().icond < csize) {

					int ci = level.back().icond;
					std::string type_command = lex_pattern[i_pattern].condition[ci].command_type;
					int vector_level = lex_pattern[i_pattern].condition[ci].vector_level;

					if (type_command == "rep") {
						pending_repeats++;
						level.back().icond++;
						continue;
					}

					if (vector_level > 0) {
						bool inside = (level.size() > 1) and (ci == level.back().icond_start);
						if (!inside) {
							int parent_icond = ci;

							int ret = csize;
							int depth = 0;
							for (int k = ci + 1; k < csize; k++) {
								int k_vl = lex_pattern[i_pattern].condition[k].vector_level;
								if (k_vl > 0) {
									depth += k_vl;
								}
								else if (k_vl < 0) {
									if (depth == 0) {
										ret = k + 1;
										break;
									}
									else {
										depth += k_vl;
									}
								}
							}

							level.back().icond_return = ret;
							level.back().icond = ci + 1;

							int remaining = vector_level;
							while (remaining > 0) {
								std::string mode = (pending_repeats > 0) ? "repeat" : "default";
								Level nl;
								nl.mode_level = mode;
								nl.icond_start = parent_icond;
								nl.icond = parent_icond;
								nl.icond_return = ret;
								nl.iter_i = i;
								nl.result_match = true;
								nl.repeats = 0;
								level.push_back(nl);
								if (pending_repeats > 0) pending_repeats--;
								remaining--;
							}
							continue;
						}
					}

					if (vector_level < 0) {
						if (level.back().mode_level == "repeat" and level.back().result_match) {
							level.back().icond = level.back().icond_start;
							level.back().iter_i = i;
							level.back().repeats++;
							level.back().result_match = true;
							continue;
						}

						bool child_result = (level.back().repeats > 0) ? true : level.back().result_match;

						if (level.size() == 1) {
							level.back().result_match = child_result;
							level.back().icond = csize;
							continue;
						}

						int ret = level.back().icond_return;
						level.pop_back();
						level.back().result_match = child_result;
						level.back().icond = ret;
						continue;
					}

					if (type_command == "tokwn" or type_command == "tok") {
						std::string input_token = btree::get(i).name;
						std::string token_name = lex_pattern[i_pattern].condition[ci].comand[0];
						if (type_command == "tok") {
							std::string id_tok = lex_pattern[i_pattern].condition[ci].comand[1];


							captured_tokens[token_name] = btree::get(i);

						}

						if (token_name == "\\n") token_name = "\n";

						if (token_name == input_token or token_name == "token") {
							level.back().result_match = true;
							i++;
						}
						else {
							level.back().result_match = false;
							if (level.back().mode_level == "repeat") {
								i = level.back().iter_i;
							}
						}
						level.back().icond++;
					}
					else if (type_command == "and") {
						if (!level.back().result_match) {
							level.back().icond = csize - 1;
						}
						else {
							level.back().icond++;
						}
					}
					else if (type_command == "or") {
						if (level.back().result_match) {
							int k = ci + 1;
							while (k < csize) {
								std::string t = lex_pattern[i_pattern].condition[k].command_type;
								if (t == "and" or t == "end_cond") break;
								k++;
							}
							level.back().icond = k;
						}
						else {
							level.back().icond++;
						}
					}
					else {
						level.back().icond++;
					}
				}








				//боди позитивная обработка
				//pif a op b else l123 условие
				//<a> захват с удалением
				//[a 0abc] захват с возможностью обращения  макропеременная хранит в 0abc количество заматченых элементов
				//<a 0> захват с без возможностью обращения  
				//pgoto l46846 
				//plabel l121
				//0abc макропеременная 
				//

				struct Macro_var {
					int value = 0;
				};

				std::unordered_map<std::string, Macro_var> macro_vars;

				if (level[0].result_match and i > start_replace) {

					btree::create_tree("buffer");
					btree::use("buffer");

					std::vector<btree::Token>& body = lex_pattern[i_pattern].body;

					// подстановка 



					int pushed = 0;


					for (int ii = 0; ii < body.size() - 50; ) {
						//std::cout << body[ii].name << std::endl;
						if (body[ii].name == "<" and captured_tokens.contains(body[ii + 1].name) and body[ii + 3].name == ">") {
							btree::push_back(captured_tokens[body[ii + 1].name]);
							pushed++;
							ii += 4;

						}
						else if (is_macro_var(body[ii].name)) {

							if (body[ii + 1].name == "=") {
								macro_vars[body[ii].name].value = std::stoi(body[ii + 2].name);
								ii += 3;
							}
							else if (body[ii + 1].name == "+" and body[ii + 2].name == "=") {
								macro_vars[body[ii].name].value += std::stoi(body[ii + 3].name);
								ii += 4;

							}
							else if (body[ii + 1].name == "-" and body[ii + 2].name == "=") {
								macro_vars[body[ii].name].value -= std::stoi(body[ii + 3].name);
								ii += 4;

							}
							else {
								btree::Token token = { btree::gen_id(),std::to_string(macro_vars[body[ii].name].value) };
								btree::push_back(token);
								pushed++;
								ii++;
							}
						}
						//pif 0a == 1a else rep

						else if (body[ii].name == "pif") {
							if (body[ii + 2].name == "=" and body[ii + 3].name == "=") {

								if (macro_vars[body[ii + 1].name].value == macro_vars[body[ii + 4].name].value) {
									ii += 7;

								}
								else {
									std::string jump_label = body[ii + 6].name;

									for (ii = 0; ii < body.size() - 50; ii++) {
										if (body[ii].name == "plabel" and body[ii + 1].name == jump_label) {
											ii += 2;
											break;
										}

									}

								}




							}
							else if (body[ii + 2].name == "<" and body[ii + 3].name == "=") {

								if (macro_vars[body[ii + 1].name].value <= macro_vars[body[ii + 4].name].value) {
									ii += 7;

								}
								else {
									std::string jump_label = body[ii + 6].name;

									for (ii = 0; ii < body.size() - 50; ii++) {
										if (body[ii].name == "plabel" and body[ii + 1].name == jump_label) {
											ii += 2;
											break;
										}

									}

								}




							}



						}

						else if (body[ii].name == "plabel") {
							ii += 2;

						}
						else if (body[ii].name == "pgoto") {
							std::string jump_label = body[ii + 1].name;
							//std::cout << "Sd";
							for (ii = 0; ii < body.size() - 50; ii++) {
								if (body[ii].name == "plabel" and body[ii + 1].name == jump_label) {
									ii += 2;
									break;
								}

							}

						}
						else {
							if (body[ii].name != "") {
								btree::push_back(body[ii]);
								pushed++;
								ii++;

							}
						}

					}

					btree::move_in("main", start_replace, i);
					btree::use("main");

					i = start_replace + pushed - 1;
				}
				else {
					i = start_replace;
				}

			}
		}





		////////////////////////////




	}

	namespace ast {
		void match(btree::Tree& tokens) {



		}
	}

}