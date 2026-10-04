#pragma once


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
	int resize_ii = 0;
	bool is_concurrence(const std::vector<btree::Token>& body, int& ii, int count, ...) {
		va_list ap;
		va_start(ap, count);

		int start = ii;
		int pos = ii;

		for (int k = 0; k < count; k++) {
			const char* pattern = va_arg(ap, const char*);
			if (strcmp(pattern, "any") == 0) {
				pos++;
				continue;
			}
			if (body[pos].name != pattern) {
				va_end(ap);
				ii = start;
				return false;
			}
			pos++;
		}

		va_end(ap);
		//ii = pos;
		resize_ii = pos - start;
		;
		return true;
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


		void goto_label(const std::vector<btree::Token>& body, int& ii) {
			std::string jump_label = body[ii].name;

			for (ii = 0; ii < body.size() - 50; ii++) {
				if (body[ii].name == "plabel" and body[ii + 1].name == jump_label) {
					ii += 2;
					break;
				}
			}
		}

		void match(int i_pattern) {
			std::unordered_map<std::string, btree::Token> captured_tokens;

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


							captured_tokens[id_tok] = btree::get(i);

						}

						if (token_name == "\\n") token_name = "\n";

						if (token_name == input_token or token_name == "token" or (token_name == "word" and input_token != "\n")) {
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
					std::string value = "";
					std::string type = "";

				};

				std::unordered_map<std::string, Macro_var> macro_vars;

				if (level[0].result_match and i > start_replace) {

					btree::create_tree("buffer");
					btree::use("buffer");

					std::vector<btree::Token>& body = lex_pattern[i_pattern].body;



					for (int ii = 0; ii < body.size() - 50; ) {

						//------------------
						/**************/
						//<123>.asd = afefef
						if (is_concurrence(body, ii, 7, "<", "any", ">", ".", "any", "=", "any")) {
							auto it = captured_tokens.find(body[ii + 1].name);
							if (it != captured_tokens.end()) {
								props::set(it->second.name, it->second.id, body[ii + 4].name, body[ii + 6].name);
							}
							ii += resize_ii;
						}
						/**************/
						//<123>.asd
						else if (is_concurrence(body, ii, 5, "<", "any", ">", ".", "any")) {
							auto it = captured_tokens.find(body[ii + 1].name);
							if (it != captured_tokens.end()) {

								std::string val = props::get(it->second.name, it->second.id, body[ii + 4].name);
								if (val.empty()) {
									std::cout << "macro bastard: property '" << body[ii + 4].name << "' not found on token <" << body[ii + 1].name << ">" << std::endl;
									exit(-1);
								}
								btree::push_back({ btree::gen_id(), val });

							}
							ii += resize_ii;
						}
						/**************/
						//<123>
						else if (is_concurrence(body, ii, 3, "<", "any", ">") and captured_tokens.contains(body[ii + 1].name)) {
							btree::push_back(captured_tokens[body[ii + 1].name]);
							ii += resize_ii;
						}
						//------------------


						//------------------

						/*********/
						// pint 0a = 10
						// pint 0a = 0b
						// pint 0a = <123>.ggggg
						else if (body[ii].name == "pint") {
							if (!is_macro_var(body[ii + 1].name)) {
								std::cout << "macro bastard: 'pint' expects var name, got '"
									<< body[ii + 1].name << "'" << std::endl;
								exit(-1);
							}

							if (macro_vars.contains(body[ii + 1].name)
								and macro_vars[body[ii + 1].name].type != ""
								and macro_vars[body[ii + 1].name].type != "pint") {
								std::cout << "macro bastard: var '" << body[ii + 1].name
									<< "' already declared as '"
									<< macro_vars[body[ii + 1].name].type << "'" << std::endl;
								exit(-1);
							}

							macro_vars[body[ii + 1].name].type = "pint";
							ii += 1;
						}
						//------------------



						//------------------

						//0a = 0b
						//0a = 40
						//0a = <132>
						//0a = <132>.aadff
					
						else if (is_macro_var(body[ii].name)) {
							if (is_concurrence(body, ii, 7, "any", "=", "<", "any", ">", ".", "any")) {

								// 1. Макропеременная должна существовать
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << body[ii].name << "' not found" << std::endl;
									exit(-1);
								}

								// 2. И быть pint
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << body[ii].name	<< "' is not pint" << std::endl;
									exit(-1);
								}

								// 3. Захват должен существовать
								auto it = captured_tokens.find(body[ii + 3].name);
								if (it == captured_tokens.end()) {
									std::cout << "macro bastard: capture '" << body[ii + 3].name << "' not found" << std::endl;
									exit(-1);
								}

								// 4. Свойство должно существовать
								std::string val = props::get(it->second.name, it->second.id, body[ii + 6].name);
								if (val.empty()) {
									std::cout << "macro bastard: property '" << body[ii + 6].name << "' not found on token <" << body[ii + 3].name << ">"<< std::endl;
									exit(-1);
								}

								// 5. И быть числом
								if (!is_integer(val)) {
									std::cout << "macro bastard: property '" << body[ii + 6].name << "' on token <" << body[ii + 3].name << "> is not integer (value='" << val << "')"	<< std::endl;
									exit(-1);
								}

								// Всё ок — присвоить
								macro_vars[body[ii].name].value = val;
								ii += resize_ii;
							}
							else if (is_concurrence(body, ii, 5, "any", "=", "<", "any", ">")) {

								// 1. Макропеременная должна существовать
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << body[ii].name
										<< "' not found" << std::endl;
									exit(-1);
								}

								// 2. И быть pint
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << body[ii].name
										<< "' is not pint (type='" << macro_vars[body[ii].name].type
										<< "')" << std::endl;
									exit(-1);
								}

								// 3. Захват должен существовать
								auto it = captured_tokens.find(body[ii + 3].name);
								if (it == captured_tokens.end()) {
									std::cout << "macro bastard: capture '" << body[ii + 3].name
										<< "' not found" << std::endl;
									exit(-1);
								}

								// 4. Имя токена должно быть числом
								if (!is_integer(it->second.name)) {
									std::cout << "macro bastard: token name '" << it->second.name
										<< "' (captured as <" << body[ii + 3].name
										<< ">) is not integer" << std::endl;
									exit(-1);
								}

								// Всё ок — записать имя токена в макропеременную
								macro_vars[body[ii].name].value = it->second.name;
								ii += resize_ii;
							}
							else if (is_concurrence(body, ii, 3, "any", "=", "any")) {
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << body[ii].name << "' not found" << std::endl;
									exit(-1);

								}
								if (macro_vars[body[ii].name].type == "pint") {


									if (is_macro_var(body[ii + 2].name)) {
										if (!macro_vars.contains(body[ii + 2].name)) {
											std::cout << "macro bastard: macro var '" << body[ii + 2].name << "' not found" << std::endl;
											exit(-1);

										}
										else {

											macro_vars[body[ii].name].value = macro_vars[body[ii + 2].name].value;
											ii += resize_ii;
										}
									}
									else if (is_integer(body[ii + 2].name)) {

										macro_vars[body[ii].name].value = body[ii + 2].name;
										ii += resize_ii;


									}
									else {
										std::cout << "macro bastard: macro var or token '" << body[ii + 2].name << "' not int" << std::endl;
										exit(-1);


									}






								}








							}










							else if (is_concurrence(body, ii, 4, "any", "+", "=", "any")) {
								macro_vars[body[ii].name].value = std::to_string(std::stoi(macro_vars[body[ii].name].value) + std::stoi(body[ii + 3].name));
								ii += resize_ii;

							}
							else if (is_concurrence(body, ii, 4, "any", "-", "=", "any")) {
								macro_vars[body[ii].name].value = std::to_string(std::stoi(macro_vars[body[ii].name].value) - std::stoi(body[ii + 3].name));
								ii += resize_ii;
							}
							else {
								//std::cout << "true";

								btree::Token token = { btree::gen_id(),macro_vars[body[ii].name].value };
								btree::push_back(token);
								ii++;
							}
						}


						else if (body[ii].name == "pif") {
							if (is_concurrence(body, ii, 7, "pif", "any", "=", "=", "any", "else", "any")) {
								if (macro_vars[body[ii + 1].name].value == macro_vars[body[ii + 4].name].value) {
									ii += resize_ii;
								}
								else {
									goto_label(body, ii += 6);
								}
							}
							else if (is_concurrence(body, ii, 7, "pif", "any", "!", "=", "any", "else", "any")) {
								if (macro_vars[body[ii + 1].name].value != macro_vars[body[ii + 4].name].value) {
									ii += resize_ii;
								}
								else {
									goto_label(body, ii += 6);
								}
							}
							else if (is_concurrence(body, ii, 7, "pif", "any", "<", "=", "any", "else", "any")) {
								if (macro_vars[body[ii + 1].name].value <= macro_vars[body[ii + 4].name].value) {
									ii += resize_ii;
								}
								else {
									goto_label(body, ii += 6);
								}
							}
							else if (is_concurrence(body, ii, 7, "pif", "any", ">", "=", "any", "else", "any")) {
								if (macro_vars[body[ii + 1].name].value >= macro_vars[body[ii + 4].name].value) {
									ii += resize_ii;
								}
								else {
									goto_label(body, ii += 6);
								}
							}
							else if (is_concurrence(body, ii, 6, "pif", "any", "<", "any", "else", "any")) {
								if (macro_vars[body[ii + 1].name].value < macro_vars[body[ii + 3].name].value) {
									ii += resize_ii;

								}
								else {
									goto_label(body, ii += 5);
								}
							}
							else if (is_concurrence(body, ii, 6, "pif", "any", ">", "any", "else", "any")) {
								if (macro_vars[body[ii + 1].name].value > macro_vars[body[ii + 3].name].value) {
									ii += resize_ii;

								}
								else {
									goto_label(body, ii += 5);
								}
							}
						}
						else if (is_concurrence(body, ii, 6, "pif", "any", "<", "any", "else", "any")) {

							/////////
						}
						else if (body[ii].name == "plabel") {
							ii += 2;
						}
						else if (body[ii].name == "pgoto") {
							goto_label(body, ii += 1);
						}
						else {
							if (body[ii].name != "") {
								push_back(body[ii]);
								ii++;

							}
						}

					}

					btree::move_in("main", start_replace, i);
					btree::use("main");
					i += btree::get_resize() - 1;
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