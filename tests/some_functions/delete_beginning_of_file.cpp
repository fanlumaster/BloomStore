#include <windows.h>
#include <iostream>

bool removeFirstBytes(const char *inputFileName, int bytesToRemove) {
    HANDLE inputFile = CreateFile(inputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (inputFile == INVALID_HANDLE_VALUE) {
        /* std::cerr << "cannot open input file." << std::endl; */
        return false;
    }

    const char *tempFileName = "temp.txt"; // temp file name
    HANDLE tempFile = CreateFile(tempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        CloseHandle(inputFile);
        /* std::cerr << "cannot create temp file." << std::endl; */
        return false;
    }

    // locate to bytesToRemove
    LARGE_INTEGER li;
    li.QuadPart = bytesToRemove;
    SetFilePointerEx(inputFile, li, NULL, FILE_BEGIN);

    // read from input file, and write to temp file
    BYTE buffer[1024];
    DWORD bytesRead;
    while (ReadFile(inputFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        WriteFile(tempFile, buffer, bytesRead, NULL, NULL);
    }

    // close file handle
    CloseHandle(inputFile);
    CloseHandle(tempFile);

    // use content of temp file to overwrite the input file
    if (!MoveFileEx(tempFileName, inputFileName, MOVEFILE_REPLACE_EXISTING)) {
        /* std::cerr << "move file content error occurs." << std::endl; */
        return false;
    }

    /* std::cout << "input file has already benn overwritten." << std::endl; */
    return true;
}

int main() {
    const char *inputFileName = "example.txt"; // input file name

    removeFirstBytes(inputFileName, 8);

    return 0;
}
