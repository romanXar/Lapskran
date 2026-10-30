#pragma once
using namespace std;


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
	std::vector<Token> body;
};

std::unordered_map<uint32_t, MacroFunction> functions;


vector<Token> init_pipeline(vector<Token> tk) {
	vector<Token> output_token;

	bool block_pipeline = false;
	bool is_pipeline = false;

	for (int i = 0; i < (int)tk.size(); i++) {
		if (tk[i].name == PIPELINE and !block_pipeline) {
			is_pipeline = true;
		}
		else if (tk[i].name == PEND and is_pipeline and !block_pipeline) {
			is_pipeline = false;
			block_pipeline = true;

			if (i + 1 < (int)tk.size() && tk[i + 1].name == NEWLINE) {
				i++;
			}
		}
		else if (is_pipeline) {
			if (!order.contains(tk[i].name) and tk[i].name != NEWLINE) {
				order[tk[i].name] = patterns.size();
				patterns.push_back({});
			}

		}
		else if (!is_pipeline) {
			output_token.push_back(tk[i]);
		}
	}

	return output_token;
}


vector<Token> init_funs(vector<Token> tk)
{
	vector<Token> output_token;

	std::vector<intptr_t> label;


	struct Fun {
		uint32_t return_type;
		uint32_t stack_size = 0;


		uint32_t pos_fun_alloc = 0;

		struct LocalVar {
			intptr_t type;
			intptr_t offset = 1;

		};

		vector<unordered_map<uint32_t, LocalVar > > lvar;

	};

	bool infun = false;
	uint32_t current_fun;
	unordered_map<uint32_t, Fun> fun;


	tk.push_back({ 0,0 });
	tk.push_back({ 0,0 });
	tk.push_back({ 0,0 });

	//обьявления
	for (int i = 0; i < tk.size() - 3;) {

		///грязный вонючий костыль чтобы свич просто работал
		if (!infun && tk[i].name != PFUN) {
			i++;
			continue;
		}

		switch (tk[i].name) {
		case PFUN:
		{
			i++;

			if (fun.try_emplace(tk[i].name).second) {
				infun = true;
				current_fun = tk[i].name;
				fun[current_fun].lvar.push_back({});
				bytecode.push_back({ ALLOC, 0, 0, 0 });

				fun[current_fun].pos_fun_alloc = bytecode.size()-1;

			}
			else {

				MACRO_CRUSH(tk[i], "function redefinition " + get_str(tk[i].name));


			}

			break;
		}
		case pINT:
		{
			i++;
			intptr_t l_var_offset;

			if (fun[current_fun].lvar.back().try_emplace(tk[i].name).second) {

				fun[current_fun].lvar.back()[tk[i].name].type = pINT;
				fun[current_fun].lvar.back()[tk[i].name].offset = fun[current_fun].stack_size++;
				l_var_offset = fun[current_fun].lvar.back()[tk[i].name].offset;
			
				i++;

				if (tk[i].name == EQ) {

					i++;

					//временно, 10 костанта из примера     pint 0a = 10

					bytecode.push_back({ MOV_II_VC, l_var_offset , 10, 0 });

					i++;

				}
	
			}
			else {
				MACRO_CRUSH(tk[i], "var redefinition " + get_str(tk[i].name));
			}
			

			break;
		}

		case PSHELL:
		{
			i++;

			bytecode.push_back({ PRINT, (intptr_t)pushcode.size() , 0, 0 });

			while (tk[i].name != NEWLINE) {
				pushcode.push_back({ VAR_I, fun[current_fun].lvar.back()[tk[i].name].offset , 0, 0 });
				i++;
			}

			pushcode.push_back({ eOF, 0 , 0, 0 });

			break;
		}
		case PEND:
		{
			bytecode[fun[current_fun].pos_fun_alloc].mem1 = fun[current_fun].stack_size;

			bytecode.push_back({ DEALLOC, fun[current_fun].stack_size , 0, 0 });

			i++;

			break;
		}
		default:
		{
			i++;
		}

		}


	}

	bytecode.push_back({ eOF, 0, 0, 0 });


	return output_token;
}

