#pragma once

namespace mengine {

	enum CmdType {
		CMD_TOKWN = 1,
		CMD_TOK,
		CMD_REP,
		CMD_END_COND,
		CMD_AND,
		CMD_OR
	};

	struct Condition {
		int vector_level = 0;
		uint32_t command_type = 0;
		std::vector<uint32_t> comand;
	};

	struct Pattern {
		int level = 0;
		uint32_t stage = 0;
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

	void select_pattern(uint32_t name) {
		if (name == LEX) pattern = &lex_pattern;
		else if (name == AST) pattern = &ast_pattern;
		else if (name == TAC) pattern = &tac_pattern;
		else if (name == HIR) pattern = &hir_pattern;
		else if (name == LIR) pattern = &lir_pattern;
		else if (name == ASM) pattern = &asm_pattern;
		else exit(-10);
	}

	std::vector<uint32_t> init(std::vector<uint32_t>& tk) {

		std::vector<uint32_t> output_token;

		int nested = -1;
		int cond_level = 0;
		int old_cond_level = 0;

		bool is_cond = false;
		bool is_body = false;

		tk.push_back(0);
		tk.push_back(0);
		tk.push_back(0);

		for (int i = 0; i < tk.size(); i++) {
			if (is_int_sid(tk[i]) and tk[i + 1] == LEVEL and (tk[i + 2] == LEX or tk[i + 2] == AST or tk[i + 2] == TAC or tk[i + 2] == HIR or tk[i + 2] == LIR or tk[i + 2] == ASM)) {
				select_pattern(tk[i + 2]);
				Pattern p;
				p.level = (int)get_num(tk[i]);
				p.stage = tk[i + 2];
				pattern->push_back(p);
				cond_level = 0;
				old_cond_level = 0;

				nested++;
			}
			else if (tk[i] == PEND) { nested--; }

			if (nested >= 0) {

				if (is_cond) {
					if (tk[i] == LT) {
						cond_level++;
					}
					else if (tk[i] == GT) {
						cond_level--;
					}
					else if (tk[i] == LBRACK) {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().command_type = CMD_REP;
						cond_level++;
					}
					else if (tk[i] == RBRACK) {
						cond_level--;
					}
					else {
						if ((tk[i] != LT and tk[i] != LBRACK) and (tk[i - 1] == LT or tk[i - 1] == LBRACK) and tk[i] != OR) {
							if (tk[i + 1] == GT) {
								pattern->back().condition.push_back({});
								pattern->back().condition.back().vector_level = cond_level - old_cond_level - 1;
								pattern->back().condition.back().command_type = CMD_TOKWN;
								pattern->back().condition.back().comand.push_back(tk[i]);
								old_cond_level = cond_level;
							}
							else if (tk[i + 2] == GT) {
								pattern->back().condition.push_back({});
								pattern->back().condition.back().vector_level = cond_level - old_cond_level - 1;
								pattern->back().condition.back().command_type = CMD_TOK;
								pattern->back().condition.back().comand.push_back(tk[i]);
								pattern->back().condition.back().comand.push_back(tk[i + 1]);
								old_cond_level = cond_level;
								i++;
							}
						}
						else if (tk[i] == OR) {
							pattern->back().condition.push_back({});
							pattern->back().condition.back().vector_level = cond_level - old_cond_level + 1;
							pattern->back().condition.back().command_type = CMD_OR;
							old_cond_level = cond_level;
						}
					}

					if ((tk[i] == GT or tk[i] == RBRACK) and (tk[i + 1] == LT or tk[i + 1] == LBRACK)) {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().vector_level = cond_level - old_cond_level + 1;
						pattern->back().condition.back().command_type = CMD_AND;
						old_cond_level = cond_level;
					}
				}
				else if (is_body) {
					pattern->back().body.push_back({ 0, tk[i] });
				}
				if ((tk[i] == LEX or tk[i] == AST or tk[i] == TAC or tk[i] == HIR or tk[i] == LIR or tk[i] == ASM)) {
					is_body = false;
					is_cond = true;
				}
				else if (tk[i] == NEWLINE and is_cond) {
					pattern->back().condition.push_back({});
					pattern->back().condition.back().vector_level = -cond_level - 1;
					pattern->back().condition.back().command_type = CMD_END_COND;
					is_cond = false;
					is_body = true;
				}

			}
			else if (nested == -1 and tk[i] == PEND) {
				is_body = false;
				is_cond = false;

				for (int m = 0; m < 50; m++) {
					pattern->back().body.push_back({ 0, 0 });
				}
			}
			else if (tk[i] == NEWLINE and tk[i - 1] == PEND and nested == -1 and is_body == false and is_cond == false) {
				//skip
			}
			else if (nested == -1 and is_body == false and is_cond == false and tk[i] != 0) {
				output_token.push_back(tk[i]);
			}

		}

		auto desc = [](const Pattern& a, const Pattern& b) {return a.level > b.level; };

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
			uint32_t pat = va_arg(ap, uint32_t);
			if (pat == ANY) {
				pos++;
				continue;
			}
			if (body[pos].name != pat) {
				va_end(ap);
				ii = start;
				return false;
			}
			pos++;
		}

