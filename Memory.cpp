#include <unordered_map>
#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>

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
    std::vector<std::string> keyBuffer;   // Key写缓冲区
    std::vector<std::string> valueBuffer; // Value写缓冲区
    size_t bufferLimit;                   // 缓冲区的大小限制
    std::ofstream diskFile;               // 模拟的磁盘文件
    const size_t keyRegionSize = 100 * 1024 * 1024; // 100 MB的key区域大小

public:
    KVStore(size_t buffer_limit, const std::string& filename) : bufferLimit(buffer_limit) {
        diskFile.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!diskFile.is_open()) {
            throw std::runtime_error("无法打开磁盘文件");
        }
    }

    ~KVStore() {
        if (diskFile.is_open()) {
            flushBuffersToDisk();  // 在析构时清空缓冲区到磁盘
            diskFile.close();
        }
    }

    // 序列化函数，将key和value分别序列化为二进制
    std::string serializeKey(const std::string& key) {
        std::ostringstream oss;
        size_t key_length = key.size();
        oss.write(reinterpret_cast<const char*>(&key_length), sizeof(key_length));  // 写入key的长度
        oss.write(key.data(), key_length);  // 写入key内容
        return oss.str();
    }

    std::string serializeValue(const std::string& value) {
        std::ostringstream oss;
        size_t value_length = value.size();
        oss.write(reinterpret_cast<const char*>(&value_length), sizeof(value_length));  // 写入value的长度
        oss.write(value.data(), value_length);  // 写入value内容
        return oss.str();
    }

    // 写入操作，将序列化的Key和Value分别写入缓冲区
    bool write(const std::string& key, const std::string& value) {
        // 序列化key和value
        std::string serialized_key = serializeKey(key);
        std::string serialized_value = serializeValue(value);

        // 写入缓冲区
        keyBuffer.push_back(serialized_key);
        valueBuffer.push_back(serialized_value);

        // 检查缓冲区是否已满
        if (keyBuffer.size() >= bufferLimit || valueBuffer.size() >= bufferLimit) {
            flushBuffersToDisk();
        }
        return true;
    }

    // 缓冲区数据写入磁盘
    void flushBuffersToDisk() {
        long key_offset = 0;                 // key部分从文件的开始位置写入
        long value_offset = keyRegionSize;   // value部分从100 MB之后写入

        // 写入所有缓冲区内容
        for (size_t i = 0; i < keyBuffer.size(); ++i) {
            // 写入key到key区域
            diskFile.seekp(key_offset, std::ios::beg);
            diskFile.write(keyBuffer[i].data(), keyBuffer[i].size());

            // 写入value到value区域
            diskFile.seekp(value_offset, std::ios::beg);
            diskFile.write(valueBuffer[i].data(), valueBuffer[i].size());

            // 获取key和value的长度，用于更新索引信息
            std::string key = deserializeKey(keyBuffer[i]);
            size_t value_length = valueBuffer[i].size() - sizeof(size_t); // value数据长度

            // 更新索引
            index[key] = KVNode(key, value_offset, value_length, nullptr, false, std::time(nullptr));

            // 更新偏移量
            key_offset += keyBuffer[i].size();
            value_offset += valueBuffer[i].size();
        }

        // 清空缓冲区
        keyBuffer.clear();
        valueBuffer.clear();
    }

    // 反序列化函数，仅用于提取键值
    std::string deserializeKey(const std::string& serialized_data) {
        std::istringstream iss(serialized_data);
        size_t key_length;
        iss.read(reinterpret_cast<char*>(&key_length), sizeof(key_length));

        std::string key(key_length, '\0');
        iss.read(&key[0], key_length);
        return key;
    }

    // 读取操作
    KVNode* read(const std::string& key) {
        if (index.find(key) != index.end()) {
            return &index[key];
        }
        return nullptr;
    }

    // 删除操作
    bool remove(const std::string& key) {
        return index.erase(key) > 0;
    }
};

int main() {
    // 设置缓冲区大小限制为2，磁盘文件为"disk_data.bin"
    KVStore kvStore(2, "disk_data.bin");

    // 测试写入
    std::cout << "Testing write operations..." << std::endl;
    kvStore.write("key1", "value1");
    kvStore.write("key2", "value2");  // 达到缓冲区上限，应触发写入到磁盘
    kvStore.write("key3", "value3");  // 再次写入一个键值对

    kvStore.flushBuffersToDisk();  // 添加此行
    // 测试读取
    std::cout << "Testing read operations..." << std::endl;
    KVNode* node1 = kvStore.read("key1");
    assert(node1 != nullptr && node1->length > 0);
    std::cout << "Read key1, found length: " << node1->length << std::endl;

    KVNode* node2 = kvStore.read("key2");
    assert(node2 != nullptr && node2->length > 0);
    std::cout << "Read key2, found length: " << node2->length << std::endl;

    KVNode* node3 = kvStore.read("key3");
    assert(node3 != nullptr && node3->length > 0);
    std::cout << "Read key3, found length: " << node3->length << std::endl;

    // 测试删除
    std::cout << "Testing delete operation..." << std::endl;
    bool deleteResult = kvStore.remove("key2");
    assert(deleteResult == true);
    std::cout << "Deleted key2 successfully." << std::endl;

    KVNode* deletedNode = kvStore.read("key2");
    assert(deletedNode == nullptr);
    std::cout << "Key2 is no longer in the store." << std::endl;

    // 清理磁盘文件
    std::remove("disk_data.bin");

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
