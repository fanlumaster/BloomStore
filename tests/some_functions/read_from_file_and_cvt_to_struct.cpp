#include <windows.h>
#include <iostream>

using namespace std;

struct MyStruct {
    int intValue;
    char stringValue[20];
};

// read and build struct data with the offset in the file
bool readStructFromFile(HANDLE hFile, unsigned int offset, unsigned int size) {
    const int bufferSize = sizeof(MyStruct); // buffer size
    char buffer[bufferSize];

    // move file pointer
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + bufferSize * offset; // set the offset
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_BEGIN)) {
        cout << "Error setting file pointer." << endl;
        CloseHandle(hFile);
        return 1;
    }

    // read file content
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, bufferSize, &bytesRead, NULL)) {
        cout << "Error reading file." << endl;
        CloseHandle(hFile);
        return 1;
    }

    // convert bytes to struct
    MyStruct *myData = reinterpret_cast<MyStruct *>(buffer);

    // output the content of the struct
    cout << "intValue: " << myData->intValue << endl;
    cout << "stringValue: " << myData->stringValue << endl;

    return true;
}

int main() {
    const char *filename = "example.txt";

    // open file
    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Error opening file." << endl;
        return 1;
    }

    readStructFromFile(hFile, 2, 0);

    // close file handler
    CloseHandle(hFile);

    return 0;
}
