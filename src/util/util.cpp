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

std::string getDuplicateMemoryName(const std::string& original_name, int index, int duplicate_cnt) {
    if (duplicate_cnt == 1) {
        return original_name;
    }
    return original_name + DUPLICATE_MEMORY_NAME_DELIMITER + std::to_string(index);
}

}  // namespace cimsim
