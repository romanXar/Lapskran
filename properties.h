#pragma once


namespace props {
	std::unordered_map<std::string,std::unordered_map<int,std::map<std::string, std::string>>> db;

	void set(const std::string& name, int id,std::string& key, const std::string& val) {
		db[name][id][key] = val;
	}

	std::string get(const std::string& name, int id,
		const std::string& key) {
		return db[name][id][key];
	}
}