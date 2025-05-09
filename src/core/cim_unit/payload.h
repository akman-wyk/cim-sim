//
// Created by wyk on 2024/7/20.
//

#pragma once

#include <memory>
#include <vector>

namespace cimsim {

struct CimInsInfo {
    int ins_pc{-1}, sub_ins_num{-1};
    bool last_sub_ins{false};
    int ins_id{-1};

    OPCODE inst_opcode{OPCODE::CIM_MVM};
    std::string_view inst_group_tag;
};

struct MacroPayload {
    CimInsInfo cim_ins_info{};

    int row{0};
    int input_bit_width{0};
    bool bit_sparse{false};

    std::vector<unsigned long long> inputs{};

    int simulated_group_cnt{1};
    int simulated_macro_cnt{1};
};

struct MacroSubInsInfo {
    CimInsInfo cim_ins_info{};
    int compartment_num{0};
    bool bit_sparse{false};

    int activation_element_col_cnt{0};
    int simulated_group_cnt{1};
    int simulated_macro_cnt{1};
};

struct MacroBatchInfo {
    int batch_num{0};
    bool last_batch{false};
};

struct MacroSubmodulePayload {
    std::shared_ptr<MacroSubInsInfo> sub_ins_info;
    std::shared_ptr<MacroBatchInfo> batch_info;
};

struct MacroGroupPayload {
    CimInsInfo cim_ins_info{};

    // group info
    bool last_group{false};

    // macro compute info
    int row{0};
    int input_bit_width{0};
    bool bit_sparse{false};

    // inputs
    std::vector<std::vector<unsigned long long>> macro_inputs{};

    // control
    int simulated_group_cnt{1};
    int simulated_macro_cnt{1};
};

struct MacroGroupControllerPayload {
    CimInsInfo cim_ins_info{};

    // group info
    bool last_group{false};

    // macro compute info
    int input_bit_width{0};
    bool bit_sparse{false};
};

struct MacroGroupSubInsInfo {
    // ins info and sub ins info
    CimInsInfo cim_ins_info{};

    // group info
    bool last_group{false};

    // macro compute info
    bool bit_sparse{false};
};

struct MacroGroupSubmodulePayload {
    std::shared_ptr<MacroGroupSubInsInfo> sub_ins_info{};
    std::shared_ptr<MacroBatchInfo> batch_info{};
};

}  // namespace cimsim
