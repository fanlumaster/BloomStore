#include <iostream>
#include <windows.h>

bool fileExists(const std::string& filename) {
    DWORD fileAttributes = GetFileAttributes(filename.c_str());
    return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

int main() {
    std::string filename = "example.txt";

    if (fileExists(filename)) {
        std::cout << "File exists." << std::endl;
    } else {
        std::cout << "File does not exist." << std::endl;
    }

    return 0;
}
