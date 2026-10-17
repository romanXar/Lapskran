#pragma once

namespace mengine {

	enum CmdType {
		CMD_TOKWN = 1,
		CMD_TOK,
		CMD_END_COND,
		CMD_AND,
		CMD_OR,
		EQ_EQ,
		BANG_EQ,
		LT_EQ,
		GT_EQ,
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

	enum PropKey {
		PROP_IS_REF = 1,     // <0a>
		PROP_IS_ARRAY,       // 0a[0i]
		PROP_IS_PROP         // <0a>.ssss
	};

	void select_pattern(uint32_t name) {
		if (name == LEX) pattern = &lex_pattern;
		else if (name == AST) pattern = &ast_pattern;
		else if (name == TAC) pattern = &tac_pattern;
		else if (name == HIR) pattern = &hir_pattern;
		else if (name == LIR) pattern = &lir_pattern;
		else if (name == ASM) pattern = &asm_pattern;
		else exit(-10);
	}

	uint32_t merge_op(uint32_t op1, uint32_t op2) {
		if (op2 == EQ) {
			if (op1 == EQ)   return EQ_EQ;
			if (op1 == BANG) return BANG_EQ;
			if (op1 == LT)   return LT_EQ;
			if (op1 == GT)   return GT_EQ;
		}
		if (op1 == EQ)   return EQ_EQ;
		if (op1 == BANG) return BANG_EQ;
		return op1;
	}

	bool pattern_desc(const Pattern& a, const Pattern& b) {
		return a.level > b.level;
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
			bool     has_else = false;
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
						f.lbl2 = next_id++;
						body_stack.push_back(f);

						int cond_len = (tk[i + 3] == EQ) ? 4 : 3;
						uint32_t a_tok = tk[i + 1];
						uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2], tk[i + 3]) : tk[i + 2];
						uint32_t b_tok = (cond_len == 4) ? tk[i + 4] : tk[i + 3];

