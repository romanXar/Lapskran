#pragma once

using namespace std;

vector<string> gen_words_array(string src) {

	// убрать BOM
	if (src.size() >= 3 &&
		(unsigned char)src[0] == 0xEF &&
		(unsigned char)src[1] == 0xBB &&
		(unsigned char)src[2] == 0xBF) {
		src.erase(0, 3);
	}

	vector<string> tokens;
	string token;

	for (size_t i = 0; i < src.size(); i++) {
		char c = src[i];

		if (c == '"') {
			if (!token.empty()) { tokens.push_back(token); token.clear(); }
			token += c;
			i++;
			while (i < src.size() && src[i] != '"') {
				token += src[i];
				i++;
			}
			if (i < src.size()) { token += src[i]; }
			tokens.push_back(token);
			token.clear();
			continue;
		}
		if (c == '\r') { continue; }

		if (c == ' ' || c == '\t' || c == '\n') {
			if (!token.empty()) { tokens.push_back(token); token.clear(); }

			tokens.push_back(string(1, c));
			continue;
		}

		if (
			c == '=' || c == '<' || c == '>' || c == '!' ||
			c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
			c == '&' || c == '|' || c == '^' || c == '~' || c == '?' ||
			c == ':' || c == '.' || c == ',' || c == '(' || c == ')' ||
			c == '{' || c == '}' || c == '[' || c == ']'
			)

		{
			if (!token.empty()) { tokens.push_back(token); token.clear(); }
			tokens.push_back(string(1, c));
			continue;
		}

		token += c;
	}

	if (!token.empty()) tokens.push_back(token);
	return tokens;
}