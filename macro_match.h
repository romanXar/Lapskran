#pragma once

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

struct Arg {
	bool is_ref = false;
	int frame_idx = 0;
	uint32_t name = 0;
	Macro_var val;
};

struct StackFrame {
	const std::vector<btree::Token>* body;
	int ibody = 0;
	std::unordered_map<uint32_t, Macro_var> macro_vars;
};

std::string get_macro_val(const btree::Token& tok, std::vector<StackFrame>& frame, int f_idx) {
	// ЗАЩИТА ОТ ВЫЛЕТА: если фрейм уже удален, возвращаем пустоту
	if (f_idx < 0 || f_idx >= frame.size()) {
		return "";
	}

	Macro_var& mv = frame[f_idx].macro_vars[tok.name];

	// РАЗЫМЕНОВЫВАЕМ ССЫЛКУ
	if (mv.type == "ref") {
		btree::Token target;
		target.name = mv.ref_var_name;
		target.id = 0;
		return get_macro_val(target, frame, mv.ref_frame_idx);
	}

	if (props::get(tok.name, tok.id, PROP_IS_REF) != NOT_FOUND) {
		btree::use("main");
		int pos = (int)mv.value;
		std::string res = get_str(btree::get(pos).name);
		btree::use("buffer");
		return res;
	}

	uint32_t idx_var = props::get(tok.name, tok.id, PROP_IS_ARRAY);
	if (idx_var != NOT_FOUND) {
		int i = is_int_sid(idx_var) ? (int)get_num(idx_var) : (int)frame[f_idx].macro_vars[idx_var].value;
		if (i < mv.array.size()) {
			return get_str(mv.array[i]);
		}
		return "0";
	}

	uint32_t prop_name = props::get(tok.name, tok.id, PROP_IS_PROP);
	if (prop_name != NOT_FOUND) {
		btree::use("main");
		int pos = (int)mv.value;
		btree::Token main_tok = btree::get(pos);
		btree::use("buffer");

		uint32_t prop_val = props::get(main_tok.name, main_tok.id, prop_name);
		if (prop_val != NOT_FOUND) {
			return get_str(prop_val);
		}
		return "";
	}

	if (mv.type == "pint") return std::to_string(mv.value);
	if (mv.type == "pstring") return mv.str_value;
	return "";
}

void set_macro_val(const btree::Token& tok, const std::string& val, std::vector<StackFrame>& frame, int f_idx) {
	// ЗАЩИТА ОТ ВЫЛЕТА: если фрейм удален, ничего не пишем
	if (f_idx < 0 || f_idx >= frame.size()) {
		return;
	}

	Macro_var& mv = frame[f_idx].macro_vars[tok.name];

	// РАЗЫМЕНОВЫВАЕМ ССЫЛКУ
	if (mv.type == "ref") {
		btree::Token target;
		target.name = mv.ref_var_name;
		target.id = 0;
		set_macro_val(target, val, frame, mv.ref_frame_idx);
		return;
	}

	if (props::get(tok.name, tok.id, PROP_IS_REF) != NOT_FOUND) {
		btree::use("main");
		int pos = (int)mv.value;
		btree::get(pos).name = intern(val);
		btree::use("buffer");
		return;
	}

	uint32_t idx_var = props::get(tok.name, tok.id, PROP_IS_ARRAY);
	if (idx_var != NOT_FOUND) {
		int i = is_int_sid(idx_var) ? (int)get_num(idx_var) : (int)frame[f_idx].macro_vars[idx_var].value;
		if (i >= mv.array.size()) {
			mv.array.resize(i + 1, intern("0"));
		}
		mv.array[i] = intern(val);
		uint32_t size_var = mv.size_var_id;
		if (size_var != 0) {
			frame[f_idx].macro_vars[size_var].value = mv.array.size();
		}
		return;
	}

	uint32_t prop_name = props::get(tok.name, tok.id, PROP_IS_PROP);
	if (prop_name != NOT_FOUND) {
		btree::use("main");
		int pos = (int)mv.value;
		btree::Token main_tok = btree::get(pos);
		props::set(main_tok.name, main_tok.id, prop_name, intern(val));
		btree::use("buffer");
		return;
	}

	uint32_t linked_arr = mv.linked_array_id;
	if (linked_arr != 0) {
		mv.value = std::stoll(val);
		frame[f_idx].macro_vars[linked_arr].array.resize(mv.value, intern("0"));
		return;
	}

	bool is_num = false;
	int64_t num = 0;
	if (!val.empty()) {
		std::from_chars_result res = std::from_chars(val.data(), val.data() + val.size(), num);
		if (res.ec == std::errc() && res.ptr == val.data() + val.size()) {
			is_num = true;
		}
	}

	if (is_num) {
		mv.type = "pint";
		mv.value = num;
	}
	else {
		mv.type = "pstring";
		mv.str_value = val;
	}
}

