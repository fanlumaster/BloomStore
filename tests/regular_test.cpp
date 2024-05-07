#include "bloom_store.h"
#include <chrono>

#define LOOP (100000)

void basicFunctionTest() {
    std::string kvPairFileName = "kv_pair";
    std::string BFChainFileName = "bf_chain";
    BloomStore *bloomStore = new BloomStore(kvPairFileName, BFChainFileName);
    KVPair *kvPair = new KVPair;
    std::memset(kvPair->key, '\0', sizeof(kvPair->key));
    std::memset(kvPair->value, '\0', sizeof(kvPair->value));
    std::string key = "";
    std::string value = "";
    char resValue[VSIZE] = {};

    // insert {"aaaa", "bbbb"}
    key = "aaaa";
    value = "bbbb";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // insert {"yyyyy", "ccc"}
    key = "yyyyy";
    value = "ccc";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // delete "aaaa"
    key = "aaaa";
    bloomStore->DeleteData(key);

    // insert {"aaaa", "bbbb"}
    key = "aaaa";
    value = "bbbb";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // insert {"yyyyy", "ccc"}
    key = "yyyyy";
    value = "ccc";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // delete "aaaa"
    key = "aaaa";
    bloomStore->DeleteData(key);

    // insert {"aaaa", "bbbb"}
    key = "aaaa";
    value = "bbbb";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // insert {"yyyyy", "ccc"}
    key = "yyyyy";
    value = "ccc";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // insert {"fany", "full"}
    key = "fany";
    value = "full";
    std::copy(key.begin(), key.end(), kvPair->key);
    kvPair->key[key.size()] = '\0';
    std::copy(value.begin(), value.end(), kvPair->value);
    kvPair->value[value.size()] = '\0';
    bloomStore->InsertData(kvPair);

    // delete "aaaa"
    key = "aaaa";
    bloomStore->DeleteData(key);

    // look up "aaaa"
    key = "aaaa";
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "The value of key " << key << " is " << resValue << ".\n";
    } else {
        std::cout << "The value of key " << key << " is NOT FOUND!" << '\n';
    }

    // look up "xxx"
    key = "xxx";
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "The value of key " << key << " is " << resValue << ".\n";
    } else {
        std::cout << "The value of key " << key << " is NOT FOUND!" << '\n';
    }

    // look up "yyyyy"
    key = "yyyyy";
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "The value of key " << key << " is " << resValue << ".\n";
    } else {
        std::cout << "The value of key " << key << " is NOT FOUND!" << '\n';
    }

    // look up "fany"
    key = "fany";
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "The value of key " << key << " is " << resValue << ".\n";
    } else {
        std::cout << "The value of key " << key << " is NOT FOUND!" << '\n';
    }
}

void simpleEvaluations() {
    std::string kvPairFileName = "kv_pair";
    std::string BFChainFileName = "bf_chain";
    BloomStore *bloomStore = new BloomStore(kvPairFileName, BFChainFileName);
    KVPair *kvPair = new KVPair;
    std::memset(kvPair->key, '\0', sizeof(kvPair->key));
    std::memset(kvPair->value, '\0', sizeof(kvPair->value));
    std::string key = "";
    std::string value = "";
    char resValue[VSIZE] = {};

    // evaluation for insertion time
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < LOOP; i++) {
        key = "key:" + std::to_string(i);
        value = "value:" + std::to_string(i);
        std::copy(key.begin(), key.end(), kvPair->key);
        kvPair->key[key.size()] = '\0';
        std::copy(value.begin(), value.end(), kvPair->value);
        kvPair->value[value.size()] = '\0';
        if (!(OK == bloomStore->InsertData(kvPair))) {
            std::cout << "Insert failed for {" << key << ", " << value << "}" << '\n';
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Complete inserting." << '\n';
    std::chrono::duration<double> duration = end - start;
    std::cout << "Insertion time for " + std::to_string(LOOP) + " entries: " + std::to_string(duration.count()) << " seconds" << '\n';

    // look up "fany"
    key = "fany";
    if (OK == bloomStore->LookupData(key, resValue)) {
        std::cout << "The value of key " << key << " is " << resValue << ".\n";
    } else {
        std::cout << "The value of key " << key << " is NOT FOUND!" << '\n';
    }

    // evaluation for look up time
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < LOOP; i++) {
        key = "key:" + std::to_string(i);
        if (OK == bloomStore->LookupData(key, resValue)) {
            /* std::cout << "The value of key " << key << " is " << resValue << ".\n"; */
        } else {
            std::cout << "The value of key " << key << " is NOT FOUND!" << '\n';
        }
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Complete looking up." << '\n';
    duration = end - start;
    std::cout << "Lookup time for " + std::to_string(LOOP) + " entries: " + std::to_string(duration.count()) << " seconds" << '\n';

    delete kvPair;
    delete bloomStore;
}

int main(int argc, char *argv[]) {
    /* basicFunctionTest(); */
    simpleEvaluations();
    return 0;
}
