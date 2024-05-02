#include "bloom_store.h"

BloomStore::BloomStore(std::string filename, std::string BFChainLogFilename) {
    log = new Log(filename);
    BFChainLog = new Log(BFChainLogFilename);
    instanceNum = 6; // default instance num
    for (int _i = 0; _i < instanceNum; _i++) {
        BloomStoreInstanceVec.push_back(BloomStoreInstance(log->GetHandle(), BFChainLog->GetHandle()));
    }
    emptyKVPairNumTotal = 0;
    allKVPairNumsTotal = 0;
}

BloomStore::~BloomStore() {
    delete log;
    delete BFChainLog;
}

RC BloomStore::InsertData(struct KVPair *kv) {
    unsigned int curInstanceIndex = HashFuncs::Hash(std::string(kv->key), instanceNum);
    BloomStoreInstance curInstance = BloomStoreInstanceVec[curInstanceIndex];
    curInstance.InsertData(kv);
    return OK;
}

// remove first bytesToRemove bytes of input file
bool BloomStore::removeFirstBytes(const char *inputFileName, int bytesToRemove) {
    HANDLE inputFile = CreateFile(inputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (inputFile == INVALID_HANDLE_VALUE) {
        /* std::cerr << "cannot open input file." << std::endl; */
        return false;
    }

    const char *tempFileName = "temp.txt"; // temp file name
    HANDLE tempFile = CreateFile(tempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        CloseHandle(inputFile);
        /* std::cerr << "cannot create temp file." << std::endl; */
        return false;
    }

    // locate to bytesToRemove
    LARGE_INTEGER li;
    li.QuadPart = bytesToRemove;
    SetFilePointerEx(inputFile, li, NULL, FILE_BEGIN);

    // read from input file, and write to temp file
    BYTE buffer[1024];
    DWORD bytesRead;
    while (ReadFile(inputFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        WriteFile(tempFile, buffer, bytesRead, NULL, NULL);
    }

    // close file handle
    CloseHandle(inputFile);
    CloseHandle(tempFile);

    // use content of temp file to overwrite the input file
    if (!MoveFileEx(tempFileName, inputFileName, MOVEFILE_REPLACE_EXISTING)) {
        /* std::cerr << "move file content error occurs." << std::endl; */
        return false;
    }

    /* std::cout << "input file has already benn overwritten." << std::endl; */
    return true;
}
