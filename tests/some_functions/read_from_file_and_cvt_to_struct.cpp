#include <windows.h>
#include <iostream>

using namespace std;

struct MyStruct {
    int intValue;
    char stringValue[20];
};

// 读取第 offset 个结构体的数据
bool readStructFromFile(HANDLE hFile, unsigned int offset, unsigned int size) {
    const int bufferSize = sizeof(MyStruct); // 缓冲区大小
    char buffer[bufferSize];

    // 将文件指针移动到所需位置
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + bufferSize * offset; // 设置为你想要读取的起始位置
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_BEGIN)) {
        cout << "Error setting file pointer." << endl;
        CloseHandle(hFile);
        return 1;
    }

    // 读取文件内容
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, bufferSize, &bytesRead, NULL)) {
        cout << "Error reading file." << endl;
        CloseHandle(hFile);
        return 1;
    }

    // 将字节转换为结构体
    MyStruct *myData = reinterpret_cast<MyStruct *>(buffer);

    // 输出结构体的内容
    cout << "intValue: " << myData->intValue << endl;
    cout << "stringValue: " << myData->stringValue << endl;

    return true;
}

int main() {
    const char *filename = "example.txt";

    // 打开文件
    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Error opening file." << endl;
        return 1;
    }

    readStructFromFile(hFile, 2, 0);

    // 关闭文件句柄
    CloseHandle(hFile);

    return 0;
}
