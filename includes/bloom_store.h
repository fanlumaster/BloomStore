#ifndef BLOOM_STORE_H
#define BLOOM_STORE_H

#include <windows.h>
#include <functional>
#include <string>
#include <vector>

// return code
typedef int RC;
#define OK 0
#define ERR (-1)

// num of hash functions that bloom filter used
#define HASH_FUNC_NUM_USED 8
#define BLOOM_FILTER_CELL_SIZE 1024
#define MAX_FILENAME_LEN 50

using HashFunction = std::function<unsigned int(std::string)>;

class HashFuncs {
  public:
    HashFuncs();
    /* HashFuncs(int num); */
    ~HashFuncs();
    /* RC GenerateHashValue(char *str, unsigned int *ret); */
    /* unsigned int HashValue(char *str, int index); */

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
};

// bloom filter
class BloomFilter {
  public:
    BloomFilter();
    BloomFilter(unsigned int num, std::vector<HashFunction> funcs);
    ~BloomFilter();
    RC InsertData(std::string str);
    bool IsContain(std::string str);

  private:
    /* int bucketNum;              // how many buckets are used in current bloom filter */
    int hashFuncNum;            // how many hash functions are used in current bloom filter
    unsigned int *hashFuncsRet; // hash value generated by hash functions
    unsigned int cellNum;
    std::vector<bool> bloomCell;
    std::vector<HashFunction> hashFunctions;
};

class Log {
  public:
    Log(const char *name);
    ~Log();
    HANDLE GetHandle();

  private:
    HANDLE handle;
    char DBName[MAX_FILENAME_LEN];
    RC _file_exists();
};

// one explicit instance
class BloomStoreInstance {
  public:
    BloomStoreInstance(const char *filename); // one instance corresponds to one db file in flash store
    ~BloomStoreInstance();
    RC InsertData(const char *key, const char *value); // insert a KV pair to database
    RC FindData(const char *key);                      // find a KV pair by key
    RC CloseDB();

  private:
    HANDLE fileHandle;
};

#endif // !BLOOM_STORE_H
