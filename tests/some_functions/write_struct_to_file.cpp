#include <windows.h>
#include <iostream>

using namespace std;

struct MyStruct {
    int intValue;
    char stringValue[20];
};

bool writeStructToFile(HANDLE hFile, MyStruct &data) {
    // 将文件指针移动到文件末尾
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0;
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_END)) {
        cout << "Error setting file pointer." << endl;
        CloseHandle(hFile);
        return 1;
    }

    // 将结构体写入文件
    DWORD bytesWritten;
    if (!WriteFile(hFile, &data, sizeof(data), &bytesWritten, NULL)) {
        cout << "Error writing to file." << endl;
        CloseHandle(hFile);
        return 1;
    }
    return true;
}

int main() {
    const char *filename = "example.txt";
    MyStruct data = {123, "Hello what man."};

    // 打开文件
    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Error opening file." << endl;
        return 1;
    }

    writeStructToFile(hFile, data);

    // 关闭文件句柄
    CloseHandle(hFile);

    cout << "Struct data written to file successfully." << endl;

    return 0;
}