void goto_label(const std::vector<btree::Token>& body, int& ii) {
	uint32_t jump_label = body[ii].name;
	for (ii = 0; ii < body.size() - sentinel_after; ii++) {
		if (body[ii].name == PLABEL and body[ii + 1].name == jump_label) {
			ii += 2;
			break;
		}
	}
}

bool is_match(std::vector<Pattern> pat,int i_pattern, std::unordered_map<uint32_t, Macro_var>& macro_vars, int& i) {
	bool result_match = true;
	int ci = 0;
	int csize = pat[i_pattern].condition.size();

	while (ci < csize) {
		uint32_t type_command = pat[i_pattern].condition[ci].command_type;
		if (type_command == CMD_TOKWN or type_command == CMD_TOK) {
			uint32_t input_token = btree::get(i).name;
			uint32_t token_name = pat[i_pattern].condition[ci].comand[0];
			if (type_command == CMD_TOK) {
				uint32_t id_tok = pat[i_pattern].condition[ci].comand[1];
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
					uint32_t t = pat[i_pattern].condition[k].command_type;
					if (t == CMD_AND or t == CMD_END_COND) break;
					k++;
				}
				ci = k;
			}
			else { ci++; }
		}
		else { ci++; }
	}

	return result_match;
}

void match(std::vector<Pattern>& pat,int i_pattern) {
	std::vector<StackFrame> frame;
	std::deque<Arg> arg_queue;

	for (int i = 0; i < btree::get_length(); i++) {

		int start_replace = i;

		frame.push_back({});
		frame.back().body = &pat[i_pattern].body;

		bool result_match = is_match(pat,i_pattern, frame.back().macro_vars, i);

		if (result_match and i > start_replace) {
			intern("0eof");
			frame.back().macro_vars[get_id("0eof")].type = "pint";
			frame.back().macro_vars[get_id("0eof")].value = btree::get_length();

			btree::create_tree("buffer");
			btree::use("buffer");

			intern("0startm");
			frame.back().macro_vars[get_id("0startm")].type = "pint";
			frame.back().macro_vars[get_id("0startm")].value = start_replace;

			intern("0endm");
			frame.back().macro_vars[get_id("0endm")].type = "pint";
			frame.back().macro_vars[get_id("0endm")].value = i;

			while (!frame.empty()) {
				auto& f = frame.back();
				auto& body = *f.body;
				int& pos = f.ibody;
				int f_idx = frame.size() - 1;

				if (pos >= body.size() - sentinel_after) {
					if (frame.size() > 1) {
						frame.pop_back();
						continue;
					}
					else {
						break;
					}
				}

				// PPUSH - Копирует ЗНАЧЕНИЕ
				if (body[pos].name == PPUSH) {
					uint32_t target = body[pos + 1].name;
					Arg a;
					a.is_ref = false;

					if (is_macro_var_sid(target)) {
						btree::Token src_tok;
						src_tok.name = target;
						src_tok.id = 0;
						std::string val = get_macro_val(src_tok, frame, f_idx);

						bool is_num = false;
						int64_t num = 0;
						if (!val.empty()) {
							std::from_chars_result res = std::from_chars(val.data(), val.data() + val.size(), num);
							if (res.ec == std::errc() && res.ptr == val.data() + val.size()) {
								is_num = true;
							}
						}

						if (is_num) {
							a.val.type = "pint";
							a.val.value = num;
						}
						else {
							a.val.type = "pstring";
							a.val.str_value = val;
						}
					}
					else {
						if (is_int_sid(target)) {
							a.val.type = "pint";
							a.val.value = get_num(target);
						}
						else {
							a.val.type = "pstring";
							a.val.str_value = get_str(target);
						}
					}
					arg_queue.push_back(a);
					pos += 2;
				}
				// PRPUSH - Копирует ССЫЛКУ
				else if (body[pos].name == PRPUSH) {
					uint32_t target = body[pos + 1].name;
					Arg a;
					if (is_macro_var_sid(target)) {
						a.is_ref = true;
						a.frame_idx = f_idx;
						a.name = target;
					}
					else {
						a.is_ref = false;
						if (is_int_sid(target)) {
							a.val.type = "pint";
							a.val.value = get_num(target);
						}
						else {
							a.val.type = "pstring";
							a.val.str_value = get_str(target);
						}
					}
					arg_queue.push_back(a);
					pos += 2;
				}
				// PLOAD - Извлекает ЗНАЧЕНИЕ
				else if (body[pos].name == PLOAD) {
					uint32_t target = body[pos + 1].name;
					Arg a = arg_queue.front();
					arg_queue.pop_front();

					if (a.is_ref) {
						btree::Token src_tok;
						src_tok.name = a.name;
						src_tok.id = 0;
						std::string val = get_macro_val(src_tok, frame, a.frame_idx);

						btree::Token dst_tok;
						dst_tok.name = target;
						dst_tok.id = 0;
						set_macro_val(dst_tok, val, frame, f_idx);
					}
					else {
						frame[f_idx].macro_vars[target] = a.val;
					}
					pos += 2;
				}
				// PRLOAD - Извлекает ССЫЛКУ
				else if (body[pos].name == PRLOAD) {
					uint32_t target = body[pos + 1].name;
					Arg a = arg_queue.front();
					arg_queue.pop_front();

					if (a.is_ref) {
						frame[f_idx].macro_vars[target].type = "ref";
						frame[f_idx].macro_vars[target].ref_frame_idx = a.frame_idx;
						frame[f_idx].macro_vars[target].ref_var_name = a.name;
					}
					else {
						frame[f_idx].macro_vars[target] = a.val;
					}
					pos += 2;
				}
				else if (is_concurrence(body, pos, 1, PNEWLINE)) {
					btree::push_back({ btree::gen_id(), NEWLINE });
					pos += resize_ii;
				}
				else if (is_macro_var_sid(body[pos].name) and !functions.contains(body[pos].name)) {
					// 1. ПРИСВАИВАНИЕ (0a = ...)
					if (is_concurrence(body, pos, 3, ANY, EQ, ANY)) {
						btree::Token left_var = body[pos];
						btree::Token right_tok = body[pos + 2];

						std::string right_var = "";
						if (is_macro_var_sid(right_tok.name)) {
							right_var = get_macro_val(right_tok, frame, f_idx);
						}
						else {
							right_var = get_str(right_tok.name);
						}

						set_macro_val(left_var, right_var, frame, f_idx);
						pos += resize_ii;
					}
					// СЛОЖЕНИЕ И ВЫЧИТАНИЕ
					else if (is_concurrence(body, pos, 4, ANY, PLUS, EQ, ANY) || is_concurrence(body, pos, 4, ANY, MINUS, EQ, ANY)) {
						btree::Token left_var = body[pos];
						uint32_t op_type = body[pos + 1].name;
						btree::Token right_tok = body[pos + 3];

						std::string left_val = get_macro_val(left_var, frame, f_idx);
						std::string right_val = "";
						if (is_macro_var_sid(right_tok.name)) {
							right_val = get_macro_val(right_tok, frame, f_idx);
						}
						else {
							right_val = get_str(right_tok.name);
						}

						bool l_is_num = false, r_is_num = false;
						int64_t l_num = 0, r_num = 0;

						if (!left_val.empty()) {
							std::from_chars_result res = std::from_chars(left_val.data(), left_val.data() + left_val.size(), l_num);
							if (res.ec == std::errc() && res.ptr == left_val.data() + left_val.size()) l_is_num = true;
						}
						if (!right_val.empty()) {
							std::from_chars_result res = std::from_chars(right_val.data(), right_val.data() + right_val.size(), r_num);
							if (res.ec == std::errc() && res.ptr == right_val.data() + right_val.size()) r_is_num = true;
						}

						if (l_is_num && r_is_num) {
							if (op_type == PLUS) set_macro_val(left_var, std::to_string(l_num + r_num), frame, f_idx);
							else set_macro_val(left_var, std::to_string(l_num - r_num), frame, f_idx);
						}
						else if (op_type == PLUS) {
							set_macro_val(left_var, left_val + right_val, frame, f_idx);
						}

						pos += resize_ii;
					}
					// 3. ОБЪЯВЛЕНИЕ ИЛИ ЧТЕНИЕ
					else if (is_concurrence(body, pos, 1, ANY)) {
						btree::Token tok = body[pos];
						uint32_t idx_var = props::get(tok.name, tok.id, PROP_IS_ARRAY);

						if (idx_var != NOT_FOUND) {
							if (!is_int_sid(idx_var) && frame[f_idx].macro_vars[tok.name].type != "array") {
								frame[f_idx].macro_vars[tok.name].type = "array";
								frame[f_idx].macro_vars[tok.name].size_var_id = idx_var;

								frame[f_idx].macro_vars[idx_var].type = "pint";
								frame[f_idx].macro_vars[idx_var].value = 0;
								frame[f_idx].macro_vars[idx_var].linked_array_id = tok.name;
							}
							else {
								std::string val = get_macro_val(tok, frame, f_idx);
								btree::push_back({ btree::gen_id(), intern(val) });
							}
							pos += resize_ii;
						}
						else {
							if (props::get(tok.name, tok.id, PROP_IS_REF) != NOT_FOUND) {
								btree::use("main");
								int pos_main = (int)frame[f_idx].macro_vars[tok.name].value;
								btree::Token main_tok = btree::get(pos_main);
								btree::use("buffer");
								btree::push_back(main_tok);
							}
							else {
								std::string val = get_macro_val(tok, frame, f_idx);
								if (!val.empty()) {
									btree::push_back({ btree::gen_id(), intern(val) });
								}
							}
							pos += resize_ii;
						}
					}
					else {
						pos++;
					}
				}

				// PIF
				else if (is_concurrence(body, pos, 6, PIF, ANY, ANY, ANY, ELSE, ANY)) {
					btree::Token left_tok = body[pos + 1];
					uint32_t op = body[pos + 2].name;
					btree::Token right_tok = body[pos + 3];

					std::string left_val = "";
					if (is_macro_var_sid(left_tok.name)) {
						left_val = get_macro_val(left_tok, frame, f_idx);
					}
					else {
						left_val = get_str(left_tok.name);
					}

					std::string right_val = "";
					if (is_macro_var_sid(right_tok.name)) {
						right_val = get_macro_val(right_tok, frame, f_idx);
					}
					else {
						right_val = get_str(right_tok.name);
					}

					bool result = false;
					bool l_is_num = false, r_is_num = false;
					int64_t l_num = 0, r_num = 0;

					if (!left_val.empty()) {
						std::from_chars_result res = std::from_chars(left_val.data(), left_val.data() + left_val.size(), l_num);
						if (res.ec == std::errc() && res.ptr == left_val.data() + left_val.size()) l_is_num = true;
					}
					if (!right_val.empty()) {
						std::from_chars_result res = std::from_chars(right_val.data(), right_val.data() + right_val.size(), r_num);
						if (res.ec == std::errc() && res.ptr == right_val.data() + right_val.size()) r_is_num = true;
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
						pos += 6;
					}
					else {
						goto_label(body, pos += 5);
					}
				}
				// PRET (Ранний выход из функции)
				else if (body[pos].name == PRET) {
					if (frame.size() > 1) {
						frame.pop_back(); // Просто выкидываем текущий фрейм функции
						continue;         // Возвращаемся в вызывающий код
					}
					else {
						break; // Если pret в паттерне — завершаем паттерн
					}
				}
				// ФУНКЦИИ (Вызов)
				else if (functions.contains(body[pos].name)) {
					StackFrame new_frame;
					new_frame.body = &functions[body[pos].name].body;

					pos++;
					frame.push_back(new_frame);
					continue;
				}

				else if (is_concurrence(body, pos, 2, PLABEL, ANY)) {
					pos += resize_ii;
				}
				else if (is_concurrence(body, pos, 2, PGOTO, ANY)) {
					goto_label(body, pos += 1);
				}

				else {
					btree::push_back(body[pos]);
					pos++;
				}
			}

			start_replace = frame[0].macro_vars[get_id("0startm")].value;
			i = frame[0].macro_vars[get_id("0endm")].value;

			btree::move_in("main", start_replace, i);
			btree::use("main");
			i += btree::get_resize() - 1;
		}
		else {
			i = start_replace;
		}

		frame.clear();
		arg_queue.clear();
	}
}