#include "bloom_store.h"

BloomStoreInstance::BloomStoreInstance(HANDLE _handle, HANDLE _BFChainHandle) {
    fileHandle = _handle;
    BFChainHandle = _BFChainHandle;
    kvPairSize = sizeof(struct KVPair);
    activeBF = new BloomFilter();
    buffer = new char[BUFFER_NUM * kvPairSize];
    bufferNum = 0;
    activeBFBuffer = new struct BloomFilterBuffer;
    emptyKVPairNum = 0;
    allKVPairNums = 0;
    BFChainStartIndex = -1; // default is -1, means not exist yet
}

BloomStoreInstance::~BloomStoreInstance() {
    delete activeBF;
    delete[] buffer;
    delete activeBFBuffer;
}

// 前提：已经定位到这个 instance 中来了
RC BloomStoreInstance::InsertData(struct KVPair *kv) {
    std::string key = kv->key;
    // TODO: 先在 activeBF 中进行查找， bloom filter func before insert, 如果没找到，那就直接插入，如果找到了，那么，找到那个位置，进行替换
    bool founded = activeBF->IsContain(key);

    if (!founded) {
        // 继续在 flash 中的 BF Chain 中查找
    } else {
        // 在 active BF 中找到了，那么，尝试在 Write Buffer 中查找
        // TODO: try to find in write buf
        bool foundedInWriteBuffer = true;

        // not found
    }
    return OK;
}

bool BloomStoreInstance::LookupKeyInActiveBF(std::string key) { return activeBF->IsContain(key); }

RC BloomStoreInstance::RetriveBFChainFromFlash() { return OK; }

// read No.offset data of BloomFilterBuffer struct
RC readStructFromFile(HANDLE hFile, unsigned int offset, int bufferSize, std::vector<BloomFilterBuffer> &bfVec, int vecIndex) {
    char buffer[bufferSize];
    // move file cursor to needed position
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + bufferSize * offset; // 设置为你想要读取的起始位置
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_BEGIN)) {
        CloseHandle(hFile);
        return FILE_RELATED_ERR;
    }

    // read file content
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, bufferSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        return FILE_RELATED_ERR;
    }

    // convert bytes to struct
    BloomFilterBuffer *myData = reinterpret_cast<BloomFilterBuffer *>(buffer);
    bfVec[vecIndex] = *myData;
    return OK;
}

// read No.offset data of KVPair struct
RC readKVPairStructFromFile(HANDLE hFile, unsigned int offset, int bufferSize, struct KVPair *kvPair) {
    char buffer[bufferSize];
    // move file cursor to needed position
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + bufferSize * offset; // set file pointer
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_BEGIN)) {
        CloseHandle(hFile);
        return FILE_RELATED_ERR;
    }

    // read file content
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, bufferSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        return FILE_RELATED_ERR;
    }

    // convert bytes to struct
    KVPair *myData = reinterpret_cast<KVPair *>(buffer);
    *kvPair = *myData;
    return OK;
}

// retrive every bit that corresponds to current hash value index
std::string retriveAllHorizontalBits(int hashFuncIndex, std::vector<BloomFilterBuffer> &bfVec, std::vector<HashFunction> &hashFunctions, std::string &key) {
    int curIndex = hashFunctions[hashFuncIndex](key);
    std::string res = "";
    for (int i = 0; i < bfVec.size(); i++) {
        res += bfVec[i].bloomCell[curIndex];
    }
    return res;
}

