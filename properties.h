#pragma once

namespace props {
	std::unordered_map<uint32_t, std::unordered_map<int, std::map<uint32_t, uint32_t>>> db;

	void set(uint32_t name, int id, uint32_t key, uint32_t val) {
		if (get_str(val) == "\"\"") {
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

	uint32_t get(uint32_t name, int id, uint32_t key) {
		auto it_name = db.find(name);
		if (it_name == db.end()) return ANY;

		auto it_id = it_name->second.find(id);
		if (it_id == it_name->second.end()) return ANY;

		auto it_key = it_id->second.find(key);
		if (it_key == it_id->second.end()) return ANY;

		return it_key->second;
	}
}