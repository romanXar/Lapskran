#pragma once

const int sentinel_before = 3;
const int sentinel_after = 50;

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

std::vector <std::vector<Pattern>> patterns;

std::unordered_map<int32_t,int32_t> order;






std::vector<Pattern>* pattern = nullptr;

enum PropKey {
	PROP_IS_REF = 1,     // <0a>
	PROP_IS_ARRAY,       // 0a[0i]
	PROP_IS_PROP         // <0a>.ssss
};

int resize_ii = 0;

struct Macro_var {
	int64_t value = 0;
	std::string type = "";        // "pint", "pstring", "array"
	std::string str_value = "";
	std::vector<uint32_t> array;  // Данные массива (интернированные ID)

	// Для жесткой синхронизации
	uint32_t size_var_id = 0;     // Для массива (ID переменной размера, например 0f)
	uint32_t linked_array_id = 0; // Для переменной размера (ID массива, например 0a)

	// НОВЫЕ ПОЛЯ ДЛЯ ССЫЛОК (ref)
	int ref_frame_idx = -1;       // Индекс фрейма, где лежит оригинальная переменная
	uint32_t ref_var_name = 0;    // Имя этой переменной
};