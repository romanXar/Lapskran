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

						int cond_len;
						if (tk[i + 3] == EQ) {
							cond_len = 4;
						}
						else {
							cond_len = 3;
						}

						uint32_t a_tok = tk[i + 1];

						uint32_t op_tok;
						if (cond_len == 4) {
							op_tok = merge_op(tk[i + 2], tk[i + 3]);
						}
						else {
							op_tok = tk[i + 2];
						}

						uint32_t b_tok;
						if (cond_len == 4) {
							b_tok = tk[i + 4];
						}
						else {
							b_tok = tk[i + 3];
						}

						pattern->back().body.push_back({ 0, PIF });
						pattern->back().body.push_back({ 0, a_tok });
						pattern->back().body.push_back({ 0, op_tok });
						pattern->back().body.push_back({ 0, b_tok });
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

						int cond_len;
						if (tk[i + 3] == EQ) {
							cond_len = 4;
						}
						else {
							cond_len = 3;
						}

						uint32_t a_tok = tk[i + 1];

						uint32_t op_tok;
						if (cond_len == 4) {
							op_tok = merge_op(tk[i + 2], tk[i + 3]);
						}
						else {
							op_tok = tk[i + 2];
						}

						uint32_t b_tok;
						if (cond_len == 4) {
							b_tok = tk[i + 4];
						}
						else {
							b_tok = tk[i + 3];
						}

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
		return f >= '0' && f <= '9' && ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z'));
	}

	struct Macro_var {
		int64_t value = 0;
		std::string type = "";
		std::string str_value = "";
	};

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
			std::unordered_map<uint32_t, Macro_var> macro_vars;

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

				if (result_match and i > start_replace) {

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

						if (body[ii].name == PNEWLINE) { btree::push_back({ btree::gen_id(), NEWLINE }); ii++; }
						//<123>.asd = afefef
						//<123>.asd = 0a
						else if (is_concurrence(body, ii, 7, LT, ANY, GT, DOT, ANY, EQ, ANY)) {

							uint32_t slot = body[ii + 1].name;

							if (macro_vars.contains(slot)) {

								btree::use("main");
								btree::Token t = btree::get((int)macro_vars[slot].value);
								btree::use("buffer");

								uint32_t val;

								if (macro_vars.contains(body[ii + 6].name)) {
									val = intern(std::to_string(macro_vars[body[ii + 6].name].value));
								}
								else {
									val = body[ii + 6].name;
								}

								props::set(t.name, t.id, body[ii + 4].name, val);
							}

							ii += resize_ii;
						}
						// <0> = 0k   pstring
						// <0> = ssrdhdgh   
						else if (is_concurrence(body, ii, 5, LT, ANY, GT, EQ, ANY) && macro_vars.contains(body[ii + 1].name))
						{
							uint32_t slot = body[ii + 1].name;

							if (macro_vars.contains(body[ii + 4].name)) {
								auto& mv = macro_vars[body[ii + 4].name];
								if (mv.type != "pstring") {
									std::cout << "macro bastard: rename requires pstring, got '" << mv.type << "'" << std::endl;
									exit(-1);
								}

								btree::use("main");
								btree::Token& t = btree::get((int)macro_vars[slot].value);
								t.name = intern(mv.str_value);
								btree::use("buffer");
							}
							else {
								btree::use("main");

								btree::Token& t = btree::get((int)macro_vars[slot].value);
								t.name = body[ii + 4].name;

								btree::use("buffer");
							}
							ii += resize_ii;
						}
						//<123>.asd
						else if (is_concurrence(body, ii, 5, LT, ANY, GT, DOT, ANY)) {
							uint32_t slot = body[ii + 1].name;

							if (macro_vars.contains(slot)) {

								btree::use("main");
								btree::Token t = btree::get((int)macro_vars[slot].value);
								btree::use("buffer");

								uint32_t val = props::get(t.name, t.id, body[ii + 4].name);
								if (val == NOT_FOUND) {
									std::cout << "macro bastard: property '" << get_str(body[ii + 4].name) << "' not found on token <" << get_str(slot) << ">" << std::endl;
									exit(-1);
								}
								btree::push_back({ btree::gen_id(), val });
							}
							ii += resize_ii;
						}

						//<123>
						else if (is_concurrence(body, ii, 3, LT, ANY, GT) and macro_vars.contains(body[ii + 1].name)) {
							btree::use("main");
							btree::Token t = btree::get((int)macro_vars[body[ii + 1].name].value);
							btree::use("buffer");

							btree::push_back(t);
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
								ii += 1;
							}
							else {
								ii += 2;
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
								ii += 1;
							}
							else {
								ii += 2;
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

								uint32_t slot = body[ii + 3].name;
								if (!macro_vars.contains(slot)) {
									std::cout << "macro bastard: capture '" << get_str(slot) << "' not found" << std::endl;
									exit(-1);
								}

								btree::use("main");
								btree::Token t = btree::get((int)macro_vars[slot].value);
								btree::use("buffer");

								uint32_t val = props::get(t.name, t.id, body[ii + 6].name);
								if (val == NOT_FOUND) {
									std::cout << "macro bastard: property '" << get_str(body[ii + 6].name) << "' not found on token <" << get_str(slot) << ">" << std::endl;
									exit(-1);
								}

								if (!is_int_sid(val)) {
									std::cout << "macro bastard: property '" << get_str(body[ii + 6].name) << "' on token <" << get_str(slot) << "> is not integer (value='" << get_str(val) << "')" << std::endl;
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

								uint32_t slot = body[ii + 3].name;
								if (!macro_vars.contains(slot)) {
									std::cout << "macro bastard: capture '" << get_str(slot) << "' not found" << std::endl;
									exit(-1);
								}

								btree::use("main");
								btree::Token t = btree::get((int)macro_vars[slot].value);
								btree::use("buffer");

								const std::string& src = get_str(t.name);

								if (macro_vars[body[ii].name].type == "pint") {
									if (!is_int_sid(t.name)) {
										std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' is pint, but loaded token '" << src << "' (captured as <" << get_str(slot) << ">) is not integer" << std::endl;
										exit(-1);
									}
									macro_vars[body[ii].name].value = get_num(t.name);
								}
								else if (macro_vars[body[ii].name].type == "pstring") {
									macro_vars[body[ii].name].str_value = src;
								}
								else {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' has unknown type '" << macro_vars[body[ii].name].type << "'" << std::endl;
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
											std::cout << "macro bastard: macro var '" << get_str(rhs) << "' is not pint (type='" << macro_vars[rhs].type << "')" << std::endl;
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
											std::cout << "macro bastard: macro var '" << get_str(rhs) << "' is not pstring (type='" << macro_vars[rhs].type << "')" << std::endl;
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
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' has unknown type '" << dst.type << "'" << std::endl;
									exit(-1);
								}
							}
							else if (is_concurrence(body, ii, 4, ANY, PLUS, EQ, ANY)) {
								if (!macro_vars.contains(body[ii].name)) {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' not found" << std::endl;
									exit(-1);
								}
								if (macro_vars[body[ii].name].type != "pint") {
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
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
									std::cout << "macro bastard: macro var '" << get_str(body[ii].name) << "' is not pint (type='" << macro_vars[body[ii].name].type << "')" << std::endl;
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

							if (is_concurrence(body, ii, 6, PIF, ANY, ANY, ANY, ELSE, ANY)) {

								int64_t a = 0, b = 0;
								bool a_is_int = false, b_is_int = false;
								std::string a_str, b_str;

								if (is_int_sid(body[ii + 1].name)) { a = get_num(body[ii + 1].name); a_is_int = true; a_str = get_str(body[ii + 1].name); }
								else if (macro_vars.contains(body[ii + 1].name) and macro_vars[body[ii + 1].name].type == "pint") { a = macro_vars[body[ii + 1].name].value; a_is_int = true; a_str = std::to_string(a); }
								else if (macro_vars.contains(body[ii + 1].name) and macro_vars[body[ii + 1].name].type == "pstring") { a_str = macro_vars[body[ii + 1].name].str_value; }
								else { a_str = get_str(body[ii + 1].name); }

								if (is_int_sid(body[ii + 3].name)) { b = get_num(body[ii + 3].name); b_is_int = true; b_str = get_str(body[ii + 3].name); }
								else if (macro_vars.contains(body[ii + 3].name) and macro_vars[body[ii + 3].name].type == "pint") { b = macro_vars[body[ii + 3].name].value; b_is_int = true; b_str = std::to_string(b); }
								else if (macro_vars.contains(body[ii + 3].name) and macro_vars[body[ii + 3].name].type == "pstring") { b_str = macro_vars[body[ii + 3].name].str_value; }
								else { b_str = get_str(body[ii + 3].name); }

								bool only_int = a_is_int and b_is_int;
								uint32_t op = body[ii + 2].name;
								bool result = false;

								if (only_int) {
									if (op == EQ_EQ) { if (a == b) result = true; }
									else if (op == BANG_EQ) { if (a != b) result = true; }
									else if (op == LT_EQ) { if (a <= b) result = true; }
									else if (op == GT_EQ) { if (a >= b) result = true; }
									else if (op == LT) { if (a < b) result = true; }
									else if (op == GT) { if (a > b) result = true; }
								}
								else {
									if (op == LT or op == GT or op == LT_EQ or op == GT_EQ) {
										std::cout << "macro bastard: cannot compare strings with '<', '>', '<=' or '>=' ('" << a_str << "' " << get_str(op) << " '" << b_str << "')" << std::endl;
										exit(-1);
									}
									else if (op == BANG_EQ) { if (a_str != b_str) result = true; }
									else if (op == EQ_EQ) { if (a_str == b_str) result = true; }
								}

								if (result) {
									ii += 6;
								}
								else {
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

}