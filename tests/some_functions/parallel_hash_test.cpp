#include <iostream>
#include <vector>
/* #include <algorithm> */
#include <execution>
#include <string>

// 修改函数，使用额外参数
std::string modifyFunction(const std::string &str, int extraParameter, std::vector<std::string> &anotherVec) {
    // 在这个示例中，假设修改函数将额外参数添加到字符串末尾
    std::cout << extraParameter << "\n";
    return str + anotherVec[extraParameter];
}

int main() {
    std::vector<std::string> vec = {"A", "B", "C", "D", "E"};

    std::vector<std::string> anotherVec = {"f", "g", "h", "i", "j"};

    // 使用并行算法并行地对 vector 中的每个元素执行修改函数，依次传递参数 0, 1, 2, 3, 4
    int extraParameter = 0;
    std::transform(std::execution::par, vec.begin(), vec.end(), vec.begin(), [&extraParameter, &anotherVec](const std::string &str) { return modifyFunction(str, extraParameter++, anotherVec); });

    // 输出修改后的 vector
    for (const std::string &value : vec) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    return 0;
}
