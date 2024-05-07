#include "bloom_store.h"

#define LOOP (1000000)

void testInsertData() {
    std::string kvPairFileName = "kv_pair";
    std::string BFChainFileName = "bf_chain";
    BloomStore *bloomStore = new BloomStore(kvPairFileName, BFChainFileName);
    KVPair *kvPair = new KVPair;
    std::memset(kvPair->key, '\0', sizeof(kvPair->key));
    std::memset(kvPair->value, '\0', sizeof(kvPair->value));
    std::string key = "fany";
    std::string value = "full";
    std::copy(key.begin(), key.end(), kvPair->key);
    std::copy(value.begin(), value.end(), kvPair->value);
    if (OK == bloomStore->InsertData(kvPair)) {
        // std::cout << "insert success." << '\n';
    }
    char resValue[VSIZE] = {};
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "lookup success, the value is: " << resValue << '\n';
    }
    delete kvPair;
    delete bloomStore;
}

void testInsertMassiveData() {
    std::string kvPairFileName = "kv_pair";
    std::string BFChainFileName = "bf_chain";
    BloomStore *bloomStore = new BloomStore(kvPairFileName, BFChainFileName);
    KVPair *kvPair = new KVPair;
    std::memset(kvPair->key, '\0', sizeof(kvPair->key));
    std::memset(kvPair->value, '\0', sizeof(kvPair->value));
    std::string key = "";
    std::string value = "";
    for (int i = 0; i < LOOP; i++) {
        key = "key:" + std::to_string(i);
        value = "value:" + std::to_string(i);
        std::copy(key.begin(), key.end(), kvPair->key);
        kvPair->key[key.size()] = '\0';
        std::copy(value.begin(), value.end(), kvPair->value);
        kvPair->value[value.size()] = '\0';
        if (OK == bloomStore->InsertData(kvPair)) {
            // std::cout << "insert success." << '\n';
        }
    }

    key = "fany";
    value = "full";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    if (OK == bloomStore->InsertData(kvPair)) {
        // std::cout << "insert success." << '\n';
    }

    char resValue[VSIZE] = {};
    key = "fany";
    if (OK == bloomStore->LookupData(key, resValue)) {
        // std::cout << "lookup success, the value is: " << '\n';
        std::cout << "lookup success, the value is: " << resValue << '\n';
    }
    for (int i = 200; i <= 200 + 66; i++) {
        key = "key:" + std::to_string(i);
        if (OK == bloomStore->LookupData(key, resValue)) {
            // std::cout << "lookup success, the value is: " << '\n';
            std::cout << "lookup success, the value is: " << resValue << '\n';
        }
    }

    delete kvPair;
    delete bloomStore;
}

int main(int argc, char *argv[]) {
    // testInsertData();
    testInsertMassiveData();
    return 0;
}
