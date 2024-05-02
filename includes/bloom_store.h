#ifndef BLOOM_STORE_H
#define BLOOM_STORE_H

#include <windows.h>
#include <functional>
#include <string>
#include <vector>
#include <bitset>
#include <execution>
#include <algorithm>

// return code
typedef int RC;
#define OK 0
#define ERR (-1)
#define KEY_NOT_FOUND (-2)
#define WARN 1
#define FILE_NOT_EXISTS 2
#define FILE_RELATED_ERR 3

// num of hash functions that bloom filter used
#define HASH_FUNC_NUM_USED 8
#define BLOOM_FILTER_CELL_SIZE 1024
#define MAX_FILENAME_LEN 50

// page size, normally 4096 bytes
#define PAGE_SIZE 4096
#define BUFFER_SIZE 4096
#define BUFFER_NUM 4096

#define BLOOM_CELL_NUM 1024

#define KSIZE 30
#define VSIZE 50

#define MAX_BF_COUNT 1000

enum OpType { ADD_DATA, DELETE_DATA };

using HashFunction = std::function<unsigned int(std::string)>;

struct KVPair {
    /* std::string key; */
    char key[KSIZE];
    /* std::string value; */
    char value[VSIZE];
    int indexNum; // index among all KVPairs in one page
    int op;
};

struct BloomFilterBuffer {
    unsigned int pageIndexInFlash;
    char bloomCell[BLOOM_CELL_NUM];
    /* std::bitset<BLOOM_CELL_NUM> bloomCell; */
};

class HashFuncs {
  public:
    HashFuncs();
    /* HashFuncs(int num); */
    ~HashFuncs();
    static unsigned int Hash(std::string str, int instanceNum);

    /* RC GenerateHashValue(char *str, unsigned int *ret); */
    unsigned int HashValue(std::string str, int index);

    static unsigned int RSHash(std::string str);
    static unsigned int JSHash(std::string str);
    static unsigned int PJWHash(std::string str);
    static unsigned int ELFHash(std::string str);
    static unsigned int BKDRHash(std::string str);
    static unsigned int SDBMHash(std::string str);
    static unsigned int DJBHash(std::string str);
    static unsigned int DEKHash(std::string str);
    static unsigned int BPHash(std::string str);
    static unsigned int FNVHash(std::string str);
    static unsigned int APHash(std::string str);

  private:
};

// bloom filter
class BloomFilter {
  public:
    BloomFilter();
    BloomFilter(std::vector<HashFunction> funcs);
    ~BloomFilter();
    RC InsertData(std::string str);
    RC InsertData(std::string str, std::bitset<BLOOM_CELL_NUM> bloomCell);
    bool IsContain(std::string str);
    bool IsContain(std::string str, std::bitset<BLOOM_CELL_NUM> cell);
    int getHashFuncNum();
    std::vector<HashFunction> &getHashFunctions();

  private:
    unsigned int cellNum;
    std::bitset<BLOOM_CELL_NUM> bloomCell;
    std::vector<HashFunction> hashFunctions;
};

class Log {
  public:
    Log(std::string filename);
    ~Log();
    HANDLE GetHandle();

  private:
    HANDLE handle;
    std::string DBName;
    RC _file_exists();
};

// one explicit instance
class BloomStoreInstance {
  public:
    BloomStoreInstance(HANDLE _handle, HANDLE _BFChainHandle); // one instance corresponds to one db file in flash store
    ~BloomStoreInstance();
    RC InsertData(struct KVPair *kv);            // insert a KV pair to database
    RC LookupData(std::string key, char *value); // find a KV pair by key
    RC DeleteData(std::string key);
    RC CloseDB();

  private:
    HANDLE fileHandle;
    HANDLE BFChainHandle;
    int kvPairSize;                           // size of one KV Pair
    BloomFilter *activeBF;                    // actiave BF
    char *buffer;                             // Write Buffer
    int bufferNum;                            // count of KVPair in write buffer
    struct BloomFilterBuffer *activeBFBuffer; // current active BF in the RAM
    int pageSize;                             // flash page size
    int emptyKVPairNum;                       // count of empty KV Pairs
    int allKVPairNums;                        // includes deleted KV Pairs
    unsigned int BFChainStartIndex;           // start of bloom chain in the flash, maintained in RAM, means there are BFChainStartIndex * sizeof(BloomFilterBuffer) bytes before
    unsigned int BFChainCnt;                  // count of BFs in the flash, maintained in RAM
    bool LookupKeyInActiveBF(std::string key);
    RC BloomStoreInstance::RetriveBFChainFromFlash();
    std::vector<std::pair<int, unsigned int>> LookupKeyInRestBFChain(std::string key);
};

class BloomStore {
  public:
    BloomStore(std::string filename, std::string BFChainLogFilename);
    ~BloomStore();
    RC InsertData(struct KVPair *kv);
    RC LookupData(std::string key); // find a KV pair by key
    RC DeleteData(std::string key);
    bool removeFirstBytes(const char *inputFileName, int bytesToRemove);

  private:
    Log *log; // flash store, the rest BF chain and KV chain are both in file
    Log *BFChainLog;
    int instanceNum;
    std::vector<BloomStoreInstance> BloomStoreInstanceVec;
    int emptyKVPairNumTotal; // all instances
    int allKVPairNumsTotal;  // all instances
};

#endif // !BLOOM_STORE_H
