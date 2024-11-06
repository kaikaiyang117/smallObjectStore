#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <list>
#include <mutex>

class ObjectStorage {
public:
    // ���캯������������
    ObjectStorage(const std::string& filename, size_t cacheSize);
    
    ~ObjectStorage();

    // �������
    void put(int key, const std::vector<char>& value);

    // ��ȡ����
    std::vector<char> get(int key);

    // ɾ������
    void del(int key);

    // �����ã���ӡ��������
    void printCache();

private:
    struct MetaDataEntry {
        int key;             // ����Key
        uint64_t offset;     // ��������ƫ����
        uint32_t size;       // �����С
    };

    // LRU������
    class LRUCache {
    private:
        size_t capacity;  // ��������
        std::list<std::pair<int, std::vector<char>>> itemList;  // ˫��������¼����˳��
        std::unordered_map<int, decltype(itemList.begin())> itemMap;  // ��ϣ�����ٲ���
        std::mutex cacheMutex;  // ���ڶ��߳�ͬ��

    public:
        LRUCache(size_t cap) : capacity(cap) {}

        std::vector<char> get(int key);
        void put(int key, const std::vector<char>& value);
        void print();
    };

    std::fstream dataFile;
    std::unordered_map<int, MetaDataEntry> metadataMap;
    LRUCache cache;
};

// ObjectStorage ��ʵ��
ObjectStorage::ObjectStorage(const std::string& filename, size_t cacheSize) : cache(cacheSize) {
    dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!dataFile) {
        dataFile.open(filename, std::ios::out | std::ios::binary);
        dataFile.close();
        dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }
}

ObjectStorage::~ObjectStorage() {
    if (dataFile.is_open()) {
        dataFile.close();
    }
}

void ObjectStorage::put(int key, const std::vector<char>& value) {
    cache.put(key, value);

    dataFile.seekp(0, std::ios::end);
    uint64_t offset = dataFile.tellp();
    uint32_t size = value.size();
    dataFile.write(value.data(), size);

    MetaDataEntry entry = {key, offset, size};
    metadataMap[key] = entry;
}

std::vector<char> ObjectStorage::get(int key) {
    std::vector<char> data = cache.get(key);
    if (!data.empty()) return data;

    if (metadataMap.find(key) == metadataMap.end()) return {};
    MetaDataEntry entry = metadataMap[key];

    data.resize(entry.size);
    dataFile.seekg(entry.offset);
    dataFile.read(data.data(), entry.size);

    cache.put(key, data);
    return data;
}

void ObjectStorage::del(int key) {
    cache.put(key, {});
    metadataMap.erase(key);
}

void ObjectStorage::printCache() {
    cache.print();
}

// LRUCache ��ʵ��
std::vector<char> ObjectStorage::LRUCache::get(int key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (itemMap.find(key) == itemMap.end()) {
        return {};  // ��������в����ڸ�����ؿ�ֵ
    }

    itemList.splice(itemList.begin(), itemList, itemMap[key]);
    return itemMap[key]->second;
}

void ObjectStorage::LRUCache::put(int key, const std::vector<char>& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    if (itemMap.find(key) != itemMap.end()) {
        itemList.splice(itemList.begin(), itemList, itemMap[key]);
        itemMap[key]->second = value;
        return;
    }

    if (itemList.size() >= capacity) {
        auto last = itemList.back();
        itemMap.erase(last.first);
        itemList.pop_back();
    }

    itemList.emplace_front(key, value);
    itemMap[key] = itemList.begin();
}

void ObjectStorage::LRUCache::print() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    for (const auto& pair : itemList) {
        std::cout << pair.first << ":" << pair.second.size() << " ";
    }
    std::cout << std::endl;
}

// ���Դ���
int main() {
    ObjectStorage storage("datafile.dat", 3);  // ���û�������Ϊ3

    // ��������
    std::vector<char> data1 = {'H', 'e', 'l', 'l', 'o'};
    storage.put(1, data1);

    std::vector<char> data2 = {'W', 'o', 'r', 'l', 'd'};
    storage.put(2, data2);

    std::vector<char> data3 = {'F', 'o', 'o'};
    storage.put(3, data3);

    // ��ӡ��ǰ����
    std::cout << "Initial cache state: ";
    storage.printCache();

    // ��ȡ�����е�����
    std::cout << "Get key 2: ";
    auto result = storage.get(2);
    for (char c : result) {
        std::cout << c;
    }
    std::cout << std::endl;

    // ���������ݣ��������δʹ����Ƴ�
    std::vector<char> data4 = {'B', 'a', 'r'};
    storage.put(4, data4);
    std::cout << "Cache after inserting key 4: ";
    storage.printCache();

    // ���Ի�ȡ���Ƴ�����
    std::cout << "Get key 1: ";
    result = storage.get(1);
    if (result.empty()) {
        std::cout << "Not found" << std::endl;
    } else {
        for (char c : result) {
            std::cout << c;
        }
    }
    std::cout << std::endl;

    return 0;
}
