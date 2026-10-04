#pragma once

uint32_t next_id = ENUM_LEN;
std::unordered_map<std::string, uint32_t> str_to_id;
std::unordered_map<uint32_t, std::string> id_to_str;

uint32_t intern(const std::string& s) {
	if (str_to_id.empty()) {
		for (uint32_t i = 0; i < ENUM_LEN; i++) {
			str_to_id[id_table[i].text] = i;
			id_to_str[i] = id_table[i].text;
		}
	}
	if (s == "\\n") return NEWLINE;
	if (s == "\n") return NEWLINE;
	if (str_to_id.contains(s)) return str_to_id[s];
	uint32_t id = next_id++;

	bool is_int = false;
	int64_t num = 0;
	if (!s.empty()) {
		long long v;
		auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
		if (ec == std::errc() && ptr == s.data() + s.size()) {
			is_int = true;
			num = v;
		}
	}

	id_table[id] = { id, s, is_int, num };
	str_to_id[s] = id;
	id_to_str[id] = s;
	return id;
}

std::vector<uint32_t> intern_all(const std::vector<std::string>& src) {
	if (str_to_id.empty()) {
		for (uint32_t i = 0; i < ENUM_LEN; i++) {
			str_to_id[id_table[i].text] = i;
			id_to_str[i] = id_table[i].text;
		}
	}
	std::vector<uint32_t> out;
	out.reserve(src.size());
	for (int i = 0; i < (int)src.size(); i++) {
		out.push_back(intern(src[i]));
	}
	return out;
}

const std::string& get_str(uint32_t id) {
	return id_to_str[id];
}

uint32_t get_id(const std::string& s) {
	if (s == "\n") return NEWLINE;
	if (str_to_id.contains(s)) return str_to_id[s];
	return ANY;
}

inline bool is_int_sid(uint32_t id) {
	return id_table[id].is_int;
}

inline int64_t get_num(uint32_t id) {
	return id_table[id].num;
}