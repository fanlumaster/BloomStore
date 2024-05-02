#include <iostream>
#include <bitset>
#include <fstream>

class MyClass {
  private:
    std::bitset<8> myBitset;

  public:
    // 构造函数
    MyClass() {
        // 初始化 bitset
        myBitset.set(3); // 设置第四个位为 1
        myBitset.set(5); // 设置第六个位为 1
    }

    // 将 bitset 写入文件
    void writeToFile(const std::string &filename) {
        // 将 bitset 转换为字符串
        std::string bitsetString = myBitset.to_string();

        // 打开文件
        std::ofstream outputFile(filename, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Error opening file." << std::endl;
            return;
        }

        // 将字符串转换为 char*
        const char *charArray = bitsetString.c_str();

        // 将 char* 写入文件
        outputFile.write(charArray, bitsetString.size());

        // 关闭文件
        outputFile.close();

        std::cout << "Bitset written to file successfully." << std::endl;
    }
};

int main() {
    MyClass obj;
    obj.writeToFile("bitset.txt");

    return 0;
}
