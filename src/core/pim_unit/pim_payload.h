//
// Created by wyk on 2024/7/20.
//

#pragma once

#include <vector>

#include "better-enums/enum.h"

namespace pimsim {

struct PimInsInfo {
    int ins_pc{-1}, sub_ins_num{-1};
    bool last_ins{false}, last_sub_ins{false};
    int ins_id{-1};
};

struct MacroPayload {
    PimInsInfo pim_ins_info{};

    int row{0};
    int input_bit_width{0};
    bool bit_sparse{false};

    std::vector<unsigned long long> inputs{};

    int total_activation_group_cnt;
    int total_activation_macro_cnt;
};

struct MacroSubInsInfo {
    PimInsInfo pim_ins_info{};
    int compartment_num{0};
    bool bit_sparse{false};

    int activation_element_col_cnt{0};
    std::vector<unsigned char> activation_element_col_mask{};

    int total_activation_group_cnt;
    int total_activation_macro_cnt;

};

struct MacroBatchInfo {
    int batch_num{0};
    bool first_batch{false};
    bool last_batch{false};
};

struct MacroSubmodulePayload {
    MacroSubInsInfo sub_ins_info;
    MacroBatchInfo batch_info;
};

struct MacroGroupPayload {
    PimInsInfo pim_ins_info{};

    // group info
    bool last_group{false};

    // macro compute info
    int row{0};
    int input_bit_width{0};
    bool bit_sparse{false};

    // inputs
    std::vector<std::vector<unsigned long long>> macro_inputs{};

    // control
    int total_activation_group_cnt;
    int total_activation_macro_cnt;
};

struct MacroGroupControllerPayload {
    PimInsInfo pim_ins_info{};

    // group info
    bool last_group{false};

    // macro compute info
    int input_bit_width{0};
    bool bit_sparse{false};
};

struct MacroGroupSubInsInfo {
    // ins info and sub ins info
    PimInsInfo pim_ins_info{};

    // group info
    bool last_group{false};

    // macro compute info
    bool bit_sparse{false};
};

struct MacroGroupSubmodulePayload {
    MacroGroupSubInsInfo sub_ins_info{};
    MacroBatchInfo batch_info{};
};

}  // namespace pimsim