vector<Token> init_patterns(vector<Token> tk) {
	std::unordered_map<uint32_t, uint32_t> var_slots;

	vector<Token> output_token;
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
		tk.push_back({ 0,0 });
	}

	for (int i = 0; i < tk.size(); i++) {

		if (order.contains(tk[i].name) and tk[i + 1].name == COLON and is_int_sid(tk[i + 2].name)) {
			select_pattern(tk[i].name);
			Pattern p;
			p.level = (int)get_num(tk[i + 2].name);
			p.stage = tk[i].name;
			pattern->push_back(p);

			nested++;
			body_stack.clear();

			is_cond = true;
			i += 2;
		}
		else if (tk[i].name == PEND) { nested--; }

		if (nested >= 0) {

			auto& cond = pattern->back().condition;
			auto& body = pattern->back().body;

			if (is_cond) {
				if ((tk[i].name != LT) and (tk[i - 1].name == LT) and tk[i].name != OR) {
					if (tk[i + 1].name == GT) {
						cond.push_back({});
						cond.back().command_type = CMD_TOKWN;
						cond.back().comand.push_back(tk[i].name);
					}
					else if (tk[i + 2].name == GT) {
						cond.push_back({});
						cond.back().command_type = CMD_TOK;
						cond.back().comand.push_back(tk[i].name);
						cond.back().comand.push_back(tk[i + 1].name);
						i++;
					}
				}
				else if (tk[i].name == OR) {
					cond.push_back({});
					cond.back().command_type = CMD_OR;
				}

				if ((tk[i].name == GT) and (tk[i + 1].name == LT)) {
					cond.push_back({});
					cond.back().command_type = CMD_AND;
				}
			}
			else if (is_body) {
				if (tk[i].name == PIF) {
					nested++;
					BodyFrame f;
					f.type = 1;
					f.lbl1 = next_id++;
					f.lbl2 = next_id++;
					body_stack.push_back(f);

					int cond_len = (tk[i + 3].name == EQ) ? 4 : 3;
					uint32_t a_tok = tk[i + 1].name;
					uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2].name, tk[i + 3].name) : tk[i + 2].name;
					uint32_t b_tok = (cond_len == 4) ? tk[i + 4].name : tk[i + 3].name;

					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl1 });
					i += cond_len;
				}
				else if (tk[i].name == PELIF) {
					BodyFrame& f = body_stack.back();
					body.push_back({ 0, PGOTO });
					body.push_back({ 0, f.lbl2 });
					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
					f.lbl1 = next_id++;

					int cond_len = (tk[i + 3].name == EQ) ? 4 : 3;
					uint32_t a_tok = tk[i + 1].name;
					uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2].name, tk[i + 3].name) : tk[i + 2].name;
					uint32_t b_tok = (cond_len == 4) ? tk[i + 4].name : tk[i + 3].name;

					body.push_back({ 0, PIF });
					body.push_back({ 0, a_tok });
					body.push_back({ 0, op_tok });
					body.push_back({ 0, b_tok });
					body.push_back({ 0, ELSE });
					body.push_back({ 0, f.lbl1 });
					i += cond_len;
				}
				else if (tk[i].name == PELSE) {
					BodyFrame& f = body_stack.back();
					f.has_else = true;
					body.push_back({ 0, PGOTO });
					body.push_back({ 0, f.lbl2 });
					body.push_back({ 0, PLABEL });
					body.push_back({ 0, f.lbl1 });
				}
				else if (tk[i].name == PWHILE) {
					nested++;
					BodyFrame f;
					f.type = 2;
					f.lbl1 = next_id++;
					f.lbl2 = next_id++;
					body_stack.push_back(f);

					int cond_len = (tk[i + 3].name == EQ) ? 4 : 3;
					uint32_t a_tok = tk[i + 1].name;
					uint32_t op_tok = (cond_len == 4) ? merge_op(tk[i + 2].name, tk[i + 3].name) : tk[i + 2].name;
					uint32_t b_tok = (cond_len == 4) ? tk[i + 4].name : tk[i + 3].name;

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
				else if (tk[i].name == PEND and body_stack.size() > 1) {
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
				else if (tk[i].name != NEWLINE) {
					if (tk[i].name == LT && is_macro_var_sid(tk[i + 1].name) && tk[i + 2].name == GT && tk[i + 3].name == DOT) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1].name;
						uint32_t prop_name = tk[i + 4].name;
						props::set(var_name, uid, PROP_IS_PROP, prop_name);
						body.push_back({ uid, var_name });
						i += 4;
					}
					else if (tk[i].name == LT && is_macro_var_sid(tk[i + 1].name) && tk[i + 2].name == GT && tk[i + 3].name == LBRACK && tk[i + 5].name == RBRACK) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1].name;
						uint32_t index_name = tk[i + 4].name;
						props::set(var_name, uid, PROP_IS_ARRAY, index_name);
						body.push_back({ uid, var_name });
						i += 5;
					}
					else if (tk[i].name == LT && is_macro_var_sid(tk[i + 1].name) && tk[i + 2].name == GT) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i + 1].name;
						props::set(var_name, uid, PROP_IS_REF, 1);
						body.push_back({ uid, var_name });
						i += 2;
					}
					else if (is_macro_var_sid(tk[i].name) && tk[i + 1].name == LBRACK && tk[i + 3].name == RBRACK) {
						uint32_t uid = next_id++;
						uint32_t var_name = tk[i].name;
						uint32_t index_name = tk[i + 2].name;
						props::set(var_name, uid, PROP_IS_ARRAY, index_name);
						body.push_back({ uid, var_name });
						i += 3;
					}
					else if ((tk[i].name == LT || tk[i].name == GT || tk[i].name == EQ || tk[i].name == BANG) && tk[i + 1].name == EQ) {
						uint32_t uid = next_id++;
						uint32_t merged_op = merge_op(tk[i].name, tk[i + 1].name);
						body.push_back({ uid, merged_op });
						i += 1;
					}
					else if (is_macro_var_sid(tk[i].name) and functions.contains(tk[i].name)) {
						body.push_back({ 0, tk[i].name });
						i += 1;
					}
					else {
						uint32_t name = tk[i].name;
						uint32_t id_val = 0;

						if (is_macro_var_sid(name)) {
							if (!var_slots.contains(name)) {
								var_slots[name] = var_slots.size();
							}
							id_val = var_slots[name];
						}

						body.push_back({ id_val, name });
					}
				}
			}


			if (tk[i].name == NEWLINE and is_cond) {
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
		else if (nested == -1 and tk[i].name == PEND) {
			is_body = false;
			is_cond = false;

			auto& body = pattern->back().body;

			for (int m = 0; m < sentinel_after; m++) {
				body.push_back({ 0, 0 });
			}
		}
		else if (tk[i].name == NEWLINE and tk[i - 1].name == PEND and nested == -1 and is_body == false and is_cond == false) {

		}
		else if (nested == -1 and is_body == false and is_cond == false and tk[i].name != 0) {
			output_token.push_back(tk[i]);
		}
	}

	for (auto& v : patterns)
		std::stable_sort(v.begin(), v.end(), pattern_desc);



	return output_token;
}