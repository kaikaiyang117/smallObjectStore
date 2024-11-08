#include <unordered_map>
#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cassert>

struct KVNode {
    std::string key;        
    long offset;            
    size_t length;          
    void* memory_address;   
    bool in_memory;         
    std::time_t timestamp;  

    KVNode(const std::string& key, long offset, size_t length, void* memory_address, bool in_memory, std::time_t timestamp)
        : key(key), offset(offset), length(length), memory_address(memory_address), in_memory(in_memory), timestamp(timestamp) {}

    KVNode() : offset(0), length(0), memory_address(nullptr), in_memory(false), timestamp(0) {}
};

class KVStore {
private:
    std::unordered_map<std::string, KVNode> index;
    std::vector<std::string> keyBuffer;
    std::vector<std::string> valueBuffer;
    size_t bufferLimit;
    std::ofstream diskFile;
    std::string disk_filename;

public:
    KVStore(size_t buffer_limit, const std::string& disk_filename) : bufferLimit(buffer_limit), disk_filename(disk_filename) {
        diskFile.open(disk_filename, std::ios::app | std::ios::binary);
        if (!diskFile.is_open()) {
            throw std::runtime_error("无法打开磁盘文件");
        }
    }

    ~KVStore() {
        if (diskFile.is_open()) {
            flushBuffersToDisk();
            diskFile.close();
        }
    }

    bool write(const std::string& key, const std::string& value) {
        keyBuffer.push_back(key);
        valueBuffer.push_back(value);

        if (keyBuffer.size() >= bufferLimit) {
            flushBuffersToDisk();
        }
        return true;
    }

    void flushBuffersToDisk() {
        long offset = diskFile.tellp();
        for (size_t i = 0; i < keyBuffer.size(); ++i) {
            const std::string& key = keyBuffer[i];
            const std::string& value = valueBuffer[i];

            diskFile.write(value.data(), value.size());
            index[key] = KVNode(key, offset, value.size(), nullptr, false, std::time(nullptr));
            offset += value.size();

            std::cout << "Flushed " << key << " to disk at offset " << offset << " with length " << value.size() << std::endl;
        }
        keyBuffer.clear();
        valueBuffer.clear();
    }

    std::string readFromDisk(long offset, size_t length) {
        std::ifstream diskFileRead(disk_filename, std::ios::binary);
        if (!diskFileRead.is_open()) {
            throw std::runtime_error("无法打开磁盘文件进行读取");
        }
        diskFileRead.seekg(offset);
        std::string value(length, '\0');
        diskFileRead.read(&value[0], length);
        diskFileRead.close();
        std::cout << "Read value from disk at offset " << offset << " with length " << length << ": " << value << std::endl;
        return value;
    }

    std::string read(const std::string& key) {
        if (index.find(key) != index.end()) {
            KVNode& node = index[key];
            if (node.in_memory && node.memory_address != nullptr) {
                return std::string(static_cast<char*>(node.memory_address), node.length);
            } else {
                return readFromDisk(node.offset, node.length);
            }
        }
        return "";
    }

    bool remove(const std::string& key) {
        return index.erase(key) > 0;
    }
};


int main() {
    KVStore kvStore(2, "disk_data.bin");

    std::cout << "Testing write operations..." << std::endl;
    kvStore.write("key1", "value1");
    kvStore.write("key2", "value2"); 
    kvStore.write("key3", "value3"); 

    kvStore.flushBuffersToDisk();

    std::cout << "Testing read operations..." << std::endl;
    std::string value1 = kvStore.read("key1");
    std::cout << "Read key1, found value: " << value1 << std::endl;
    
    assert(value1 == "value1");

    std::string value2 = kvStore.read("key2");
    assert(value2 == "value2");
    std::cout << "Read key2, found value: " << value2 << std::endl;

    std::string value3 = kvStore.read("key3");
    assert(value3 == "value3");
    std::cout << "Read key3, found value: " << value3 << std::endl;

    std::cout << "Testing delete operation..." << std::endl;
    bool deleteResult = kvStore.remove("key2");
    assert(deleteResult == true);
    std::cout << "Deleted key2 successfully." << std::endl;

    std::string deletedValue = kvStore.read("key2");
    assert(deletedValue.empty());
    std::cout << "Key2 is no longer in the store." << std::endl;

    std::remove("disk_data.bin");

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
