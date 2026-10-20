#pragma once

enum Reserved {
	NOT_FOUND,
	PFUN,
	PIF,
	PELIF,
	PELSE,
	PWHILE,
	pINT,
	pSTRING,
	PLABEL,
	PGOTO,
	PEND,

	LEVEL,
	LEX,
	AST,
	TAC,
	HIR,
	LIR,
	ASM,
	ELSE,
	OR,
	AND,
	REP,
	END_COND,
	TOK,
	TOKWN,
	NEWLINE,
	PNEWLINE,

	LT,
	GT,
	EQ,
	BANG,
	PLUS,
	MINUS,
	DOT,
	COMMA,
	LPAREN,
	RPAREN,
	LBRACK,
	RBRACK,

	REF,
	PCALL,
	PPUSH,
	PRPUSH,
	PLOAD,
	PRLOAD,
	PRET,

	PIPELINE,


	TOKEN,
	wORD,
	COLON,
	ANY,

	ENUM_LEN
};

struct InternEntry {
	uint32_t id;
	std::string text;
	bool is_int = false;
	int64_t num = 0;
	bool is_macro_var = false;
};

std::vector<InternEntry> id_table = {
	{ NOT_FOUND, "" },
	{ PFUN, "pfun" },
	{ PIF,       "pif" },
	{ PELIF,       "pelif" },
	{ PELSE,       "pelse" },
	{ PWHILE,    "pwhile" },
	{ pINT,      "pint" },
	{ pSTRING,   "pstring" },
	{ PLABEL,    "plabel" },
	{ PGOTO,     "pgoto" },
	{ PEND,      "pend" },
	{ LEVEL,     "level" },
	{ LEX,       "lex" },
	{ AST,       "ast" },
	{ TAC,       "tac" },
	{ HIR,       "hir" },
	{ LIR,       "lir" },
	{ ASM,       "asm" },
	{ ELSE,      "else" },
	{ OR,        "or" },
	{ AND,       "and" },
	{ REP,       "rep" },
	{ END_COND,  "end_cond" },
	{ TOK,       "tok" },
	{ TOKWN,     "tokwn" },
	{ NEWLINE,   "\n" },
	{ PNEWLINE,  "pnewline" },
	{ LT,        "<" },
	{ GT,        ">" },
	{ EQ,        "=" },
	{ BANG,      "!" },
	{ PLUS,      "+" },
	{ MINUS,     "-" },
	{ DOT,       "." },
	{ COMMA,     "," },
	{ LPAREN,    "(" },
	{ RPAREN,    ")" },
	{ LBRACK,    "[" },
	{ RBRACK,    "]" },
	{ REF,       "ref" },
	{ PCALL,     "pcall" },
	{ PPUSH,     "ppush" },
	{ PRPUSH,     "prpush" },
	{ PLOAD,     "pload" },
	{ PRLOAD,    "prload" },
	{ PRET,    "pret" },
	{ PIPELINE,    "Pipeline" },
	{ TOKEN,     "token" },
	{ wORD,      "word" },
	{ COLON,      ":" },


	{ ANY,       "any" },
};


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
		std::from_chars_result res = std::from_chars(s.data(), s.data() + s.size(), v);
		if (res.ec == std::errc() && res.ptr == s.data() + s.size()) {
			is_int = true;
			num = v;
		}
	}

	bool is_mv = false;
	if (s.size() >= 2) {
		unsigned char f = s[0];
		unsigned char b = s.back();
		if (f >= '0' && f <= '9' && ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z'))) {
			is_mv = true;
		}
	}

	id_table.push_back({ id, s, is_int, num, is_mv });
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
	return NOT_FOUND;
}

inline bool is_int_sid(uint32_t id) {
	return id_table[id].is_int;
}

inline int64_t get_num(uint32_t id) {
	return id_table[id].num;
}

inline bool is_macro_var_sid(uint32_t id) {
	return id_table[id].is_macro_var;
}