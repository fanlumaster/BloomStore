#include "bloom_store.h"

Log::Log(std::string filename) {
    DBName = filename + ".db";
    if (_file_exists() != OK) {
        DeleteFile(DBName.c_str());
    }
    handle = CreateFile(DBName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

Log::~Log() { CloseHandle(handle); }

HANDLE Log::GetHandle() { return handle; }

RC Log::_file_exists() {
    DWORD fileAttr = GetFileAttributes(DBName.c_str());
    if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        return OK;
    } else {
        return FILE_NOT_EXISTS;
    }
}
