#include <unordered_map>
#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>

struct KVNode {
    int key;              //�ؼ������͸�Ϊint
    long offset;          //���ļ��е�ƫ����
    size_t length;        //����
    void* memory_address; //�ڴ��ַ
    bool in_memory;       //�Ƿ����ڴ���
    std::time_t timestamp; //ʱ���

    KVNode(int key, long offset, size_t length, void* memory_address, bool in_memory, std::time_t timestamp)
        : key(key), offset(offset), length(length), memory_address(memory_address), in_memory(in_memory), timestamp(timestamp) {}

    KVNode() : key(0), offset(0), length(0), memory_address(nullptr), in_memory(false), timestamp(0) {}
};

class KVStore {

private:
    std::unordered_map<int, KVNode> HashMap;//�ڵ��ϣ��
    std::vector<std::string> keyBuffer;
    std::vector<std::string> valueBuffer;
    size_t bufferLimit; //��������С
    std::ofstream diskFile;
    std::string disk_filename;

public:
    KVStore(size_t buffer_limit, const std::string& disk_filename) : bufferLimit(buffer_limit), disk_filename(disk_filename) {
        diskFile.open(disk_filename, std::ios::app | std::ios::binary);
        if (!diskFile.is_open()) {
            throw std::runtime_error("�޷��򿪴����ļ�");
        }
    }

    ~KVStore() {
        if (diskFile.is_open()) {
            // flushBuffersToDisk();
            diskFile.close();
        }
    }

    std::string serializeKey(int key) {
        std::ostringstream oss;
        oss.write(reinterpret_cast<const char*>(&key), sizeof(key));  // д��key��ֵ
        return oss.str();
    }

    int deserializeKey(const std::string& binaryData) {
        int key;
        std::istringstream iss(binaryData);
        iss.read(reinterpret_cast<char*>(&key), sizeof(key));  // �����ж�ȡ���������ݵ�key
        return key;
}

    template<typename T>
    std::string serializeValue(const T& value) {
        std::ostringstream oss;
        oss.write(reinterpret_cast<const char*>(&value), sizeof(T));  // д��value������
        return oss.str();
    }

    template<typename T>
    T deserializeValue(const std::string& binaryData) {
        T value;
        std::istringstream iss(binaryData);
        iss.read(reinterpret_cast<char*>(&value), sizeof(T));  // �����ж�ȡ���������ݵ�value
        return value;
    }


    // bool write(const std::string& key, const std::string& value) {
    //     keyBuffer.push_back(key);
    //     valueBuffer.push_back(value);

    //     if (keyBuffer.size() >= bufferLimit) {
    //         flushBuffersToDisk();
    //     }
    //     return true;
    // }

    // void flushBuffersToDisk() {
    //     long offset = diskFile.tellp();
    //     for (size_t i = 0; i < keyBuffer.size(); ++i) {
    //         const std::string& key = keyBuffer[i];
    //         const std::string& value = valueBuffer[i];

    //         diskFile.write(value.data(), value.size());
    //         HashMap[key] = KVNode(key, offset, value.size(), nullptr, false, std::time(nullptr));
    //         offset += value.size();

    //         std::cout << "Flushed " << key << " to disk at offset " << offset << " with length " << value.size() << std::endl;
    //     }
    //     keyBuffer.clear();
    //     valueBuffer.clear();
    // }

    std::string readFromDisk(long offset, size_t length) {
        std::ifstream diskFileRead(disk_filename, std::ios::binary);
        if (!diskFileRead.is_open()) {
            throw std::runtime_error("�޷��򿪴����ļ����ж�ȡ");
        }
        diskFileRead.seekg(offset);
        std::string value(length, '\0');
        diskFileRead.read(&value[0], length);
        diskFileRead.close();
        std::cout << "Read value from disk at offset " << offset << " with length " << length << ": " << value << std::endl;
        return value;
    }

    // std::string read(const std::string& key) {
    //     if (HashMap.find(key) != HashMap.end()) {
    //         KVNode& node = HashMap[key];
    //         if (node.in_memory && node.memory_address != nullptr) {
    //             return std::string(static_cast<char*>(node.memory_address), node.length);
    //         } else {
    //             return readFromDisk(node.offset, node.length);
    //         }
    //     }
    //     return "";
    // }

    // bool remove(const std::string& key) {
    //     return HashMap.erase(key) > 0;
    // }
};


int main() {
    KVStore kvStore(2, "disk_data.bin");

    std::cout << "Testing write operations..." << std::endl;

    std::string value1 = "test";
    std::string value = kvStore.serializeValue(value1);
    std::cout << "value: " << kvStore.deserializeValue<std::string>(value) << std::endl;
    // kvStore.write("key1", "value1");

    return 0;
}
