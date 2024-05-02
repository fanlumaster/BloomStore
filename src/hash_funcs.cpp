#include "bloom_store.h"

HashFuncs::HashFuncs(void) {}

HashFuncs::~HashFuncs() {}

// achieve the partition with evenly distribution and disjoin keys
unsigned int HashFuncs::Hash(std::string str, int instanceNum) {
    unsigned int seed = 131;
    unsigned int hash = 0;
    for (char c : str) {
        hash = hash * seed + c;
    }
    return (hash % instanceNum);
}

unsigned int HashFuncs::HashValue(std::string str, int index) {
    switch (index) {
    case 0:
        return this->RSHash(str);
    case 1:
        return this->JSHash(str);
    case 2:
        return this->PJWHash(str);
    case 3:
        return this->ELFHash(str);
    case 4:
        return this->BKDRHash(str);
    case 5:
        return this->SDBMHash(str);
    case 6:
        return this->DJBHash(str);
    case 7:
        return this->DEKHash(str);
    case 8:
        return this->BPHash(str);
    case 9:
        return this->FNVHash(str);
    case 10:
        return this->APHash(str);
    default:
        return ERR;
    }
}

unsigned int HashFuncs::RSHash(std::string str) {
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    for (char c : str) {
        hash = hash * a + c;
        a = a * b;
    }
    return hash;
}

unsigned int HashFuncs::JSHash(std::string str) {
    unsigned int hash = 1315423911;
    for (char c : str) {
        hash ^= ((hash << 5) + c + (hash >> 2));
    }
    return hash;
}

unsigned int HashFuncs::PJWHash(std::string str) {
    const unsigned int BitsInUnsignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    const unsigned int ThreeQuarters = (unsigned int)((BitsInUnsignedInt * 3) / 4);
    const unsigned int OneEighth = (unsigned int)(BitsInUnsignedInt / 8);
    const unsigned int HighBits = (unsigned int)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
    unsigned int hash = 0;
    unsigned int test = 0;
    for (char c : str) {
        hash = (hash << OneEighth) + c;
        if ((test = hash & HighBits) != 0) {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }
    return hash;
}

unsigned int HashFuncs::ELFHash(std::string str) {
    unsigned int hash = 0;
    unsigned int x = 0;
    for (char c : str) {
        hash = (hash << 4) + c;
        if ((x = hash & 0xF0000000L) != 0) {
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }
    return hash;
}

unsigned int HashFuncs::BKDRHash(std::string str) {
    unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
    unsigned int hash = 0;
    for (char c : str) {
        hash = (hash * seed) + c;
    }
    return hash;
}

unsigned int HashFuncs::SDBMHash(std::string str) {
    unsigned int hash = 0;
    for (char c : str) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

unsigned int HashFuncs::DJBHash(std::string str) {
    unsigned int hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

unsigned int HashFuncs::DEKHash(std::string str) {
    unsigned int hash = str.size();
    //	cout<< " in DEKHash for test let me see the strlen is "<< hash <<endl;
    for (char c : str) {
        hash = ((hash << 5) ^ (hash >> 27)) ^ c;
    }
    return hash;
}

unsigned int HashFuncs::BPHash(std::string str) {
    unsigned int hash = str.size();
    for (char c : str) {
        hash = hash << 7 ^ c;
    }
    return hash;
}

unsigned int HashFuncs::FNVHash(std::string str) {
    const unsigned int fnv_prime = 0x811C9DC5;
    unsigned int hash = 0;
    for (char c : str) {
        hash *= fnv_prime;
        hash ^= c;
    }
    return hash;
}

unsigned int HashFuncs::APHash(std::string str) {
    unsigned int hash = 0xAAAAAAAA;
    int len = str.size();
    unsigned int i = 0;
    for (i = 0; i < len; i++) {
        hash ^= ((i & 1) == 0) ? ((hash << 7) ^ (str[i]) * (hash >> 3)) : (~((hash << 11) + (str[i]) ^ (hash >> 5)));
    }
    return hash;
}
