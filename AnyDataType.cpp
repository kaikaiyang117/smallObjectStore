#include "Data.pb.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <type_traits>

// 序列化函数
std::string serializeData(const AnyDataType& message) {
    std::string serialized_data;
    if (!message.SerializeToString(&serialized_data)) {
        throw std::runtime_error("Failed to serialize data.");
    }
    return serialized_data;
}

// 反序列化函数
AnyDataType deserializeData(const std::string& serialized_data) {
    AnyDataType message;
    if (!message.ParseFromString(serialized_data)) {
        throw std::runtime_error("Failed to deserialize data.");
    }
    return message;
}


// template<typename T>
// AnyDataType wrapData(const T& data) {
//     AnyDataType message;    
//     if (std::is_same<T, int32_t>::value) {
//         message.set_int_value(data);
//     } else if (std::is_same<T, int64_t>::value) {
//         message.set_int64_value(data);
//     } else if (std::is_same<T, uint32_t>::value) {
//         message.set_uint32_value(data);
//     } else if (std::is_same<T, uint64_t>::value) {
//         message.set_uint64_value(data);
//     } else if (std::is_same<T, float>::value) {
//         message.set_float_value(data);
//     } else if (std::is_same<T, double>::value) {
//         message.set_double_value(data);
//     } else if (std::is_same<T, bool>::value) {
//         message.set_bool_value(data);
//     } else if (std::is_same<T, std::string>::value) {
//         message.set_string_value(data);
//     } else {
//         throw std::invalid_argument("Unsupported data type.");
//     }
//     return message;
// }

// 从AnyDataType消息中提取数据
// void unwrapData(const AnyDataType& message) {
//     if (message.has_int_value()) {
//         std::cout << "Int value: " << message.int_value() << std::endl;
//     } else if (message.has_int64_value()) {
//         std::cout << "Int64 value: " << message.int64_value() << std::endl;
//     } else if (message.has_uint32_value()) {
//         std::cout << "Uint32 value: " << message.uint32_value() << std::endl;
//     } else if (message.has_uint64_value()) {
//         std::cout << "Uint64 value: " << message.uint64_value() << std::endl;
//     } else if (message.has_float_value()) {
//         std::cout << "Float value: " << message.float_value() << std::endl;
//     } else if (message.has_double_value()) {
//         std::cout << "Double value: " << message.double_value() << std::endl;
//     } else if (message.has_bool_value()) {
//         std::cout << "Bool value: " << message.bool_value() << std::endl;
//     } else if (message.has_string_value()) {
//         std::cout << "String value: " << message.string_value() << std::endl;
//     } else {
//         std::cout << "No data found in message." << std::endl;
//     }
// }

int main() {
    // 测试 int64 类型
    int32_t int32_data = 901234LL;
    std::cout << "test" << std::endl;
    AnyDataType int64_message = wrapData(int32_data);
    // std::string serialized_data = serializeData(int64_message);

    // AnyDataType deserialized_message = deserializeData(serialized_data);
    // unwrapData(deserialized_message);
}
