#include "bloom_store.h"

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
RC readStructFromFile(HANDLE hFile, unsigned int offset, int bufferSize, std::vector<BloomFilterBuffer *> &bfVec, int vecIndex) {
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

// read No.offset data of KVPair struct
RC buildKVPairStructFromBytes(char *kvPairDataInFlashTmp, unsigned int offset, int bufferSize, struct KVPair *kvPair) {
    char buffer[sizeof(KVPair)];

    memcpy(buffer, kvPairDataInFlashTmp + offset, bufferSize);

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
        res = bfVec[i]->bloomCell[BLOOM_FILTER_CELL_SIZE - 1 - curIndex] + res; // pay attention to the index here! bitset take the index from right to left
    }
    return res;
}

RC buildBFStructFromBytes(char *BFChainInFlashTmpData, unsigned int offset, int bufferSize, std::vector<BloomFilterBuffer *> &bfVec, int vecIndex) {
    char buffer[sizeof(BloomFilterBuffer)];
    memcpy(buffer, BFChainInFlashTmpData + offset, bufferSize);
    BloomFilterBuffer *myData = reinterpret_cast<BloomFilterBuffer *>(buffer);
    bfVec[vecIndex] = new BloomFilterBuffer;
    bfVec[vecIndex]->pageIndexInFlash = myData->pageIndexInFlash;
    memcpy(bfVec[vecIndex]->bloomCell, myData->bloomCell, sizeof(BloomFilterBuffer));
    return OK;
}

