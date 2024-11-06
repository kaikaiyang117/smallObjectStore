#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <list>
#include <mutex>

class ObjectStorage {
public:
    // 构造函数和析构函数
    ObjectStorage(const std::string& filename, size_t cacheSize);
    
    ~ObjectStorage();

    // 插入对象
    void put(int key, const std::vector<char>& value);

    // 获取对象
    std::vector<char> get(int key);

    // 删除对象
    void del(int key);

    // 调试用，打印缓存内容
    void printCache();

private:
    struct MetaDataEntry {
        int key;             // 对象Key
        uint64_t offset;     // 数据区的偏移量
        uint32_t size;       // 对象大小
    };

    // LRU缓存类
    class LRUCache {
    private:
        size_t capacity;  // 缓存容量
        std::list<std::pair<int, std::vector<char>>> itemList;  // 双向链表，记录缓存顺序
        std::unordered_map<int, decltype(itemList.begin())> itemMap;  // 哈希表，快速查找
        std::mutex cacheMutex;  // 用于多线程同步

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

// ObjectStorage 类实现
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

// LRUCache 类实现
std::vector<char> ObjectStorage::LRUCache::get(int key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (itemMap.find(key) == itemMap.end()) {
        return {};  // 如果缓存中不存在该项，返回空值
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

// 测试代码
int main() {
    ObjectStorage storage("datafile.dat", 3);  // 设置缓存容量为3

    // 插入数据
    std::vector<char> data1 = {'H', 'e', 'l', 'l', 'o'};
    storage.put(1, data1);

    std::vector<char> data2 = {'W', 'o', 'r', 'l', 'd'};
    storage.put(2, data2);

    std::vector<char> data3 = {'F', 'o', 'o'};
    storage.put(3, data3);

    // 打印当前缓存
    std::cout << "Initial cache state: ";
    storage.printCache();

    // 获取缓存中的数据
    std::cout << "Get key 2: ";
    auto result = storage.get(2);
    for (char c : result) {
        std::cout << c;
    }
    std::cout << std::endl;

    // 插入新数据，导致最久未使用项被移除
    std::vector<char> data4 = {'B', 'a', 'r'};
    storage.put(4, data4);
    std::cout << "Cache after inserting key 4: ";
    storage.printCache();

    // 尝试获取被移除的项
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
