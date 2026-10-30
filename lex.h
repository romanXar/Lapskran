#pragma once

using namespace std;
vector<Word> gen_tokens_array(vector<Word> words) {

	vector<Word> tokens;

	for (size_t i = 0; i < words.size(); i++) {
		string w = words[i].word;

		if (w == " " || w == "\t" || w == "\r") continue;

		if (w == "\n") {
			if (!tokens.empty() && tokens.back().word != "\n") tokens.push_back(words[i]);
			continue;
		}
		
		tokens.push_back(words[i]);
	}

	return tokens;
}