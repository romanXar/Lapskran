#pragma once

bool pattern_desc(const Pattern& a, const Pattern& b) {
	return a.level > b.level;
}

void select_pattern(uint32_t name) {
	pattern = &patterns[order[name]];
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

struct MacroFunction {
	uint32_t name;
	std::vector<btree::Token> body;
};

std::unordered_map<uint32_t, MacroFunction> functions; // Глобальный реестр


std::vector<uint32_t> init_pipeline(std::vector<uint32_t>& tk) {
	std::vector<uint32_t> output_token;

	bool block_pipeline = false;
	bool is_pipeline = false;

	for (int i = 0; i < (int)tk.size(); i++) {
		if (tk[i] == PIPELINE and !block_pipeline) {
			is_pipeline = true;
		}
		else if (tk[i] == PEND and is_pipeline and !block_pipeline) {
			is_pipeline = false;
			block_pipeline = true;

			if (i + 1 < (int)tk.size() && tk[i + 1] == NEWLINE) {
				i++;
			}
		}
		else if (is_pipeline) {
			if (!order.contains(tk[i]) and tk[i] != NEWLINE) {
				order[tk[i]] = patterns.size();
				patterns.push_back({});
			}

		}
		else if (!is_pipeline) {
			output_token.push_back(tk[i]);
		}
	}

	return output_token;
}

std::vector<uint32_t> init_funs(std::vector<uint32_t>& tk) {
	std::vector<uint32_t> output_token;
	int nested = -1;
	bool is_declare = false;
	bool is_body = false;

	struct BodyFrame {
		int      type = 0;
		uint32_t lbl1 = 0;
		uint32_t lbl2 = 0;
		bool     has_else = false;
	};

	std::vector<BodyFrame> body_stack;
	MacroFunction func;

	for (int sen = 0; sen < sentinel_before; sen++) {
		tk.push_back(0);
	}

	for (int i = 0; i < tk.size(); i++) {

		if (nested == -1) {
			if (tk[i] == PFUN) {
				func = MacroFunction();
				func.name = tk[i + 1];
				i++;

				nested++;
				is_declare = true;
				is_body = false;
			}
			else if (tk[i] != 0) {
				output_token.push_back(tk[i]);
			}
		}

		else if (nested >= 0) {
			if (is_declare) {
				if (tk[i] == NEWLINE) {
					is_declare = false;
					is_body = true;

					body_stack.clear();
					BodyFrame root;
					root.type = 0;
					body_stack.push_back(root);
				}
			}

			else if (is_body) {
				auto& body = func.body;

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

					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl1 });
					i += cond_len;
				}
				else if (tk[i] == PELIF) {
					BodyFrame& f = body_stack.back();
					body.push_back({ 0, PGOTO });
					body.push_back({ 0, f.lbl2 });
					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
					f.lbl1 = next_id++;

					int cond_len = (tk[i + 3] == EQ) ? 4 : 3;
					uint32_t a_tok = tk[i + 1];
					uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2], tk[i + 3]) : tk[i + 2];
					uint32_t b_tok = (cond_len == 4) ? tk[i + 4] : tk[i + 3];

					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl1 });
					i += cond_len;
				}
				else if (tk[i] == PELSE) {
					BodyFrame& f = body_stack.back();
					f.has_else = true;
					body.push_back({ 0, PGOTO });
					body.push_back({ 0, f.lbl2 });
					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
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

					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl2 });
					i += cond_len;
				}
				else if (tk[i] == PEND) {
					nested--;

					if (body_stack.size() > 1) {
						BodyFrame f = body_stack.back();
						body_stack.pop_back();
						if (f.type == 1) {
							if (!f.has_else) {
								body.push_back({ 0, PGOTO });
								body.push_back({ 0, f.lbl2 });
								body.push_back({ 0, PLABEL });
								body.push_back({ 0, f.lbl1 });
							}
							body.push_back({ 0, PLABEL });
							body.push_back({ 0, f.lbl2 });
						}
						else if (f.type == 2) {
							body.push_back({ 0, PGOTO });
							body.push_back({ 0, f.lbl1 });
							body.push_back({ 0, PLABEL });
							body.push_back({ 0, f.lbl2 });
						}
					}
					else {
						for (int m = 0; m < sentinel_after; m++) {
							body.push_back({ 0, 0 });
						}
						functions[func.name] = func;
						is_body = false;
					}
				}
				else if (tk[i] == PRET) {
					body.push_back({ 0, PRET });
				}
				else if (tk[i] == PPUSH || tk[i] == PRPUSH || tk[i] == PLOAD || tk[i] == PRLOAD) {
					body.push_back({ 0, tk[i] });
					body.push_back({ 0, tk[i + 1] });
					i += 1;
				}
				else if (tk[i] != NEWLINE) {
					if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT && tk[i + 3] == DOT) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1];
						uint32_t prop_name = tk[i + 4];
						props::set(var_name, uid, PROP_IS_PROP, prop_name);
						body.push_back({ uid, var_name });
						i += 4;
					}
					else if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT && tk[i + 3] == LBRACK && tk[i + 5] == RBRACK) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1];
						uint32_t index_name = tk[i + 4];
						props::set(var_name, uid, PROP_IS_ARRAY, index_name);
						body.push_back({ uid, var_name });
						i += 5;
					}
					else if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1];
						props::set(var_name, uid, PROP_IS_REF, 1);
						body.push_back({ uid, var_name });
						i += 2;
					}
					else if (is_macro_var_sid(tk[i]) && tk[i + 1] == LBRACK && tk[i + 3] == RBRACK) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i];
						uint32_t index_name = tk[i + 2];
						props::set(var_name, uid, PROP_IS_ARRAY, index_name);
						body.push_back({ uid, var_name });
						i += 3;
					}
					else if ((tk[i] == LT || tk[i] == GT || tk[i] == EQ || tk[i] == BANG) && tk[i + 1] == EQ) {
						uint32_t uid = next_id++;
						uint32_t merged_op = merge_op(tk[i], tk[i + 1]);
						body.push_back({ uid, merged_op });
						i += 1;
					}
					else if (is_macro_var_sid(tk[i]) and functions.contains(tk[i])) {
						body.push_back({ 0, tk[i] });
						i += 1;
					}
					else {
						body.push_back({ 0, tk[i] });
					}
				}
			}
		}
	}
	return output_token;
}