		va_end(ap);
		resize_ii = pos - start;
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

		void goto_label(const std::vector<btree::Token>& body, int& ii) {
			uint32_t jump_label = body[ii].name;

			for (ii = 0; ii < body.size() - 50; ii++) {
				if (body[ii].name == PLABEL and body[ii + 1].name == jump_label) {
					ii += 2;
					break;
				}
			}
		}

		void match(int i_pattern) {
			std::unordered_map<uint32_t, btree::Token> captured_tokens;

			for (int i = 0; i < btree::get_length(); i++) {

				int start_replace = i;
				int csize = lex_pattern[i_pattern].condition.size();

				struct Level {
					int mode_level = 0;
					int icond_start = 0;
					int icond = 0;
					int icond_return = 0;
					int iter_i = 0;
					bool result_match = true;
					int repeats = 0;
				};

				std::vector<Level> level;
				level.push_back({});

				int pending_repeats = 0;

				while (level.back().icond < csize) {

					int ci = level.back().icond;
					uint32_t type_command = lex_pattern[i_pattern].condition[ci].command_type;
					int vector_level = lex_pattern[i_pattern].condition[ci].vector_level;

					if (type_command == CMD_REP) {
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
								int mode = (pending_repeats > 0) ? 1 : 0;
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
						if (level.back().mode_level == 1 and level.back().result_match) {
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

					if (type_command == CMD_TOKWN or type_command == CMD_TOK) {
						uint32_t input_token = btree::get(i).name;
						uint32_t token_name = lex_pattern[i_pattern].condition[ci].comand[0];
						if (type_command == CMD_TOK) {
							uint32_t id_tok = lex_pattern[i_pattern].condition[ci].comand[1];
							captured_tokens[id_tok] = btree::get(i);
						}

						if (token_name == input_token or token_name == TOKEN or (token_name == wORD and input_token != NEWLINE)) {
							level.back().result_match = true;
							i++;
						}
						else {
							level.back().result_match = false;
							if (level.back().mode_level == 1) {
								i = level.back().iter_i;
							}
						}
						level.back().icond++;
					}
					else if (type_command == CMD_AND) {
						if (!level.back().result_match) {
							level.back().icond = csize - 1;
						}
						else {
							level.back().icond++;
						}
					}
					else if (type_command == CMD_OR) {
						if (level.back().result_match) {
							int k = ci + 1;
							while (k < csize) {
								uint32_t t = lex_pattern[i_pattern].condition[k].command_type;
								if (t == CMD_AND or t == CMD_END_COND) break;
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

				struct Macro_var {
					int64_t value = 0;
					std::string type = "";
				};

				std::unordered_map<uint32_t, Macro_var> macro_vars;

				if (level[0].result_match and i > start_replace) {

					btree::create_tree("buffer");
					btree::use("buffer");

					std::vector<btree::Token>& body = lex_pattern[i_pattern].body;

					for (int ii = 0; ii < body.size() - 50; ) {

						//<123>.asd = afefef
						if (is_concurrence(body, ii, 7, LT, ANY, GT, DOT, ANY, EQ, ANY)) {
							auto it = captured_tokens.find(body[ii + 1].name);
							if (it != captured_tokens.end()) {
								props::set(it->second.name, it->second.id, body[ii + 4].name, body[ii + 6].name);
							}
							ii += resize_ii;
						}
						//<123>.asd
						else if (is_concurrence(body, ii, 5, LT, ANY, GT, DOT, ANY)) {
							auto it = captured_tokens.find(body[ii + 1].name);
							if (it != captured_tokens.end()) {

								uint32_t val = props::get(it->second.name, it->second.id, body[ii + 4].name);
								if (val == ANY) {
									std::cout << "macro bastard: property '" << get_str(body[ii + 4].name) << "' not found on token <" << get_str(body[ii + 1].name) << ">" << std::endl;
									exit(-1);
								}
								btree::push_back({ btree::gen_id(), val });
							}
							ii += resize_ii;
						}
						//<123>
						else if (is_concurrence(body, ii, 3, LT, ANY, GT) and captured_tokens.contains(body[ii + 1].name)) {
							btree::push_back(captured_tokens[body[ii + 1].name]);
							ii += resize_ii;
						}

						// pint 0a = 10
						else if (body[ii].name == pINT) {
							if (!is_macro_var(get_str(body[ii + 1].name))) {
								std::cout << "macro bastard: 'pint' expects var name, got '"
									<< get_str(body[ii + 1].name) << "'" << std::endl;
								exit(-1);
							}

							if (macro_vars.contains(body[ii + 1].name)
								and macro_vars[body[ii + 1].name].type != ""
								and macro_vars[body[ii + 1].name].type != "pint") {
								std::cout << "macro bastard: var '" << get_str(body[ii + 1].name)
									<< "' already declared as '"
									<< macro_vars[body[ii + 1].name].type << "'" << std::endl;
								exit(-1);
							}

							macro_vars[body[ii + 1].name].type = "pint";
							ii += 1;
						}

						//0a = ...
						else if (is_macro_var(get_str(body[ii].name))) {
							if (is_concurrence(body, ii, 7, ANY, EQ, LT, ANY, GT, DOT, ANY)) {

								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}

								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' is not pint" << std::endl;
									exit(-1);
								}

								auto it = captured_tokens.find(body[ii + 3].name);
								if (it == captured_tokens.end()) {
									std::cout << "macro bastard: capture '" << get_str(body[ii + 3].name) << "' not found" << std::endl;
									exit(-1);
								}

								uint32_t val = props::get(it->second.name, it->second.id, body[ii + 6].name);
								if (val == ANY) {
									std::cout << "macro bastard: property '" << get_str(body[ii + 6].name) << "' not found on token <" << get_str(body[ii + 3].name) << ">" << std::endl;
									exit(-1);
								}

								if (!is_int_sid(val)) {
									std::cout << "macro bastard: property '" << get_str(body[ii + 6].name) << "' on token <" << get_str(body[ii + 3].name) << "> is not integer (value='" << get_str(val) << "')" << std::endl;
									exit(-1);
								}

								macro_vars[body[ii].name].value = get_num(val);
								ii += resize_ii;
							}
							else if (is_concurrence(body, ii, 5, ANY, EQ, LT, ANY, GT)) {

								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}

								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
										<< "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
									exit(-1);
								}

								auto it = captured_tokens.find(body[ii + 3].name);
								if (it == captured_tokens.end()) {
									std::cout << "macro bastard: capture '" << get_str(body[ii + 3].name) << "' not found" << std::endl;
									exit(-1);
								}

								if (!is_int_sid(it->second.name)) {
									std::cout << "macro bastard: token name '" << get_str(it->second.name)
										<< "' (captured as <" << get_str(body[ii + 3].name) << ">) is not integer" << std::endl;
									exit(-1);
								}

								macro_vars[body[ii].name].value = get_num(it->second.name);
								ii += resize_ii;
							}
							else if (is_concurrence(body, ii, 3, ANY, EQ, ANY)) {
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
										<< "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
									exit(-1);
								}

								if (is_macro_var(get_str(body[ii + 2].name))) {
									if (!macro_vars.contains(body[ii + 2].name)) {
										std::cout << "macro bastard: macro var '" << get_str(body[ii + 2].name) << "' not found" << std::endl;
										exit(-1);
									}
									if (macro_vars[body[ii + 2].name].type != "pint") {
										std::cout << "macro bastard: macro var '" << get_str(body[ii + 2].name)
											<< "' is not pint (type='" << macro_vars[body[ii + 2].name].type << "')" << std::endl;
										exit(-1);
									}
									macro_vars[body[ii].name].value = macro_vars[body[ii + 2].name].value;
									ii += resize_ii;
								}
								else if (is_int_sid(body[ii + 2].name)) {
									macro_vars[body[ii].name].value = get_num(body[ii + 2].name);
									ii += resize_ii;
								}
								else {
									std::cout << "macro bastard: macro var or token '" << get_str(body[ii + 2].name) << "' not int" << std::endl;
									exit(-1);
								}
							}
							else if (is_concurrence(body, ii, 4, ANY, PLUS, EQ, ANY)) {
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
										<< "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
									exit(-1);
								}
								if (!is_int_sid(body[ii + 3].name)) {
									std::cout << "macro bastard: token '" << get_str(body[ii + 3].name) << "' not int" << std::endl;
									exit(-1);
								}
								macro_vars[body[ii].name].value += get_num(body[ii + 3].name);
								ii += resize_ii;
							}
							else if (is_concurrence(body, ii, 4, ANY, MINUS, EQ, ANY)) {
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
										<< "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
									exit(-1);
								}
								if (!is_int_sid(body[ii + 3].name)) {
									std::cout << "macro bastard: token '" << get_str(body[ii + 3].name) << "' not int" << std::endl;
									exit(-1);
								}
								macro_vars[body[ii].name].value -= get_num(body[ii + 3].name);
								ii += resize_ii;
							}
							else {
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
										<< "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
									exit(-1);
								}
								btree::push_back({ btree::gen_id(), intern(std::to_string(macro_vars[body[ii].name].value)) });
								ii++;
							}
						}

						else if (body[ii].name == PIF) {
							if (is_concurrence(body, ii, 7, PIF, ANY, ANY, EQ, ANY, ELSE, ANY)) {
								int64_t a, b;
								if (is_int_sid(body[ii + 1].name)) a = get_num(body[ii + 1].name);
								else if (macro_vars.contains(body[ii + 1].name)) a = macro_vars[body[ii + 1].name].value;
								else a = 0;

								if (is_int_sid(body[ii + 4].name)) b = get_num(body[ii + 4].name);
								else if (macro_vars.contains(body[ii + 4].name)) b = macro_vars[body[ii + 4].name].value;
								else b = 0;

								uint32_t op_first = body[ii + 2].name;
								bool result = false;

								if (op_first == BANG) {
									if (a != b) { result = true; ii += resize_ii; }
								}
								else if (op_first == EQ) {
									if (a == b) { result = true; ii += resize_ii; }
								}
								else if (op_first == LT) {
									if (a <= b) { result = true; ii += resize_ii; }
								}
								else if (op_first == GT) {
									if (a >= b) { result = true; ii += resize_ii; }
								}
								if (!result) {
									goto_label(body, ii += 6);
								}
							}

							if (is_concurrence(body, ii, 6, PIF, ANY, ANY, ANY, ELSE, ANY)) {
								int64_t a, b;
								if (is_int_sid(body[ii + 1].name)) a = get_num(body[ii + 1].name);
								else if (macro_vars.contains(body[ii + 1].name)) a = macro_vars[body[ii + 1].name].value;
								else a = 0;

								if (is_int_sid(body[ii + 3].name)) b = get_num(body[ii + 3].name);
								else if (macro_vars.contains(body[ii + 3].name)) b = macro_vars[body[ii + 3].name].value;
								else b = 0;

								uint32_t op_first = body[ii + 2].name;
								bool result = false;

								if (op_first == LT) {
									if (a < b) { result = true; ii += resize_ii; }
								}
								else if (op_first == GT) {
									if (a > b) { result = true; ii += resize_ii; }
								}
								if (!result) {
									goto_label(body, ii += 5);
								}
							}
						}

						else if (body[ii].name == PLABEL) {
							ii += 2;
						}
						else if (body[ii].name == PGOTO) {
							goto_label(body, ii += 1);
						}
						else {
							if (body[ii].name != 0) {
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

	}

	namespace ast {
		void match(btree::Tree& tree) {

		}
	}

}