#pragma once

struct StackFrame {
	const std::vector<Token>* body;
	int ibody = 0;

	std::unordered_map<uint32_t, Macro_var> macro_vars;
};

bool is_match(TStream& bmain,std::vector<Pattern>& pat,int i_pattern, std::unordered_map<uint32_t, Macro_var>& macro_vars, int& i) {
	bool result_match = true;
	int ci = 0;
	int csize = pat[i_pattern].condition.size();

	while (ci < csize) {
		uint32_t type_command = pat[i_pattern].condition[ci].command_type;
		if (type_command == CMD_TOKWN or type_command == CMD_TOK) {
			uint32_t input_token = bmain.get(i).name;
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

template<typename T, size_t CHUNK_SIZE = 16>
struct ChunkedStack {
	std::vector<std::unique_ptr<T[]>> chunks;
	size_t sp = 0;

	void push_back(const T& item) {
		size_t chunk_idx = sp / CHUNK_SIZE;
		if (chunk_idx >= chunks.size()) {
			chunks.push_back(std::make_unique<T[]>(CHUNK_SIZE));
		}
		chunks[chunk_idx][sp % CHUNK_SIZE] = item;
		sp++;
	}

	T& front() {
		if (sp == 0) {
			static T dummy{};
			return dummy;
		}
		return chunks[sp / CHUNK_SIZE][(sp - 1) % CHUNK_SIZE];
	}

	void pop_front() {
		if (sp > 0) sp--;
	}

	bool empty() const {
		return sp == 0;
	}

	void clear() {
		sp = 0; 
	}
};


void match(TStream& bmain,std::vector<Pattern>& pat,int i_pattern) {
	std::vector<StackFrame> frame;




	for (int i = 0; i < bmain.get_length(); i++) {

		int start_replace = i;
			

		frame.push_back({});

		frame.back().body = &pat[i_pattern].body;


		if (is_match(bmain, pat, i_pattern, frame.back().macro_vars, i) and i > start_replace) {
			//frame.back().macro_vars[get_id("0eof")].type = "pint";
			//frame.back().macro_vars[get_id("0eof")].value = bmain.get_length();

			TStream bbuffer;

			//frame.back().macro_vars[get_id("0startm")].type = "pint";
			//frame.back().macro_vars[get_id("0startm")].value = start_replace;

			//frame.back().macro_vars[get_id("0endm")].type = "pint";
			//frame.back().macro_vars[get_id("0endm")].value = i;

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


				else {
					bbuffer.push_back(body[pos]);
					pos++;
				}
			}

			//start_replace = frame[0].macro_vars[get_id("0startm")].value;
			//i = frame[0].macro_vars[get_id("0endm")].value;

			bbuffer.move_in(&bmain, start_replace, i);
			i += bmain.get_resize() - 1;
		}
		else {
			i = start_replace;
		}

		frame.clear();
	}
}