std::vector<uint32_t> init_patterns(std::vector<uint32_t>& tk) {

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

	for (int sen = 0; sen < sentinel_before; sen++) {
		tk.push_back(0);
	}

	for (int i = 0; i < tk.size(); i++) {

		if (order.contains(tk[i]) and tk[i + 1] == COLON and is_int_sid(tk[i + 2])) {
			select_pattern(tk[i]);
			Pattern p;
			p.level = (int)get_num(tk[i + 2]);
			p.stage = tk[i];
			pattern->push_back(p);

			nested++;
			body_stack.clear();

			is_cond = true; // <--- СРАЗУ ГОВОРИМ, ЧТО ДАЛЬШЕ ИДЕТ УСЛОВИЕ
			i += 2;         // <--- ПРОПУСКАЕМ ':' и '0' (цикл сам сделает i++ и попадет на '<')
		}
		else if (tk[i] == PEND) { nested--; }

		if (nested >= 0) {

			auto& cond = pattern->back().condition;
			auto& body = pattern->back().body;

			if (is_cond) {
				if ((tk[i] != LT) and (tk[i - 1] == LT) and tk[i] != OR) {
					if (tk[i + 1] == GT) {
						cond.push_back({});
						cond.back().command_type = CMD_TOKWN;
						cond.back().comand.push_back(tk[i]);
					}
					else if (tk[i + 2] == GT) {
						cond.push_back({});
						cond.back().command_type = CMD_TOK;
						cond.back().comand.push_back(tk[i]);
						cond.back().comand.push_back(tk[i + 1]);
						i++;
					}
				}
				else if (tk[i] == OR) {
					cond.push_back({});
					cond.back().command_type = CMD_OR;
				}

				if ((tk[i] == GT) and (tk[i + 1] == LT)) {
					cond.push_back({});
					cond.back().command_type = CMD_AND;
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

					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl1 });
					i += cond_len;
				}
				else if (tk[i] == PELIF) {
					BodyFrame& f = body_stack.back();
					body.push_back({ 0, PGOTO });
					body.push_back({ 0, f.lbl2 });
					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
					f.lbl1 = next_id++;

					int cond_len = (tk[i + 3] == EQ) ? 4 : 3;
					uint32_t a_tok = tk[i + 1];
					uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2], tk[i + 3]) : tk[i + 2];
					uint32_t b_tok = (cond_len == 4) ? tk[i + 4] : tk[i + 3];

					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl1 });
					i += cond_len;
				}
				else if (tk[i] == PELSE) {
					BodyFrame& f = body_stack.back();
					f.has_else = true;
					body.push_back({ 0, PGOTO });
					body.push_back({ 0, f.lbl2 });
					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
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

					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl2 });
					i += cond_len;
				}
				else if (tk[i] == PEND and body_stack.size() > 1) {
					BodyFrame f = body_stack.back();
					body_stack.pop_back();
					if (f.type == 1) {
						if (!f.has_else) {
							body.push_back({ 0, PGOTO });
							body.push_back({ 0, f.lbl2 });
							body.push_back({ 0, PLABEL });
							body.push_back({ 0, f.lbl1 });
						}
						body.push_back({ 0, PLABEL });
						body.push_back({ 0, f.lbl2 });
					}
					else if (f.type == 2) {
						body.push_back({ 0, PGOTO });
						body.push_back({ 0, f.lbl1 });
						body.push_back({ 0, PLABEL });
						body.push_back({ 0, f.lbl2 });
					}
				}
				else if (tk[i] != NEWLINE) {
					if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT && tk[i + 3] == DOT) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1];
						uint32_t prop_name = tk[i + 4];
						props::set(var_name, uid, PROP_IS_PROP, prop_name);
						body.push_back({ uid, var_name });
						i += 4;
					}
					else if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT && tk[i + 3] == LBRACK && tk[i + 5] == RBRACK) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1];
						uint32_t index_name = tk[i + 4];
						props::set(var_name, uid, PROP_IS_ARRAY, index_name);
						body.push_back({ uid, var_name });
						i += 5;
					}
					else if (tk[i] == LT && is_macro_var_sid(tk[i + 1]) && tk[i + 2] == GT) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1];
						props::set(var_name, uid, PROP_IS_REF, 1);
						body.push_back({ uid, var_name });
						i += 2;
					}
					else if (is_macro_var_sid(tk[i]) && tk[i + 1] == LBRACK && tk[i + 3] == RBRACK) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i];
						uint32_t index_name = tk[i + 2];
						props::set(var_name, uid, PROP_IS_ARRAY, index_name);
						body.push_back({ uid, var_name });
						i += 3;
					}
					else if ((tk[i] == LT || tk[i] == GT || tk[i] == EQ || tk[i] == BANG) && tk[i + 1] == EQ) {
						uint32_t uid = next_id++;
						uint32_t merged_op = merge_op(tk[i], tk[i + 1]);
						body.push_back({ uid, merged_op });
						i += 1;
					}
					else if (is_macro_var_sid(tk[i]) and functions.contains(tk[i])) {
						body.push_back({ 0, tk[i] });
						i += 1;
					}
					else {
						body.push_back({ 0, tk[i] });
					}
				}
			}


			if (tk[i] == NEWLINE and is_cond) {
				cond.push_back({});
				cond.back().command_type = CMD_END_COND;
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

			auto& body = pattern->back().body;

			for (int m = 0; m < sentinel_after; m++) {
				body.push_back({ 0, 0 });
			}
		}
		else if (tk[i] == NEWLINE and tk[i - 1] == PEND and nested == -1 and is_body == false and is_cond == false) {
			//skip
		}
		else if (nested == -1 and is_body == false and is_cond == false and tk[i] != 0) {
			output_token.push_back(tk[i]);
		}
	}

	for (auto& v : patterns)
		std::stable_sort(v.begin(), v.end(), pattern_desc);



	return output_token;
}