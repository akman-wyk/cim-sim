//
// Created by wyk on 2024/8/19.
//

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

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

enum class FieldLength {
    OPCODE = 6,
    REG = 5,
    FUNCT_TYPE2 = 5,
    FUNCT_TYPE3 = 5,
    IMM_TYPE3 = 11,
    IMM_TYPE4 = 16,
    IMM_TYPE5 = 21,
    IMM_TYPE6 = 26,
};

int main(int argc, char* argv[]) {
    // std::string file1(argv[1]), file2(argv[2]);
    // if (check_text_file_same(file1, file2)) {
    //     std::cout << "Files same" << std::endl;
    // } else {
    //     std::cout << "Files do not same" << std::endl;
    // }

    auto a = static_cast<uint32_t>(FieldLength::IMM_TYPE6);
    std::cout << a << std::endl;
    return 0;
}
