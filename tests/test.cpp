#include "bloom_store.h"

int main(int argc, char *argv[]) {
    std::string kvPairFileName = "kv_pair.db";
    std::string BFChainFileName = "bf_chain.db";
    BloomStore *bloomStore = new BloomStore(kvPairFileName, BFChainFileName);
    KVPair *kvPair = new KVPair;
    std::memset(kvPair->key, '\0', sizeof(kvPair->key));
    std::memset(kvPair->value, '\0', sizeof(kvPair->value));
    std::string key = "fany";
    std::string value = "full";
    std::copy(key.begin(), key.end(), kvPair->key);
    std::copy(value.begin(), value.end(), kvPair->value);
    if (OK == bloomStore->InsertData(kvPair)) {
        std::cout << "insert success." << '\n';
    }
    char resValue[VSIZE] = {};
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "lookup success, the value is: " << resValue << '\n';
    }
    delete kvPair;
    delete bloomStore;
    return 0;
}
