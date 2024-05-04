#include "bloom_store.h"
#include <fileapi.h>

BloomStoreInstance::BloomStoreInstance() {
    kvPairSize = sizeof(struct KVPair);
    activeBF = new BloomFilter();
    /* buffer = new char[BUFFER_NUM * kvPairSize]; */
    bufferNum = 0;
    activeBFBuffer = new struct BloomFilterBuffer;
    emptyKVPairNum = 0;
    allKVPairNums = 0;
    BFChainStartIndex = -1; // default is -1, means not exist yet
    BFChainCnt = 0;
}

BloomStoreInstance::BloomStoreInstance(HANDLE _handle, HANDLE _BFChainHandle) {
    fileHandle = _handle;
    BFChainHandle = _BFChainHandle;
    kvPairSize = sizeof(struct KVPair);
    activeBF = new BloomFilter();
    bufferNum = 0;
    activeBFBuffer = new struct BloomFilterBuffer;
    emptyKVPairNum = 0;
    allKVPairNums = 0;
    BFChainStartIndex = -1; // default is -1, means not exist yet
    BFChainCnt = 0;
}

BloomStoreInstance::~BloomStoreInstance() {
    delete activeBF;
    delete activeBFBuffer;
}

bool BloomStoreInstance::LookupKeyInActiveBF(std::string key) { return activeBF->IsContain(key); }

RC BloomStoreInstance::RetriveBFChainFromFlash() { return OK; }

// read No.offset data of BloomFilterBuffer struct
RC readStructFromFile(HANDLE hFile, unsigned int offset, int bufferSize, std::vector<BloomFilterBuffer*> &bfVec, int vecIndex) {
    char buffer[sizeof(BloomFilterBuffer)];
    // move file cursor to needed position
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + offset; // set the offset, the unit is byte
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
    bfVec[vecIndex] = new BloomFilterBuffer;
    bfVec[vecIndex]->pageIndexInFlash = myData->pageIndexInFlash;
    memcpy(bfVec[vecIndex]->bloomCell, myData->bloomCell, sizeof(BloomFilterBuffer));
    return OK;
}

// read No.offset data of KVPair struct
RC readKVPairStructFromFile(HANDLE hFile, unsigned int offset, int bufferSize, struct KVPair *kvPair) {
    char buffer[sizeof(KVPair)];
    // move file cursor to needed position
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + offset; // set file pointer
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
    memcpy(kvPair->key, myData->key, KSIZE);
    memcpy(kvPair->value, myData->value, VSIZE);
    return OK;
}

// retrive every bit that corresponds to current hash value index
std::string retriveAllHorizontalBits(int hashFuncIndex, std::vector<BloomFilterBuffer *> &bfVec, std::vector<HashFunction> &hashFunctions, std::string &key, const std::string &str) {
    int curIndex = hashFunctions[hashFuncIndex](key) % BLOOM_FILTER_CELL_SIZE;
    std::string res = "";
    for (int i = 0; i < bfVec.size(); i++) {
        res += bfVec[i]->bloomCell[BLOOM_FILTER_CELL_SIZE - 1 - curIndex]; // take care the index here! bitset take the index from right to left
    }
    return res;
}

// parallel look up in the left BF chain
// return: all the founded BF index and its corresponding BloomFilterBuffer's pageIndexInFlash
std::vector<std::pair<int, unsigned int>> BloomStoreInstance::LookupKeyInRestBFChain(std::string key) {
    // 1. retrive BFChain in flash BFChain to vector
    int oneBufferSize = sizeof(BloomFilterBuffer);
    std::vector<BloomFilterBuffer*> bfVec(BFChainCnt);
    for (int j = 0; j < BFChainCnt; j++) {
        readStructFromFile(BFChainHandle, BFChainStartIndex + j * oneBufferSize, oneBufferSize, bfVec, j);
        // std::string fanywhat = bfVec[j]->bloomCell
    }
    
    // 并行进行判断
    std::vector<std::string> bfHorizontalData(activeBF->getHashFuncNum());
    int hashFuncIndex = 0;
    std::vector<HashFunction> &curHashFuncs = activeBF->getHashFunctions();
    std::transform(std::execution::par, bfHorizontalData.begin(), bfHorizontalData.end(), bfHorizontalData.begin(), [&hashFuncIndex, &bfVec, &curHashFuncs, &key](const std::string &str) { return retriveAllHorizontalBits(hashFuncIndex++, bfVec, curHashFuncs, key, str); });
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
            foundedIndexVec.push_back(std::make_pair(i, bfVec[i]->pageIndexInFlash));
        }
    }
    return foundedIndexVec;
}

