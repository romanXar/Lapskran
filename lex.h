#pragma once

using namespace std;
vector<string> gen_tokens_array(vector<string> words) {

	vector<string> tokens;

	for (size_t i = 0; i < words.size(); i++) {
		string w = words[i];

		if (w == " " || w == "\t" || w == "\r") continue;

		if (w == "\n") {
			if (!tokens.empty() && tokens.back() != "\n") tokens.push_back(w);
			continue;
		}

		if (w.size() >= 1 && w.front() == '"') { tokens.push_back(w); continue; }

		// 0level 123level ... → 0, level
		size_t p = 0;
		while (p < w.size() && w[p] >= '0' && w[p] <= '9') p++;
		if (p > 0 && p < w.size() && w.substr(p) == "level") {
			tokens.push_back(w.substr(0, p));
			tokens.push_back("level");
			continue;
		}

		// int23 uint984 float45 → int, 23
		string types[3] = { "int", "uint", "float" };
		bool done = false;
		for (int k = 0; k < 3; k++) {
			string t = types[k];
			if (w.size() > t.size() && w.substr(0, t.size()) == t) {
				bool alldig = true;
				for (size_t j = t.size(); j < w.size(); j++) {
					if (w[j] < '0' || w[j] > '9') { alldig = false; break; }
				}
				if (alldig) {
					tokens.push_back(t);
					tokens.push_back(w.substr(t.size()));
					done = true;
					break;
				}
			}
		}
		if (done) continue;

		tokens.push_back(w);
	}

	return tokens;
}