// parallel look up in the left BF chain
// return: all the founded BF index and its corresponding BloomFilterBuffer's pageIndexInFlash
std::vector<std::pair<int, unsigned int>> BloomStoreInstance::LookupKeyInRestBFChain(std::string key) {
    // 1. 取出 flash 中的 BFChain 到 vector 中
    int oneBufferSize = sizeof(BloomFilterBuffer);
    std::vector<BloomFilterBuffer> bfVec(BFChainCnt);
    for (int i = BFChainStartIndex, j = 0; i < BFChainStartIndex + BFChainCnt; i++, j++) {
        readStructFromFile(BFChainHandle, i * oneBufferSize, oneBufferSize, bfVec, j);
    }
    // 并行进行判断
    std::vector<std::string> bfHorizontalData(activeBF->getHashFuncNum());
    int hashFuncIndex = 0;
    std::vector<HashFunction> &curHashFuncs = activeBF->getHashFunctions();
    std::transform(std::execution::par, bfHorizontalData.begin(), bfHorizontalData.end(), bfHorizontalData.begin(), [&hashFuncIndex, &bfVec, &curHashFuncs, &key]() { return retriveAllHorizontalBits(hashFuncIndex++, bfVec, curHashFuncs, key); });
    std::bitset<MAX_BF_COUNT> bitRes;
    for (int i = 0; i < bfVec.size(); i++) {
        bitRes.set(i);
    }
    for (int i = 0; i < bfHorizontalData.size(); i++) {
        std::bitset<MAX_BF_COUNT> bitTmp(bfHorizontalData[i]);
        bitRes = bitRes & bitTmp;
    }
    std::vector<std::pair<int, unsigned int>> foundedIndexVec;
    for (int i = 0; i < bfHorizontalData.size(); i++) {
        if (bitRes[i]) {
            foundedIndexVec.push_back(std::make_pair(i, bfVec[i].pageIndexInFlash));
        }
    }
    return foundedIndexVec;
}

RC BloomStoreInstance::LookupData(std::string key, char *value) {
    // (1) 在活动BF中查找键
    bool inActiveBF = LookupKeyInActiveBF(key);
    // (2) 如果在活动BF中未找到键，则遵循RAM中的flash指针以检索BF链的其余部分，并执行BF的并行查找
    bool inBFChain = false;
    std::vector<std::pair<int, unsigned int>> bfChainFoundedIndexVec;
    /* if (!inActiveBF && BFChainStartIndex >= 0) { */
    /*     bfChainFoundedIndexVec = LookupKeyInRestBFChain(key); */
    /* } */
    /* if (bfChainFoundedIndexVec.size() > 0) { */
    /*     inBFChain = true; */
    /* } */
    struct KVPair *newKv;
    newKv = new struct KVPair;
    int kvPairBufferSize = sizeof(struct KVPair);
    // (3) 如果在活动BF中找到键，则继续在KV对写缓冲区中检查键
    if (inActiveBF) {
        for (int i = 0; i < bufferNum; i++) {
            newKv = reinterpret_cast<KVPair *>(buffer[kvPairBufferSize * i]);
            // (4) 如果在写缓冲区中找到键，则键查找操作返回肯定值
            if (strcmp(key.c_str(), newKv->key) == 0) {
                memcpy(value, newKv->value, VSIZE);
                delete newKv;
                return OK;
            }
        }
    }
    // (5) 如果在写缓冲区中未找到键，则BloomStore检索BF链的其余部分，并在所有获取的BF上执行BF的并行查找
    if (BFChainStartIndex >= 0) {
        bfChainFoundedIndexVec = LookupKeyInRestBFChain(key);
    }
    if (bfChainFoundedIndexVec.size() > 0) {
        inBFChain = true;
    }
    // (6) 如果在BF链的任何BF中找不到键，则该键被视为新键
    if (!inBFChain) {
        return KEY_NOT_FOUND;
    }
    // (7)
    // 对于BF链中找到键的每个BF，将提取一个flash指针并跟随以搜索其对应的包含KV对的flash页面；如果需要搜索多个flash页面，则BloomStore将根据它们的写入时间的相反顺序（从最大BF标签）搜索页面；然后，在找到第一个与搜索的键匹配的KV对时，BloomStore停止其查找并肯定返回。
    std::reverse(bfChainFoundedIndexVec.begin(), bfChainFoundedIndexVec.end());
    for (int i = 0; i < bfChainFoundedIndexVec.size(); i++) {
        unsigned int curFlashPageIndex = bfChainFoundedIndexVec[i].second;
        int kvPairBufferSize = sizeof(struct KVPair);
        for (unsigned int j = 0; j < BUFFER_NUM; j++) {
            readKVPairStructFromFile(fileHandle, (curFlashPageIndex + j) * kvPairBufferSize, kvPairBufferSize, newKv);
            if (strcmp(newKv->value, key.c_str())) {
                memcpy(value, newKv->value, VSIZE);
                delete newKv;
                return OK;
            }
        }
    }
    // (8) 如果找到键，则查找操作返回肯定值
    // (9) 如果未找到键，则查找操作返回否定值
    delete newKv;
    return KEY_NOT_FOUND;
}