// parallel look up in the left BF chain
// return: all the founded BF index and its corresponding BloomFilterBuffer's pageIndexInFlash
std::vector<std::pair<int, unsigned int>> BloomStoreInstance::LookupKeyInRestBFChain(std::string key) {
    // 1. retrive BFChain in flash BFChain to vector
    int oneBufferSize = sizeof(BloomFilterBuffer);
    std::vector<BloomFilterBuffer *> bfVec(BFChainCnt);
    char *BFChainInFlashTmpData = new char[sizeof(BloomFilterBuffer) * BFChainCnt];
    readBytesFromFile(BFChainHandle, BFChainStartIndex, oneBufferSize * BFChainCnt, BFChainInFlashTmpData);
    for (int j = 0; j < BFChainCnt; j++) {
        buildBFStructFromBytes(BFChainInFlashTmpData, j * oneBufferSize, oneBufferSize, bfVec, j);
    }
    delete[] BFChainInFlashTmpData;

    // 2. execute evaluations in parallel
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
    // (2) if not found key in active BF，then look up the rest BF Chain in the flash
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
    // (3) if found in active BF, then continue to find key in current kv buffer
    if (inActiveBF) {
        for (int i = 0; i < bufferNum; i++) {
            // newKv = reinterpret_cast<KVPair *>(buffer[kvPairBufferSize * i]);
            struct KVPair *curKv = reinterpret_cast<KVPair *>(buffer + kvPairBufferSize * i);
            // (4) if found the key in write buffer, then return OK
            if (strcmp(key.c_str(), curKv->key) == 0) {
                memcpy(value, curKv->value, VSIZE);
                return KEY_FOUND_IN_RAM;
            }
        }
    }
    // (5) if not found in write buffer, then try to look up in the flash
    if (BFChainStartIndex >= 0) {
        bfChainFoundedIndexVec = LookupKeyInRestBFChain(key);
    }
    if (bfChainFoundedIndexVec.size() > 0) {
        inBFChain = true;
    }
    // (6) not found in flash either
    if (!inBFChain) {
        return KEY_NOT_FOUND;
    }
    // (7) try to find in flash
    char *kvPairDataInFlashTmp = new char[sizeof(KVPair) * BUFFER_NUM];
    std::reverse(bfChainFoundedIndexVec.begin(), bfChainFoundedIndexVec.end());
    for (int i = 0; i < bfChainFoundedIndexVec.size(); i++) {
        unsigned int curFlashPageIndex = bfChainFoundedIndexVec[i].second;
        readBytesFromFile(fileHandle, curFlashPageIndex, sizeof(KVPair) * BUFFER_NUM, kvPairDataInFlashTmp);
        int kvPairBufferSize = sizeof(struct KVPair);
        for (unsigned int j = 0; j < BUFFER_NUM; j++) {
            buildKVPairStructFromBytes(kvPairDataInFlashTmp, j * kvPairBufferSize, kvPairBufferSize, newKv);
            //  (8) found
            if (strcmp(newKv->key, key.c_str()) == 0) {
                memcpy(value, newKv->value, VSIZE);
                return KEY_FOUND_IN_FLASH;
            }
        }
    }
    delete[] kvPairDataInFlashTmp;
    // (9) not found
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

bool BloomStoreInstance::readBytesFromFile(HANDLE hFile, unsigned int offset, int bufferSize, char *data) {
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

RC BloomStoreInstance::InsertData(struct KVPair *kv) {
    std::string key = kv->key;
    std::string valueToInsert = kv->value;
    // 1. look up in the activeBF first, execute bloom filter func before inserting data
    // if not found, then insert directly, if found, then, substitude in place
    // and, we only change in place when finding the key in RAM
    char value[VSIZE];
    struct KVPair *kvHelper;
    kvHelper = new struct KVPair;
    int kvPairBufferSize = sizeof(struct KVPair);
    RC lookupRc = LookupData(key, value);
    if (key == "key:99") {
        key = "key:99";
    }
    activeBF->InsertData(key);
    // 1.1 if founded in RAM
    if (lookupRc == KEY_FOUND_IN_RAM) {
        if (strcmp(value, kv->value) == 0) {
            return OK;
        }
        // 1.2 update KV Pair in the RAM
        for (int i = 0; i < bufferNum; i++) {
            kvHelper = reinterpret_cast<KVPair *>(buffer + kvPairBufferSize * i);
            if (strcmp(key.c_str(), kvHelper->key) == 0) {
                memcpy(buffer + (kvPairBufferSize * i), kv, kvPairBufferSize);
                return OK;
            }
        }
    }

    if (lookupRc == KEY_FOUND_IN_FLASH) {
        if (strcmp(value, kv->value) == 0) {
            return OK;
        }
    }

    // 2. update/insert
    // 2.1 insert to RAM first
    memcpy(buffer + (kvPairBufferSize * bufferNum), kv, kvPairBufferSize);
    bufferNum += 1;

    // 2.2 if current buffer is full, then write the buffer to the flash
    if (bufferNum == BUFFER_NUM) {
        int kvPairFileSize = GetFileSize(fileHandle, NULL);
        // 2.1. write the KVPair page buffer
        writeBytesToFile(fileHandle, buffer, BUFFER_NUM * kvPairSize);
        // 2.2 write the BFChain buffer
        // 2.2.1 build current activeBFBuffer
        activeBFBuffer->pageIndexInFlash = kvPairFileSize;
        std::string curBloomCell = activeBF->getBloomCell().to_string();
        std::copy(curBloomCell.begin(), curBloomCell.end(), activeBFBuffer->bloomCell);
        // 2.2.2 retrive BF Chain in the flash and concat the active buffer to the end of the previous BF Chain, then, write the newly built BF Chain to the flash
        int bfFileSize = GetFileSize(BFChainHandle, NULL);
        int oneBufferSize = sizeof(BloomFilterBuffer);
        std::vector<BloomFilterBuffer> bfVec(BFChainCnt);
        char tmpBFData[sizeof(BloomFilterBuffer)]; // for a single BloomFilterBuffer
        // copy and write in chunk
        char *BFChainInFlashTmpData = new char[sizeof(BloomFilterBuffer) * (BFChainCnt + 1)];
        readBytesFromFile(BFChainHandle, BFChainStartIndex, oneBufferSize * BFChainCnt, BFChainInFlashTmpData);
        // concat
        memcpy(BFChainInFlashTmpData + (oneBufferSize * BFChainCnt), (char *)activeBFBuffer, sizeof(BloomFilterBuffer));
        // write to flash
        writeBytesToFile(BFChainHandle, BFChainInFlashTmpData, oneBufferSize * (BFChainCnt + 1));
        delete[] BFChainInFlashTmpData;

        // flush to the flash manually
        if (!FlushFileBuffers(fileHandle)) {
            CloseHandle(fileHandle);
            return 1;
        }
        if (!FlushFileBuffers(BFChainHandle)) {
            CloseHandle(BFChainHandle);
            return 1;
        }

        int tmpSize = GetFileSize(fileHandle, NULL);
        int tmp02Size = GetFileSize(BFChainHandle, NULL);
        // 2.3 update some RAM variables
        bufferNum = 0;
        BFChainStartIndex = bfFileSize;
        BFChainCnt += 1;
    }
    return OK;
}
