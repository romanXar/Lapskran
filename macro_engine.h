#pragma once

namespace mengine {

	enum CmdType {
		CMD_TOKWN = 1,
		CMD_TOK,
		CMD_END_COND,
		CMD_AND,
		CMD_OR
	};

	struct Condition {
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


		bool is_cond = false;
		bool is_body = false;

		struct BodyFrame {
			int      type = 0;
			uint32_t lbl1 = 0;
			uint32_t lbl2 = 0;
		};
		std::vector<BodyFrame> body_stack;

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

				nested++;
				body_stack.clear();
			}

			else if (tk[i] == PEND) { nested--; }

			if (nested >= 0) {

				if (is_cond) {


					if ((tk[i] != LT) and (tk[i - 1] == LT) and tk[i] != OR) {
						if (tk[i + 1] == GT) {
							pattern->back().condition.push_back({});
							pattern->back().condition.back().command_type = CMD_TOKWN;
							pattern->back().condition.back().comand.push_back(tk[i]);
						}
						else if (tk[i + 2] == GT) {
							pattern->back().condition.push_back({});
							pattern->back().condition.back().command_type = CMD_TOK;
							pattern->back().condition.back().comand.push_back(tk[i]);
							pattern->back().condition.back().comand.push_back(tk[i + 1]);
							i++;
						}
					}
					else if (tk[i] == OR) {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().command_type = CMD_OR;
					}


					if ((tk[i] == GT) and (tk[i + 1] == LT)) {
						pattern->back().condition.push_back({});
						pattern->back().condition.back().command_type = CMD_AND;
					}
				}
				else if (is_body) {

					if (tk[i] == PIF) {
						nested++;
						BodyFrame f;
						f.type = 1;
						f.lbl1 = next_id++;
						body_stack.push_back(f);

						int cond_len = 3;
						if (tk[i + 3] == EQ) {
							cond_len = 4;
						}

						pattern->back().body.push_back({ 0, PIF });
						for (int k = 1; k <= cond_len; k++) {
							pattern->back().body.push_back({ 0, tk[i + k] });
						}
						pattern->back().body.push_back({ 0, ELSE });
						pattern->back().body.push_back({ 0, f.lbl1 });

						i += cond_len;
					}
					else if (tk[i] == PWHILE) {
						nested++;
						BodyFrame f;
						f.type = 2;
						f.lbl1 = next_id++;
						f.lbl2 = next_id++;
						body_stack.push_back(f);

						int cond_len = 3;
						if (tk[i + 3] == EQ) {
							cond_len = 4;
						}

						pattern->back().body.push_back({ 0, PLABEL });
						pattern->back().body.push_back({ 0, f.lbl1 });
						pattern->back().body.push_back({ 0, PIF });
						for (int k = 1; k <= cond_len; k++) {
							pattern->back().body.push_back({ 0, tk[i + k] });
						}
						pattern->back().body.push_back({ 0, ELSE });
						pattern->back().body.push_back({ 0, f.lbl2 });

						i += cond_len;
					}
					else if (tk[i] == PEND and body_stack.size() > 1) {
						BodyFrame f = body_stack.back();
						body_stack.pop_back();

						if (f.type == 1) {
							pattern->back().body.push_back({ 0, PLABEL });
							pattern->back().body.push_back({ 0, f.lbl1 });
						}
						else if (f.type == 2) {
							pattern->back().body.push_back({ 0, PGOTO });
							pattern->back().body.push_back({ 0, f.lbl1 });
							pattern->back().body.push_back({ 0, PLABEL });
							pattern->back().body.push_back({ 0, f.lbl2 });
						}
					}
					else if (tk[i] != NEWLINE) {
						pattern->back().body.push_back({ 0, tk[i] });
					}
				}

				if (tk[i] == LEX or tk[i] == AST or tk[i] == TAC or tk[i] == HIR or tk[i] == LIR or tk[i] == ASM) {
					is_body = false;
					is_cond = true;
				}
				else if (tk[i] == NEWLINE and is_cond) {
					pattern->back().condition.push_back({});
					pattern->back().condition.back().command_type = CMD_END_COND;
					is_cond = false;
					is_body = true;

					body_stack.clear();
					BodyFrame root;
					root.type = 0;
					root.lbl1 = 0;
					root.lbl2 = 0;
					body_stack.push_back(root);
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

                bool result_match = true;
                int ci = 0;

                while (ci < csize) {
                    uint32_t type_command = lex_pattern[i_pattern].condition[ci].command_type;

                    if (type_command == CMD_TOKWN or type_command == CMD_TOK) {
                        uint32_t input_token = btree::get(i).name;
                        uint32_t token_name = lex_pattern[i_pattern].condition[ci].comand[0];
                        if (type_command == CMD_TOK) {
                            uint32_t id_tok = lex_pattern[i_pattern].condition[ci].comand[1];
                            captured_tokens[id_tok] = btree::get(i);
                        }

                        if (token_name == input_token or token_name == TOKEN or (token_name == wORD and input_token != NEWLINE)) {
                            result_match = true;
                            i++;
                        }
                        else {
                            result_match = false;
                        }
                        ci++;
                    }
                    else if (type_command == CMD_AND) {
                        if (!result_match) {
                            ci = csize;
                        }
                        else {
                            ci++;
                        }
                    }
                    else if (type_command == CMD_OR) {
                        if (result_match) {
                            int k = ci + 1;
                            while (k < csize) {
                                uint32_t t = lex_pattern[i_pattern].condition[k].command_type;
                                if (t == CMD_AND or t == CMD_END_COND) break;
                                k++;
                            }
                            ci = k;
                        }
                        else {
                            ci++;
                        }
                    }
                    else {
                        ci++;
                    }
                }

                struct Macro_var {
                    int64_t value = 0;
                    std::string type = "";
                    std::string str_value = "";
                };

                std::unordered_map<uint32_t, Macro_var> macro_vars;

                if (result_match and i > start_replace) {

                    btree::create_tree("buffer");
                    btree::use("buffer");

                    std::vector<btree::Token>& body = lex_pattern[i_pattern].body;
                    bool canceled = false;

                    for (int ii = 0; ii < body.size() - 50; ) {

                        if (body[ii].name == PCANCEL) { canceled = true; break; }
                        else if (body[ii].name == PNEWLINE) { btree::push_back({ btree::gen_id(), NEWLINE });ii++; }
                        //<123>.asd = afefef
                        //<123>.asd = 0a
                        else if (is_concurrence(body, ii, 7, LT, ANY, GT, DOT, ANY, EQ, ANY)) {
                            auto it = captured_tokens.find(body[ii + 1].name);

                            if (it != captured_tokens.end()) {
                                if (macro_vars.contains(body[ii + 6].name)) {
                                    props::set(it->second.name, it->second.id, body[ii + 4].name, intern(std::to_string(macro_vars[body[ii + 6].name].value)));
                                }
                                else {
                                    props::set(it->second.name, it->second.id, body[ii + 4].name, body[ii + 6].name);
                                }

                            }
                            ii += resize_ii;
                        }
                        // <0> = 0k   (rename captured token, только если 0k — объявленная pstring-макропеременная)
                        else if (is_concurrence(body, ii, 5, LT, ANY, GT, EQ, ANY)
                            && macro_vars.contains(body[ii + 4].name))
                        {
                            auto it = captured_tokens.find(body[ii + 1].name);
                            if (it != captured_tokens.end()) {
                                auto& mv = macro_vars[body[ii + 4].name];
                                if (mv.type != "pstring") {
                                    std::cout << "macro bastard: rename requires pstring, got '"
                                        << mv.type << "'" << std::endl;
                                    exit(-1);
                                }
                                it->second.name = intern(mv.str_value);
                            }
                            ii += resize_ii;
                        }
                        //<123>.asd
                        else if (is_concurrence(body, ii, 5, LT, ANY, GT, DOT, ANY)) {
                            auto it = captured_tokens.find(body[ii + 1].name);
                            if (it != captured_tokens.end()) {

                                uint32_t val = props::get(it->second.name, it->second.id, body[ii + 4].name);
                                if (val == NOT_FOUND) {
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
                        // pstring 0a
                            // pstring 0a = <0>
                        else if (body[ii].name == pSTRING) {
                            if (!is_macro_var(get_str(body[ii + 1].name))) {
                                std::cout << "macro bastard: 'pstring' expects var name, got '" << get_str(body[ii + 1].name) << "'" << std::endl;
                                exit(-1);
                            }

                            if (macro_vars.contains(body[ii + 1].name) and macro_vars[body[ii + 1].name].type != "" and macro_vars[body[ii + 1].name].type != "pstring") {
                                std::cout << "macro bastard: var '" << get_str(body[ii + 1].name) << "' already declared as '" << macro_vars[body[ii + 1].name].type << "'" << std::endl;
                                exit(-1);
                            }

                            macro_vars[body[ii + 1].name].type = "pstring";

                            if (body[ii + 2].name == EQ or body[ii + 2].name == PLUS or body[ii + 2].name == MINUS) {
                                ii += 1; // оставляем ii на имени — следующая итерация разберёт присваивание
                            }
                            else {
                                ii += 2; // чистое объявление — пропускаем и pSTRING, и имя
                            }
                        }
                        // pint 0a
                        // pint 0a = 10
                        else if (body[ii].name == pINT) {
                            if (!is_macro_var(get_str(body[ii + 1].name))) {
                                std::cout << "macro bastard: 'pint' expects var name, got '" << get_str(body[ii + 1].name) << "'" << std::endl;
                                exit(-1);
                            }

                            if (macro_vars.contains(body[ii + 1].name) and macro_vars[body[ii + 1].name].type != "" and macro_vars[body[ii + 1].name].type != "pint") {
                                std::cout << "macro bastard: var '" << get_str(body[ii + 1].name) << "' already declared as '" << macro_vars[body[ii + 1].name].type << "'" << std::endl;
                                exit(-1);
                            }

                            macro_vars[body[ii + 1].name].type = "pint";

                            if (body[ii + 2].name == EQ or body[ii + 2].name == PLUS or body[ii + 2].name == MINUS) {
                                ii += 1; // пусть присваивание разберётся на следующей итерации
                            }
                            else {
                                ii += 2; // объявление без инициализации
                            }
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
                                if (val == NOT_FOUND) {
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

                                auto it = captured_tokens.find(body[ii + 3].name);
                                if (it == captured_tokens.end()) {
                                    std::cout << "macro bastard: capture '" << get_str(body[ii + 3].name) << "' not found" << std::endl;
                                    exit(-1);
                                }

                                const std::string& src = get_str(it->second.name);

                                if (macro_vars[body[ii].name].type == "pint") {
                                    if (!is_int_sid(it->second.name)) {
                                        std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
                                            << "' is pint, but loaded token '" << src
                                            << "' (captured as <" << get_str(body[ii + 3].name) << ">) is not integer" << std::endl;
                                        exit(-1);
                                    }
                                    macro_vars[body[ii].name].value = get_num(it->second.name);
                                }
                                else if (macro_vars[body[ii].name].type == "pstring") {
                                    macro_vars[body[ii].name].str_value = src;
                                }
                                else {
                                    std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
                                        << "' has unknown type '" << macro_vars[body[ii].name].type << "'" << std::endl;
                                    exit(-1);
                                }

                                ii += resize_ii;
                            }
                            else if (is_concurrence(body, ii, 3, ANY, EQ, ANY)) {
                                if (!macro_vars.contains(body[ii].name)) {
                                    std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
                                    exit(-1);
                                }

                                auto& dst = macro_vars[body[ii].name];
                                uint32_t rhs = body[ii + 2].name;

                                if (dst.type == "pint") {
                                    if (is_macro_var(get_str(rhs))) {
                                        if (!macro_vars.contains(rhs)) {
                                            std::cout << "macro bastard: macro var '" << get_str(rhs) << "' not found" << std::endl;
                                            exit(-1);
                                        }
                                        if (macro_vars[rhs].type != "pint") {
                                            std::cout << "macro bastard: macro var '" << get_str(rhs)
                                                << "' is not pint (type='" << macro_vars[rhs].type << "')" << std::endl;
                                            exit(-1);
                                        }
                                        dst.value = macro_vars[rhs].value;
                                        ii += resize_ii;
                                    }
                                    else if (is_int_sid(rhs)) {
                                        dst.value = get_num(rhs);
                                        ii += resize_ii;
                                    }
                                    else {
                                        std::cout << "macro bastard: macro var or token '" << get_str(rhs) << "' not int" << std::endl;
                                        exit(-1);
                                    }
                                }
                                else if (dst.type == "pstring") {
                                    if (is_macro_var(get_str(rhs))) {
                                        if (!macro_vars.contains(rhs)) {
                                            std::cout << "macro bastard: macro var '" << get_str(rhs) << "' not found" << std::endl;
                                            exit(-1);
                                        }
                                        if (macro_vars[rhs].type != "pstring") {
                                            std::cout << "macro bastard: macro var '" << get_str(rhs)
                                                << "' is not pstring (type='" << macro_vars[rhs].type << "')" << std::endl;
                                            exit(-1);
                                        }
                                        dst.str_value = macro_vars[rhs].str_value;
                                        ii += resize_ii;
                                    }
                                    else {
                                        dst.str_value = get_str(rhs);
                                        ii += resize_ii;
                                    }
                                }
                                else {
                                    std::cout << "macro bastard: macro var '" << get_str(body[ii].name)
                                        << "' has unknown type '" << dst.type << "'" << std::endl;
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
                                if (macro_vars[body[ii].name].type == "pint") {
                                    btree::push_back({ btree::gen_id(), intern(std::to_string(macro_vars[body[ii].name].value)) });

                                }
                                else if (macro_vars[body[ii].name].type == "pstring") {
                                    btree::push_back({ btree::gen_id(), intern(macro_vars[body[ii].name].str_value) });

                                }
                                ii++;
                            }
                        }
                        //
                        else if (body[ii].name == PIF) {

                            if (is_concurrence(body, ii, 7, PIF, ANY, ANY, EQ, ANY, ELSE, ANY) or is_concurrence(body, ii, 6, PIF, ANY, ANY, ANY, ELSE, ANY)) {
                                int offset = (resize_ii == 7) ? 0 : -1; // 7-токен: b на ii+4, 6-токен: b на ii+3

                                int64_t a = 0, b = 0;
                                bool a_is_int = false, b_is_int = false;
                                std::string a_str, b_str;

                                if (is_int_sid(body[ii + 1].name)) { a = get_num(body[ii + 1].name); a_is_int = true; a_str = get_str(body[ii + 1].name); }
                                else if (macro_vars.contains(body[ii + 1].name) and macro_vars[body[ii + 1].name].type == "pint") { a = macro_vars[body[ii + 1].name].value; a_is_int = true; a_str = std::to_string(a); }
                                else if (macro_vars.contains(body[ii + 1].name) and macro_vars[body[ii + 1].name].type == "pstring") { a_str = macro_vars[body[ii + 1].name].str_value; }
                                else { a_str = get_str(body[ii + 1].name); }

                                if (is_int_sid(body[ii + 4 + offset].name)) { b = get_num(body[ii + 4 + offset].name); b_is_int = true; b_str = get_str(body[ii + 4 + offset].name); }
                                else if (macro_vars.contains(body[ii + 4 + offset].name) and macro_vars[body[ii + 4 + offset].name].type == "pint") { b = macro_vars[body[ii + 4 + offset].name].value; b_is_int = true; b_str = std::to_string(b); }
                                else if (macro_vars.contains(body[ii + 4 + offset].name) and macro_vars[body[ii + 4 + offset].name].type == "pstring") { b_str = macro_vars[body[ii + 4 + offset].name].str_value; }
                                else { b_str = get_str(body[ii + 4 + offset].name); }

                                bool only_int = a_is_int and b_is_int;

                                uint32_t op_first = body[ii + 2].name;
                                bool result = false;
                                if (only_int) {
                                    if (op_first == BANG) {
                                        if (a != b) { result = true; ii += resize_ii; }
                                    }
                                    else if (op_first == EQ) {
                                        if (a == b) { result = true; ii += resize_ii; }
                                    }
                                    else if (op_first == LT) {

                                        if (offset == 0 and a <= b) { result = true; ii += resize_ii; }
                                        else if (offset == -1 and a < b) { result = true; ii += resize_ii; }
                                    }
                                    else if (op_first == GT) {
                                        if (offset == 0 and a >= b) { result = true; ii += resize_ii; }
                                        else if (offset == -1 and a > b) { result = true; ii += resize_ii; }
                                    }
                                }
                                else {
                                    if (op_first == LT or op_first == GT) {
                                        std::cout << "macro bastard: cannot compare strings with '<=' or '>=' ('" << a_str << "' " << get_str(op_first) << " '" << b_str << "')" << std::endl;
                                        exit(-1);
                                    }
                                    else if (op_first == BANG) {
                                        if (a_str != b_str) { result = true; ii += resize_ii; }
                                    }
                                    else if (op_first == EQ) {
                                        if (a_str == b_str) { result = true; ii += resize_ii; }
                                    }
                                }
                                if (!result) {
                                    goto_label(body, ii += 6 + offset);
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

                    if (canceled) {
                        btree::use("main");
                        btree::destroy_tree("buffer");
                        i = start_replace;
                    }
                    else {
                        btree::move_in("main", start_replace, i);
                        btree::use("main");
                        i += btree::get_resize() - 1;
                    }
                }
                else {
                    i = start_replace;
                }

            }
        }

	}

}