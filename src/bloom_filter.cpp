#include "bloom_store.h"

BloomFilter::BloomFilter() : cellNum(BLOOM_CELL_NUM), hashFunctions() {
    bloomCell.reset();
    // default hash functions that will be used
    hashFunctions.push_back(HashFuncs::RSHash);
    hashFunctions.push_back(HashFuncs::JSHash);
    hashFunctions.push_back(HashFuncs::PJWHash);
    // hashFunctions.push_back(HashFuncs::ELFHash); // in most cases, ELF hash is the same as PJW hash
    hashFunctions.push_back(HashFuncs::BKDRHash);
    hashFunctions.push_back(HashFuncs::SDBMHash);
    hashFunctions.push_back(HashFuncs::DJBHash);
    hashFunctions.push_back(HashFuncs::DEKHash);
}

BloomFilter::BloomFilter(std::vector<HashFunction> funcs) : cellNum(BLOOM_CELL_NUM), hashFunctions(funcs) { bloomCell.reset(); }

BloomFilter::~BloomFilter() {}

RC BloomFilter::InsertData(std::string str) {
    for (auto const &func : hashFunctions) {
        int curIndex = func(str) % cellNum;
        bloomCell.set(curIndex);
    }
    return OK;
}

RC BloomFilter::InsertData(std::string str, std::bitset<BLOOM_CELL_NUM> cell) {
    for (auto const &func : hashFunctions) {
        cell.set(func(str) % cellNum);
    }
    return OK;
}

bool BloomFilter::IsContain(std::string str) {
    for (auto const &func : hashFunctions) {
        int curIndex = func(str) % cellNum;
        if (!bloomCell[curIndex]) {
            return false;
        }
    }
    return true;
}

bool BloomFilter::IsContain(std::string str, std::bitset<BLOOM_CELL_NUM> cell) {
    for (auto const &func : hashFunctions) {
        if (!cell[func(str) % cellNum]) {
            return false;
        }
    }
    return true;
}

int BloomFilter::getHashFuncNum() { return hashFunctions.size(); }

std::vector<HashFunction> &BloomFilter::getHashFunctions() { return hashFunctions; }

std::bitset<BLOOM_CELL_NUM> &BloomFilter::getBloomCell() { return bloomCell; }
