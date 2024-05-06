#include <iostream>
#include <vector>
/* #include <algorithm> */
#include <execution>
#include <string>

// use extra params
std::string modifyFunction(const std::string &str, int extraParameter, std::vector<std::string> &anotherVec) {
    std::cout << extraParameter << "\n";
    return str + anotherVec[extraParameter];
}

int main() {
    std::vector<std::string> vec = {"A", "B", "C", "D", "E"};

    std::vector<std::string> anotherVec = {"f", "g", "h", "i", "j"};

    // change data in parallel
    int extraParameter = 0;
    std::transform(std::execution::par, vec.begin(), vec.end(), vec.begin(), [&extraParameter, &anotherVec](const std::string &str) { return modifyFunction(str, extraParameter++, anotherVec); });

    // output vector that has been changed
    for (const std::string &value : vec) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    return 0;
}
