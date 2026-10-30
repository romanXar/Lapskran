#pragma once
using namespace std;
vector<string> gen_words_array(string src);


	
//грязный вонючий костыль
struct Word {
	std::string word;
	std::string file_name;
	std::string line_in_file;

};
///


struct Frame {
	std::string text;
	size_t pos;
	std::string dir;
	std::string file;
	int line;
};

std::string dir_of(const std::string& path) {
	size_t p = path.find_last_of("\\/");
	if (p == std::string::npos) {
		return "";
	}
	return path.substr(0, p + 1);
}

std::string load_file(const std::string& p) {

	FILE* f = fopen(p.c_str(), "rb");
	if (f == nullptr) {
		printf("cannot open: %s\n", p.c_str());
		exit(1);
	}

	std::string data;
	char buf[4096];

	while (true) {
		size_t n = fread(buf, 1, sizeof(buf), f);
		if (n == 0) {
			break;
		}
		data.append(buf, n);
	}

	fclose(f);
	return data;
}

vector<Word> expand(const std::string& root) {

	std::set<std::string> visited;
	std::vector<Frame> stack;
	vector<Word> out;


	visited.insert(root);
	stack.push_back({ load_file(root), 0, dir_of(root), root,0});

	while (!stack.empty()) {

		Frame& top = stack.back();

		if (top.pos >= top.text.size()) {
			stack.pop_back();
			continue;
		}

		size_t nl = top.text.find('\n', top.pos);
		if (nl == std::string::npos) {
			nl = top.text.size();
		}

		std::string line = top.text.substr(top.pos, nl - top.pos);
		std::string file_name = top.file;
		top.line++;

		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		if (nl < top.text.size()) {
			top.pos = nl + 1;
		}
		else {
			top.pos = nl;
		}

		if (line.compare(0, 7, "import ") == 0) {

			std::string sub = line.substr(7);
			std::string full = top.dir + sub;

			if (!visited.count(full)) {
				visited.insert(full);
				stack.push_back({ load_file(full), 0, dir_of(full), full,0 });
			}
		}
		else {

			std::vector<std::string> words = gen_words_array(line);
			for (size_t w = 0; w < words.size(); w++) {

				out.push_back({words[w],file_name,to_string(top.line)});
				
			}
			out.push_back({ "\n",file_name,to_string(top.line) });
		
		}
	}

	return out;
}