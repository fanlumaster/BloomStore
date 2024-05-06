#include <iostream>
#include <bitset>
#include <fstream>

class MyClass {
  private:
    std::bitset<8> myBitset;

  public:
    MyClass() {
        myBitset.set(3);
        myBitset.set(5);
    }

    // write bitset to file
    void writeToFile(const std::string &filename) {
        // convert bitset to string
        std::string bitsetString = myBitset.to_string();

        std::ofstream outputFile(filename, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Error opening file." << std::endl;
            return;
        }

        const char *charArray = bitsetString.c_str();

        outputFile.write(charArray, bitsetString.size());

        outputFile.close();

        std::cout << "Bitset written to file successfully." << std::endl;
    }
};

int main() {
    MyClass obj;
    obj.writeToFile("bitset.txt");

    return 0;
}
