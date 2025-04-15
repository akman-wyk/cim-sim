//
// Created by wyk on 2024/7/4.
//

#include "util.h"

#include <iostream>

namespace cimsim {

int BytesToInt(const std::vector<unsigned char>& bytes, bool little_endian) {
    unsigned int result = 0;
    if (little_endian) {
        for (int i = 3; i >= 0; i--) {
            result = (result << 8) + bytes[i];
        }
    } else {
        for (int i = 0; i <= 3; i++) {
            result = (result << 8) + bytes[i];
        }
    }
    return static_cast<int>(result);
}

std::vector<unsigned char> IntToBytes(int value, bool little_endian) {
    std::vector<unsigned char> bytes{0, 0, 0, 0};
    auto un_value = static_cast<unsigned int>(value);
    if (little_endian) {
        for (int i = 0; i <= 3; i++) {
            bytes[i] = (un_value & 0xff);
            un_value >>= 8;
        }
    } else {
        for (int i = 3; i >= 0; i--) {
            bytes[i] = (un_value & 0xff);
            un_value >>= 8;
        }
    }
    return std::move(bytes);
}

bool check_text_file_same(const std::string& file1, const std::string& file2) {
    if (std::ifstream in1(file1), in2(file2); in1 && in2) {
        std::string line1, line2;
        int line_num = 1;
        std::getline(in1, line1);
        std::getline(in2, line2);
        while (in1 && in2) {
            if (line1 != line2) {
                std::cerr << "Not same at line: " << line_num << std::endl;
                return false;
            }
            std::getline(in1, line1);
            std::getline(in2, line2);
            line_num++;
        }
        if (in1 || in2) {
            std::cerr << "files do not have same lines" << std::endl;
            return false;
        }
        return true;
    } else {
        std::cerr << "files do not exist" << std::endl;
        return false;
    }
}

std::string getDuplicateMemoryName(const std::string& original_name, int duplicate_id) {
    if (duplicate_id == 0) {
        return original_name;
    }
    return original_name + DUPLICATE_MEMORY_NAME_DELIMITER + std::to_string(duplicate_id);
}

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr) {
    out << arr[0];
    for (int i = 1; i < arr.size(); i++) {
        out << ", " << arr[i];
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::unordered_map<int, int>& map) {
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin()) {
            out << ", ";
        }
        out << it->first << ": " << it->second;
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::vector<int>& ins_id_list) {
    out << "[";
    for (int i = 0; i < ins_id_list.size(); i++) {
        if (i > 0) {
            out << ", ";
        }
        out << ins_id_list[i];
    }
    out << "]";
    return out;
}

std::string splitAndGetLastPart(const std::string& original_str, const std::string& delimiter) {
    size_t pos = 0;
    std::vector<std::string> parts;
    std::string str = original_str;

    // 使用 stringstream 分割字符串
    while ((pos = str.find(delimiter)) != std::string::npos) {
        parts.push_back(str.substr(0, pos));
        str.erase(0, pos + delimiter.length());
    }
    parts.push_back(str);  // 添加最后一部分

    // 返回最后一部分
    return parts.back();
}

}  // namespace cimsim
