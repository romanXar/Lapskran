#pragma once
struct Frame {
    std::string text;
    size_t pos;
    std::string dir;   // папка, где лежит этот файл
};

// отрезать имя файла, оставить папку вместе со слэшем
std::string dir_of(const std::string& path) {
    size_t p = path.find_last_of("\\/");
    return (p == std::string::npos) ? "" : path.substr(0, p + 1);
}

std::string expand(const std::string& root) {

    std::set<std::string> visited;
    std::vector<Frame> stack;
    std::string out;

    auto load = [](const std::string& p) -> std::string {
        std::ifstream f(p, std::ios::binary);
        if (!f) { std::cout << "cannot open: " << p << "\n"; exit(1); }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
        };

    visited.insert(root);
    stack.push_back({ load(root), 0, dir_of(root) });

    while (!stack.empty()) {

        Frame& top = stack.back();

        if (top.pos >= top.text.size()) {
            stack.pop_back();
            continue;
        }

        size_t nl = top.text.find('\n', top.pos);
        if (nl == std::string::npos) nl = top.text.size();

        std::string line = top.text.substr(top.pos, nl - top.pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        top.pos = (nl < top.text.size()) ? nl + 1 : nl;

        if (line.compare(0, 7, "import ") == 0) {

            std::string sub = line.substr(7);
            std::string full = top.dir + sub;   // относительно папки текущего файла

            if (!visited.count(full)) {
                visited.insert(full);
                stack.push_back({ load(full), 0, dir_of(full) });
            }
        }
        else {
            out += line;
            out += "\n";
        }
    }

    return out;
}