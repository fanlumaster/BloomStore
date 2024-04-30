#include "bloom_store.h"

BloomFilter::BloomFilter() : cellNum(BLOOM_FILTER_CELL_SIZE), bloomCell(std::vector<bool>(BLOOM_FILTER_CELL_SIZE, false)), hashFunctions() {
    // default hash functions that will be used
    hashFunctions.push_back(HashFuncs::RSHash);
    hashFunctions.push_back(HashFuncs::JSHash);
    hashFunctions.push_back(HashFuncs::PJWHash);
    hashFunctions.push_back(HashFuncs::ELFHash);
    hashFunctions.push_back(HashFuncs::BKDRHash);
    hashFunctions.push_back(HashFuncs::SDBMHash);
    hashFunctions.push_back(HashFuncs::DJBHash);
    hashFunctions.push_back(HashFuncs::DEKHash);
}

BloomFilter::BloomFilter(unsigned int num, std::vector<HashFunction> funcs) : cellNum(num), bloomCell(num), hashFunctions(funcs) {}

BloomFilter::~BloomFilter() {}

RC BloomFilter::InsertData(std::string str) {
    for (auto const &func : hashFunctions) {
        bloomCell[func(str) % cellNum] = true;
    }
    return OK;
}

bool BloomFilter::IsContain(std::string str) {
    for (auto const &func : hashFunctions) {
        if (bloomCell[func(str) % cellNum] == false) {
            return false;
        }
    }
    return true;
}
