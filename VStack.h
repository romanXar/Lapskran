#pragma once
#include <cstdint>

class VMemory {
private:
    // CArray<uint64_t, CArrayMode::Stack, 65536> data; // Старый вариант
    uint64_t* data; 
    intptr_t cur = -1;
    const intptr_t CAPACITY = 10000000; 

public:
    void Alloc(intptr_t size) {
        cur += size;
        // data.EnsureSize(cur); 
    }

    void DeAlloc(intptr_t size) {
        cur -= size;
    }

    void Set(intptr_t data_pos, intptr_t value) {
        // data.get(cur - data_pos) = static_cast<uint64_t>(value);
        data[cur - data_pos] = static_cast<uint64_t>(value); 
    }

    intptr_t Get(intptr_t data_pos) {
        // return data.get(cur - data_pos);
        return data[cur - data_pos]; 
    }

    void Push(intptr_t value) {
        Alloc(1);
        Set(0, value);
    }

    intptr_t Pop() {
        //if (cur == 0) {
        //    return 0;
       // }
        intptr_t result = Get(1);
        DeAlloc(1);
        return result;
    }

    VMemory() {
        cur = 0;
        // data.EnsureSize(131072); 
        data = new uint64_t[CAPACITY]; 
    }

    ~VMemory() {
        delete[] data; 
    }
};