RC BloomStoreInstance::LookupData(std::string key, char *value) {
    // (1) look up key in the active BF
    bool inActiveBF = LookupKeyInActiveBF(key);
    // (2) if not found key in active BF，则遵循RAM中的flash指针以检索BF链的其余部分，并执行BF的并行查找
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
            // newKv = reinterpret_cast<KVPair *>(buffer[kvPairBufferSize * i]);
            struct KVPair *curKv = reinterpret_cast<KVPair *>(buffer + kvPairBufferSize * i);
            // (4) 如果在写缓冲区中找到键，则键查找操作返回肯定值
            if (strcmp(key.c_str(), curKv->key) == 0) {
                memcpy(value, curKv->value, VSIZE);
                return KEY_FOUND_IN_RAM;
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
    // (7) 对于BF链中找到键的每个BF，将提取一个flash指针并跟随以搜索其对应的包含KV对的flash页面；如果需要搜索多个flash页面，则BloomStore将根据它们的写入时间的相反顺序（从最大BF标签）搜索页面；然后，在找到第一个与搜索的键匹配的KV对时，BloomStore停止其查找并肯定返回。
    std::reverse(bfChainFoundedIndexVec.begin(), bfChainFoundedIndexVec.end());
    for (int i = 0; i < bfChainFoundedIndexVec.size(); i++) {
        unsigned int curFlashPageIndex = bfChainFoundedIndexVec[i].second;
        int kvPairBufferSize = sizeof(struct KVPair);
        for (unsigned int j = 0; j < BUFFER_NUM; j++) {
            readKVPairStructFromFile(fileHandle, curFlashPageIndex + j * kvPairBufferSize, kvPairBufferSize, newKv);
            if (strcmp(newKv->key, key.c_str()) == 0) {
                memcpy(value, newKv->value, VSIZE);
                return KEY_FOUND_IN_FLASH;
            }
        }
    }
    // (8) 如果找到键，则查找操作返回肯定值
    // (9) 如果未找到键，则查找操作返回否定值
    return KEY_NOT_FOUND;
}

bool writeKVPairStructToFile(HANDLE hFile, KVPair &data) {
    // move file pointer to the end of file
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0;
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_END)) {
        CloseHandle(hFile);
        return false;
    }

    // write bytes to file
    DWORD bytesWritten;
    if (!WriteFile(hFile, &data, sizeof(data), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        return false;
    }
    return true;
}

bool readBytesFromFile(HANDLE hFile, unsigned int offset, int bufferSize, char *data) {
    // move file pointer to the end of file
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0 + offset; // set file pointer
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_BEGIN)) {
        CloseHandle(hFile);
        return false;
    }

    // read file content
    DWORD bytesRead;
    if (!ReadFile(hFile, data, bufferSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        return false;
    }
    return true;
}

bool writeBytesToFile(HANDLE hFile, char *data, int bufferSize) {
    // move file pointer to the end of file
    LARGE_INTEGER moveOffset;
    moveOffset.QuadPart = 0;
    if (!SetFilePointerEx(hFile, moveOffset, NULL, FILE_END)) {
        CloseHandle(hFile);
        return false;
    }

    // write bytes to file
    DWORD bytesWritten;
    if (!WriteFile(hFile, data, bufferSize, &bytesWritten, NULL)) {
        CloseHandle(hFile);
        return false;
    }
    return true;
}

// 前提：已经定位到这个 instance 中来了
RC BloomStoreInstance::InsertData(struct KVPair *kv) {
    std::string key = kv->key;
    std::string valueToInsert = kv->value;
    // TODO: 先在 activeBF 中进行查找， bloom filter func before insert, 如果没找到，那就直接插入，如果找到了，那么，找到那个位置，进行替换
    char value[VSIZE];
    struct KVPair *kvHelper;
    kvHelper = new struct KVPair;
    int kvPairBufferSize = sizeof(struct KVPair);
    RC lookupRc = LookupData(key, value);
    if (key == "key:99") {
        key = "key:99";
    }
    activeBF->InsertData(key);
    // if founded in RAM
    if (lookupRc == KEY_FOUND_IN_RAM) {
        if (strcmp(value, kv->value) == 0) {
            return OK;
        }
        // update KV Pair in the RAM
        for (int i = 0; i < bufferNum; i++) {
            kvHelper = reinterpret_cast<KVPair *>(buffer[kvPairBufferSize * i]);
            if (strcmp(key.c_str(), kvHelper->key) == 0) {
                memcpy(buffer + (kvPairBufferSize * i), kv, kvPairBufferSize);
                delete kvHelper;
                return OK;
            }
        }
    }

    if (lookupRc == KEY_FOUND_IN_FLASH) {
        if (strcmp(value, kv->value) == 0) {
            return OK;
        }
    }

    // update/insert
    memcpy(buffer + (kvPairBufferSize * bufferNum), kv, kvPairBufferSize);
    bufferNum += 1;

    // if current buffer is full, then write the buffer to the flash
    if (bufferNum == BUFFER_NUM) {
        int kvPairFileSize = GetFileSize(fileHandle, NULL);
        // 1. write the KVPair page buffer
        writeBytesToFile(fileHandle, buffer, BUFFER_NUM * kvPairSize);
        // 2. write the BFChain buffer
        // 2.1 build current activeBFBuffer
        activeBFBuffer->pageIndexInFlash = kvPairFileSize;
        std::string curBloomCell = activeBF->getBloomCell().to_string();
        std::copy(curBloomCell.begin(), curBloomCell.end(), activeBFBuffer->bloomCell);
        // 2.2
        int bfFileSize = GetFileSize(BFChainHandle, NULL);
        int oneBufferSize = sizeof(BloomFilterBuffer);
        std::vector<BloomFilterBuffer> bfVec(BFChainCnt);
        /* struct BloomFilterBuffer *tmpData = new struct BloomFilterBuffer; */
        char tmpData[sizeof(struct BloomFilterBuffer)];
        for (int j = 0; j < BFChainCnt; j++) {
            readBytesFromFile(BFChainHandle, BFChainStartIndex + j * oneBufferSize, oneBufferSize, tmpData);
            writeBytesToFile(BFChainHandle, tmpData, oneBufferSize);
        }
        writeBytesToFile(BFChainHandle, (char *)activeBFBuffer, oneBufferSize);

       if (!FlushFileBuffers(BFChainHandle)) {
            CloseHandle(BFChainHandle);
            return 1;
        }
        int tmpSize =  GetFileSize(fileHandle, NULL);
        int tmp02Size = GetFileSize(BFChainHandle, NULL);
        // 2.3 update some RAM variables
        bufferNum = 0;
        BFChainStartIndex = bfFileSize;
        BFChainCnt += 1;
    }
    return OK;
}
