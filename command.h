#pragma once
void print_help() {
	std::cout << "Lapskran helper:\n\n";
	std::cout << "Lapskran build <file_path>     compile a source file\n";
	std::cout << "Lapskran add_path              add compiler to PATH\n";
	std::cout << "Lapskran help                  show this help\n";
}


void add_path() {

	// путь к папке, где лежит сам .exe
	char exe[MAX_PATH];
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	std::string dir = exe;
	dir = dir.substr(0, dir.find_last_of("\\/"));

	// открыть HKCU\Environment
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
		std::cout << "error: cannot open registry\n";
		return;
	}

	// прочитать текущий User PATH
	char buf[32767] = { 0 };
	DWORD size = sizeof(buf);
	DWORD type = REG_EXPAND_SZ;
	LONG q = RegQueryValueExA(hKey, "Path", nullptr, &type, (LPBYTE)buf, &size);

	if (q == ERROR_FILE_NOT_FOUND) {
		buf[0] = 0;   // Path отсутствует — начинаем с пустой строки
	}
	else if (q != ERROR_SUCCESS) {
		std::cout << "error: cannot read PATH\n";
		RegCloseKey(hKey);
		return;
	}

	std::string current = buf;

	// уже есть — выходим
	if (current.find(dir) != std::string::npos) {
		std::cout << "already in PATH: " << dir << "\n";
		RegCloseKey(hKey);
		return;
	}

	// склеить и записать
	std::string updated = current.empty() ? dir : current + ";" + dir;

	LONG w = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE*)updated.c_str(), (DWORD)updated.size() + 1);
	RegCloseKey(hKey);

	if (w != ERROR_SUCCESS) {
		std::cout << "error: cannot write PATH\n";
		return;
	}

	// сообщить системе, что окружение изменилось
	SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, nullptr);

	std::cout << "added to PATH: " << dir << "\n";
	std::cout << "restart your terminal to apply\n";
}