						pattern->back().body.push_back({ 0, PIF });
						pattern->back().body.push_back({ 0, a_tok });
						pattern->back().body.push_back({ 0, op_tok });
						pattern->back().body.push_back({ 0, b_tok });
						pattern->back().body.push_back({ 0, ELSE });
						pattern->back().body.push_back({ 0, f.lbl1 });
						i += cond_len;
					}
					else if (tk[i] == PELIF) {
						BodyFrame& f = body_stack.back();
						pattern->back().body.push_back({ 0, PGOTO });
						pattern->back().body.push_back({ 0, f.lbl2 });
						pattern->back().body.push_back({ 0, PLABEL });
						pattern->back().body.push_back({ 0, f.lbl1 });
						f.lbl1 = next_id++;

						int cond_len = (tk[i + 3] == EQ) ? 4 : 3;
						uint32_t a_tok = tk[i + 1];
						uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2], tk[i + 3]) : tk[i + 2];
						uint32_t b_tok = (cond_len == 4) ? tk[i + 4] : tk[i + 3];

						pattern->back().body.push_back({ 0, PIF });
						pattern->back().body.push_back({ 0, a_tok });
						pattern->back().body.push_back({ 0, op_tok });
						pattern->back().body.push_back({ 0, b_tok });
						pattern->back().body.push_back({ 0, ELSE });
						pattern->back().body.push_back({ 0, f.lbl1 });
						i += cond_len;
					}
					else if (tk[i] == PELSE) {
						BodyFrame& f = body_stack.back();
						f.has_else = true;
						pattern->back().body.push_back({ 0, PGOTO });
						pattern->back().body.push_back({ 0, f.lbl2 });
						pattern->back().body.push_back({ 0, PLABEL });
						pattern->back().body.push_back({ 0, f.lbl1 });
					}
					else if (tk[i] == PWHILE) {
						nested++;
						BodyFrame f;
						f.type = 2;
						f.lbl1 = next_id++;
						f.lbl2 = next_id++;
						body_stack.push_back(f);

						int cond_len = (tk[i + 3] == EQ) ? 4 : 3;
						uint32_t a_tok = tk[i + 1];
						uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2], tk[i + 3]) : tk[i + 2];
						uint32_t b_tok = (cond_len == 4) ? tk[i + 4] : tk[i + 3];

						pattern->back().body.push_back({ 0, PLABEL });
						pattern->back().body.push_back({ 0, f.lbl1 });
						pattern->back().body.push_back({ 0, PIF });
						pattern->back().body.push_back({ 0, a_tok });
						pattern->back().body.push_back({ 0, op_tok });
						pattern->back().body.push_back({ 0, b_tok });
						pattern->back().body.push_back({ 0, ELSE });
						pattern->back().body.push_back({ 0, f.lbl2 });
						i += cond_len;
					}
					else if (tk[i] == PEND and body_stack.size() > 1) {
						BodyFrame f = body_stack.back();
						body_stack.pop_back();
						if (f.type == 1) {
							if (!f.has_else) {
								pattern->back().body.push_back({ 0, PGOTO });
								pattern->back().body.push_back({ 0, f.lbl2 });
								pattern->back().body.push_back({ 0, PLABEL });
								pattern->back().body.push_back({ 0, f.lbl1 });
							}
							pattern->back().body.push_back({ 0, PLABEL });
							pattern->back().body.push_back({ 0, f.lbl2 });
						}
						else if (f.type == 2) {
							pattern->back().body.push_back({ 0, PGOTO });
							pattern->back().body.push_back({ 0, f.lbl1 });
							pattern->back().body.push_back({ 0, PLABEL });
							pattern->back().body.push_back({ 0, f.lbl2 });
						}
					}
					else if (tk[i] != NEWLINE) {
						// 1. <0a>.ssss
						if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT && tk[i + 3] == DOT) {
							uint32_t uid = next_id++;
							uint32_t var_name = tk[i + 1];
							uint32_t prop_name = tk[i + 4];
							props::set(var_name, uid, PROP_IS_PROP, prop_name);
							pattern->back().body.push_back({ uid, var_name });
							i += 4;
						}
						// 2. <0a>[0i]
						else if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT && tk[i + 3] == LBRACK && tk[i + 5] == RBRACK) {
							uint32_t uid = next_id++;
							uint32_t var_name = tk[i + 1];
							uint32_t index_name = tk[i + 4];
							props::set(var_name, uid, PROP_IS_ARRAY, index_name);
							pattern->back().body.push_back({ uid, var_name });
							i += 5;
						}
						// 3. <0a>
						else if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT) {
							uint32_t uid = next_id++;
							uint32_t var_name = tk[i + 1];
							props::set(var_name, uid, PROP_IS_REF, 1);
							pattern->back().body.push_back({ uid, var_name });
							i += 2;
						}
						// 4. 0a[0i]
						else if (is_macro_var_sid(tk[i]) && tk[i + 1] == LBRACK && tk[i + 3] == RBRACK) {
							uint32_t uid = next_id++;
							uint32_t var_name = tk[i];
							uint32_t index_name = tk[i + 2];
							props::set(var_name, uid, PROP_IS_ARRAY, index_name);
							pattern->back().body.push_back({ uid, var_name });
							i += 3;
						}
						// 5. <=, >=, ==, !=
						else if ((tk[i] == LT || tk[i] == GT || tk[i] == EQ || tk[i] == BANG) && tk[i + 1] == EQ) {
							uint32_t uid = next_id++;
							uint32_t merged_op = merge_op(tk[i], tk[i + 1]);
							pattern->back().body.push_back({ uid, merged_op });
							i += 1;
						}
						// 6. Обычные токены
						else {
							pattern->back().body.push_back({ 0, tk[i] });
						}
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

		std::stable_sort(lex_pattern.begin(), lex_pattern.end(), pattern_desc);
		std::stable_sort(ast_pattern.begin(), ast_pattern.end(), pattern_desc);
		std::stable_sort(tac_pattern.begin(), tac_pattern.end(), pattern_desc);
		std::stable_sort(hir_pattern.begin(), hir_pattern.end(), pattern_desc);
		std::stable_sort(lir_pattern.begin(), lir_pattern.end(), pattern_desc);
		std::stable_sort(asm_pattern.begin(), asm_pattern.end(), pattern_desc);
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
			if (pat == ANY) { pos++; continue; }
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

	struct Macro_var {
		int64_t value = 0;
		std::string type = "";        // "pint", "pstring", "array"
		std::string str_value = "";
		std::vector<uint32_t> array;  // Данные массива (интернированные ID)

		// Для жесткой синхронизации
		uint32_t size_var_id = 0;     // Для массива (ID переменной размера, например 0f)
		uint32_t linked_array_id = 0; // Для переменной размера (ID массива, например 0a)
	};

	void goto_label(const std::vector<btree::Token>& body, int& ii) {
		uint32_t jump_label = body[ii].name;
		for (ii = 0; ii < body.size() - 50; ii++) {
			if (body[ii].name == PLABEL and body[ii + 1].name == jump_label) {
				ii += 2;
				break;
			}
		}
	}

	std::string get_macro_val(const btree::Token& tok, std::unordered_map<uint32_t, Macro_var>& macro_vars) {
		if (props::get(tok.name, tok.id, PROP_IS_REF) != NOT_FOUND) {
			btree::use("main");
			int pos = (int)macro_vars[tok.name].value;
			std::string res = get_str(btree::get(pos).name);
			btree::use("buffer");
			return res;
		}

		uint32_t idx_var = props::get(tok.name, tok.id, PROP_IS_ARRAY);
		if (idx_var != NOT_FOUND) {
			int i = is_int_sid(idx_var) ? (int)get_num(idx_var) : (int)macro_vars[idx_var].value;
			if (i < macro_vars[tok.name].array.size()) {
				return get_str(macro_vars[tok.name].array[i]);
			}
			return "0";
		}

		uint32_t prop_name = props::get(tok.name, tok.id, PROP_IS_PROP);
		if (prop_name != NOT_FOUND) {
			btree::use("main");
			int pos = (int)macro_vars[tok.name].value;
			btree::Token main_tok = btree::get(pos);
			btree::use("buffer");

			uint32_t prop_val = props::get(main_tok.name, main_tok.id, prop_name);
			if (prop_val != NOT_FOUND) {
				return get_str(prop_val);
			}
			return "";
		}

		if (macro_vars[tok.name].type == "pint") return std::to_string(macro_vars[tok.name].value);
		if (macro_vars[tok.name].type == "pstring") return macro_vars[tok.name].str_value;
		return "";
	}

	void set_macro_val(const btree::Token& tok, const std::string& val, std::unordered_map<uint32_t, Macro_var>& macro_vars) {
		if (props::get(tok.name, tok.id, PROP_IS_REF) != NOT_FOUND) {
			btree::use("main");
			int pos = (int)macro_vars[tok.name].value;
			btree::get(pos).name = intern(val);
			btree::use("buffer");
			return;
		}

		uint32_t idx_var = props::get(tok.name, tok.id, PROP_IS_ARRAY);
		if (idx_var != NOT_FOUND) {
			int i = is_int_sid(idx_var) ? (int)get_num(idx_var) : (int)macro_vars[idx_var].value;
			if (i >= macro_vars[tok.name].array.size()) {
				macro_vars[tok.name].array.resize(i + 1, intern("0"));
			}
			macro_vars[tok.name].array[i] = intern(val);
			uint32_t size_var = macro_vars[tok.name].size_var_id;
			if (size_var != 0) {
				macro_vars[size_var].value = macro_vars[tok.name].array.size();
			}
			return;
		}

		uint32_t prop_name = props::get(tok.name, tok.id, PROP_IS_PROP);
		if (prop_name != NOT_FOUND) {
			btree::use("main");
			int pos = (int)macro_vars[tok.name].value;
			btree::Token main_tok = btree::get(pos);
			props::set(main_tok.name, main_tok.id, prop_name, intern(val));
			btree::use("buffer");
			return;
		}

		uint32_t linked_arr = macro_vars[tok.name].linked_array_id;
		if (linked_arr != 0) {
			macro_vars[tok.name].value = std::stoll(val);
			macro_vars[linked_arr].array.resize(macro_vars[tok.name].value, intern("0"));
			return;
		}

		bool is_num = false;
		int64_t num = 0;
		if (!val.empty()) {
			auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), num);
			if (ec == std::errc() && ptr == val.data() + val.size()) {
				is_num = true;
			}
		}

		if (is_num) {
			macro_vars[tok.name].type = "pint";
			macro_vars[tok.name].value = num;
		}
		else {
			macro_vars[tok.name].type = "pstring";
			macro_vars[tok.name].str_value = val;
		}
	}

	void match(int i_pattern) {
		std::unordered_map<uint32_t, Macro_var> macro_vars;
		// Псевдокод внутри match:
		macro_vars[get_id("0eof")].type = "pint";
		macro_vars[get_id("0eof")].value = btree::get_length(); // Размер всего дерева
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
						macro_vars[id_tok].value = i;
						macro_vars[id_tok].type = "pint";
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
					if (!result_match) { ci = csize; }
					else { ci++; }
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
					else { ci++; }
				}
				else { ci++; }
			}

			if (result_match and i > start_replace) {

				intern("0eof");


				macro_vars[get_id("0eof")].type = "pint";
				macro_vars[get_id("0eof")].value = btree::get_length(); // Размер всего дерева

				btree::create_tree("buffer");
				btree::use("buffer");

				intern("0startm");
				intern("0endm");





				macro_vars[get_id("0startm")].type = "pint";
				macro_vars[get_id("0startm")].value = start_replace;

				macro_vars[get_id("0endm")].type = "pint";
				macro_vars[get_id("0endm")].value = i;

				std::vector<btree::Token>& body = lex_pattern[i_pattern].body;

				for (int ii = 0; ii < body.size() - 50; ) {

					if (is_concurrence(body, ii, 1, PNEWLINE)) {
						btree::push_back({ btree::gen_id(), NEWLINE });
						ii += resize_ii;
					}

					else if (is_macro_var_sid(body[ii].name)) {

						// 1. ПРИСВАИВАНИЕ (0a = ...)
						if (is_concurrence(body, ii, 3, ANY, EQ, ANY)) {
							btree::Token left_var = body[ii];
							btree::Token right_tok = body[ii + 2];

							std::string right_var = "";
							if (is_macro_var_sid(right_tok.name)) {
								right_var = get_macro_val(right_tok, macro_vars);
							}
							else {
								right_var = get_str(right_tok.name);
							}

							set_macro_val(left_var, right_var, macro_vars);
							ii += resize_ii;
						}

						// СЛОЖЕНИЕ И ВЫЧИТАНИЕ (0a += ... ИЛИ 0a -= ...)
						else if (is_concurrence(body, ii, 4, ANY, PLUS, EQ, ANY) || is_concurrence(body, ii, 4, ANY, MINUS, EQ, ANY)) {
							btree::Token left_var = body[ii];
							uint32_t op_type = body[ii + 1].name;
							btree::Token right_tok = body[ii + 3];

							std::string left_val = get_macro_val(left_var, macro_vars);
							std::string right_val = is_macro_var_sid(right_tok.name) ? get_macro_val(right_tok, macro_vars) : get_str(right_tok.name);

							bool l_is_num = false, r_is_num = false;
							int64_t l_num = 0, r_num = 0;

							if (!left_val.empty()) {
								auto [ptr, ec] = std::from_chars(left_val.data(), left_val.data() + left_val.size(), l_num);
								if (ec == std::errc() && ptr == left_val.data() + left_val.size()) l_is_num = true;
							}
							if (!right_val.empty()) {
								auto [ptr, ec] = std::from_chars(right_val.data(), right_val.data() + right_val.size(), r_num);
								if (ec == std::errc() && ptr == right_val.data() + right_val.size()) r_is_num = true;
							}

							if (l_is_num && r_is_num) {
								if (op_type == PLUS) set_macro_val(left_var, std::to_string(l_num + r_num), macro_vars);
								else set_macro_val(left_var, std::to_string(l_num - r_num), macro_vars);
							}
							else if (op_type == PLUS) {
								set_macro_val(left_var, left_val + right_val, macro_vars);
							}

							ii += resize_ii;
						}

						// 3. ОБЪЯВЛЕНИЕ ИЛИ ЧТЕНИЕ (0a, 0a[0f], 0a[40])
						else if (is_concurrence(body, ii, 1, ANY)) {
							btree::Token tok = body[ii];
							uint32_t idx_var = props::get(tok.name, tok.id, PROP_IS_ARRAY);

							if (idx_var != NOT_FOUND) {
								if (!is_int_sid(idx_var) && macro_vars[tok.name].type != "array") {
									macro_vars[tok.name].type = "array";
									macro_vars[tok.name].size_var_id = idx_var;

									macro_vars[idx_var].type = "pint";
									macro_vars[idx_var].value = 0;
									macro_vars[idx_var].linked_array_id = tok.name;
								}
								else {
									std::string val = get_macro_val(tok, macro_vars);
									btree::push_back({ btree::gen_id(), intern(val) });
								}
								ii += resize_ii;
							}
							else {
								// НЕ МАССИВ. Проверяем, это ссылка <0a> или обычная переменная
								if (props::get(tok.name, tok.id, PROP_IS_REF) != NOT_FOUND) {
									// <0a> - берем ОРИГИНАЛЬНЫЙ ТОКЕН из main, чтобы сохранить его ID и СВОЙСТВА
									btree::use("main");
									int pos = (int)macro_vars[tok.name].value;
									btree::Token main_tok = btree::get(pos);
									btree::use("buffer");
									btree::push_back(main_tok);
								}
								else {
									// Обычная переменная - достаем строку и создаем новый токен
									std::string val = get_macro_val(tok, macro_vars);

									// ИСПРАВЛЕНИЕ: Не пушим пустые токены!
									if (!val.empty()) {
										btree::push_back({ btree::gen_id(), intern(val) });
									}
								}
								ii += resize_ii;
							}
						}
						else {
							ii++;
						}
					}


					// ОБРАБОТКА PIF
					else if (is_concurrence(body, ii, 6, PIF, ANY, ANY, ANY, ELSE, ANY)) {
						btree::Token left_tok = body[ii + 1];
						uint32_t op = body[ii + 2].name;
						btree::Token right_tok = body[ii + 3];

						std::string left_val = is_macro_var_sid(left_tok.name) ? get_macro_val(left_tok, macro_vars) : get_str(left_tok.name);
						std::string right_val = is_macro_var_sid(right_tok.name) ? get_macro_val(right_tok, macro_vars) : get_str(right_tok.name);

						bool result = false;
						bool l_is_num = false, r_is_num = false;
						int64_t l_num = 0, r_num = 0;
						if (!left_val.empty()) {
							auto [ptr, ec] = std::from_chars(left_val.data(), left_val.data() + left_val.size(), l_num);
							if (ec == std::errc() && ptr == left_val.data() + left_val.size()) l_is_num = true;
						}
						if (!right_val.empty()) {
							auto [ptr, ec] = std::from_chars(right_val.data(), right_val.data() + right_val.size(), r_num);
							if (ec == std::errc() && ptr == right_val.data() + right_val.size()) r_is_num = true;
						}

						if (l_is_num && r_is_num) {
							if (op == EQ_EQ) result = (l_num == r_num);
							else if (op == BANG_EQ) result = (l_num != r_num);
							else if (op == LT) result = (l_num < r_num);
							else if (op == GT) result = (l_num > r_num);
							else if (op == LT_EQ) result = (l_num <= r_num);
							else if (op == GT_EQ) result = (l_num >= r_num);
						}
						else {
							if (op == EQ_EQ) result = (left_val == right_val);
							else if (op == BANG_EQ) result = (left_val != right_val);
						}

						if (result) {
							ii += 6;
						}
						else {
							goto_label(body, ii += 5);
						}
					}


					else if (is_concurrence(body, ii, 2, PLABEL, ANY)) {
						ii += resize_ii;
					}
					else if (is_concurrence(body, ii, 2, PGOTO, ANY)) {
						goto_label(body, ii += 1);
					}

					else {
						btree::push_back(body[ii]);
						ii++;
					}
				}

				start_replace = macro_vars[get_id("0startm")].value;
				i = macro_vars[get_id("0endm")].value;

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