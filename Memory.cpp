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
    std::vector<std::string> keyBuffer;   // Keyд������
    std::vector<std::string> valueBuffer; // Valueд������
    size_t bufferLimit;                   // �������Ĵ�С����
    std::ofstream diskFile;               // ģ��Ĵ����ļ�
    const size_t keyRegionSize = 100 * 1024 * 1024; // 100 MB��key�����С

public:
    KVStore(size_t buffer_limit, const std::string& filename) : bufferLimit(buffer_limit) {
        diskFile.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!diskFile.is_open()) {
            throw std::runtime_error("�޷��򿪴����ļ�");
        }
    }

    ~KVStore() {
        if (diskFile.is_open()) {
            flushBuffersToDisk();  // ������ʱ��ջ�����������
            diskFile.close();
        }
    }

    // ���л���������key��value�ֱ����л�Ϊ������
    std::string serializeKey(const std::string& key) {
        std::ostringstream oss;
        size_t key_length = key.size();
        oss.write(reinterpret_cast<const char*>(&key_length), sizeof(key_length));  // д��key�ĳ���
        oss.write(key.data(), key_length);  // д��key����
        return oss.str();
    }

    std::string serializeValue(const std::string& value) {
        std::ostringstream oss;
        size_t value_length = value.size();
        oss.write(reinterpret_cast<const char*>(&value_length), sizeof(value_length));  // д��value�ĳ���
        oss.write(value.data(), value_length);  // д��value����
        return oss.str();
    }

    // д������������л���Key��Value�ֱ�д�뻺����
    bool write(const std::string& key, const std::string& value) {
        // ���л�key��value
        std::string serialized_key = serializeKey(key);
        std::string serialized_value = serializeValue(value);

        // д�뻺����
        keyBuffer.push_back(serialized_key);
        valueBuffer.push_back(serialized_value);

        // ��黺�����Ƿ�����
        if (keyBuffer.size() >= bufferLimit || valueBuffer.size() >= bufferLimit) {
            flushBuffersToDisk();
        }
        return true;
    }

    // ����������д�����
    void flushBuffersToDisk() {
        long key_offset = 0;                 // key���ִ��ļ��Ŀ�ʼλ��д��
        long value_offset = keyRegionSize;   // value���ִ�100 MB֮��д��

        // д�����л���������
        for (size_t i = 0; i < keyBuffer.size(); ++i) {
            // д��key��key����
            diskFile.seekp(key_offset, std::ios::beg);
            diskFile.write(keyBuffer[i].data(), keyBuffer[i].size());

            // д��value��value����
            diskFile.seekp(value_offset, std::ios::beg);
            diskFile.write(valueBuffer[i].data(), valueBuffer[i].size());

            // ��ȡkey��value�ĳ��ȣ����ڸ���������Ϣ
            std::string key = deserializeKey(keyBuffer[i]);
            size_t value_length = valueBuffer[i].size() - sizeof(size_t); // value���ݳ���

            // ��������
            index[key] = KVNode(key, value_offset, value_length, nullptr, false, std::time(nullptr));

            // ����ƫ����
            key_offset += keyBuffer[i].size();
            value_offset += valueBuffer[i].size();
        }

        // ��ջ�����
        keyBuffer.clear();
        valueBuffer.clear();
    }

    // �����л���������������ȡ��ֵ
    std::string deserializeKey(const std::string& serialized_data) {
        std::istringstream iss(serialized_data);
        size_t key_length;
        iss.read(reinterpret_cast<char*>(&key_length), sizeof(key_length));

        std::string key(key_length, '\0');
        iss.read(&key[0], key_length);
        return key;
    }

    // ��ȡ����
    KVNode* read(const std::string& key) {
        if (index.find(key) != index.end()) {
            return &index[key];
        }
        return nullptr;
    }

    // ɾ������
    bool remove(const std::string& key) {
        return index.erase(key) > 0;
    }
};

int main() {
    // ���û�������С����Ϊ2�������ļ�Ϊ"disk_data.bin"
    KVStore kvStore(2, "disk_data.bin");

    // ����д��
    std::cout << "Testing write operations..." << std::endl;
    kvStore.write("key1", "value1");
    kvStore.write("key2", "value2");  // �ﵽ���������ޣ�Ӧ����д�뵽����
    kvStore.write("key3", "value3");  // �ٴ�д��һ����ֵ��

    kvStore.flushBuffersToDisk();  // ��Ӵ���
    // ���Զ�ȡ
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

    // ����ɾ��
    std::cout << "Testing delete operation..." << std::endl;
    bool deleteResult = kvStore.remove("key2");
    assert(deleteResult == true);
    std::cout << "Deleted key2 successfully." << std::endl;

    KVNode* deletedNode = kvStore.read("key2");
    assert(deletedNode == nullptr);
    std::cout << "Key2 is no longer in the store." << std::endl;

    // ��������ļ�
    std::remove("disk_data.bin");

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
