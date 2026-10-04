#pragma once


namespace props {
	std::unordered_map<std::string,std::unordered_map<int,std::map<std::string, std::string>>> db;

    void set(const std::string& name, int id, const std::string& key, const std::string& val) {
        if (val == "\"\"") {
            auto it_name = db.find(name);
            if (it_name == db.end()) return;

            auto it_id = it_name->second.find(id);
            if (it_id == it_name->second.end()) return;

            it_id->second.erase(key);

            if (it_id->second.empty()) it_name->second.erase(it_id);
            if (it_name->second.empty()) db.erase(it_name);
            return;
        }

        db[name][id][key] = val;
    }

    std::string get(const std::string& name, int id, const std::string& key) {
        auto it_name = db.find(name);
        if (it_name == db.end()) return "";

        auto it_id = it_name->second.find(id);
        if (it_id == it_name->second.end()) return "";

        auto it_key = it_id->second.find(key);
        if (it_key == it_id->second.end()) return "";

        return it_key->second;
    }
}