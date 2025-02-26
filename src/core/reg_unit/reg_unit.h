//
// Created by wyk on 2024/7/19.
//

#pragma once
#include <array>
#include <unordered_map>
#include <vector>

#include "base_component/base_module.h"
#include "base_component/register.h"
#include "config/config.h"
#include "core/payload/payload.h"

namespace pimsim {

BETTER_ENUM(SpecialRegId, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            pim_input_bit_width = 0, pim_output_bit_width = 1, pimm_weight_bit_width = 2, group_size = 3,
            activation_group_num = 4, activation_element_col_num = 5, group_input_step = 6, value_sparse_mask_addr = 7,
            bit_sparse_meta_addr = 8,

            simd_input_1_bit_width = 16, simd_input_2_bit_width = 17, simd_input_3_bit_width = 18,
            simd_input_4_bit_width = 19, simd_output_bit_width = 20, input_3_address = 21, input_4_address = 22)

class RegUnit : public BaseModule {
public:
    SC_HAS_PROCESS(RegUnit);

    RegUnit(const char* name, const RegisterUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    int getSpecialBoundGeneralId(int special_id) const;

    void writeRegister(const RegUnitWritePayload& write_req);

    int readRegister(int id, bool special = false) const;

    bool checkRegValues(const std::array<int, GENERAL_REG_NUM>& general_reg_expected_values,
                        const std::array<int, SPECIAL_REG_NUM>& special_reg_expected_values);

    std::string getGeneralRegistersString() const;

private:
    const RegisterUnitConfig& config_;
    std::unordered_map<int, int> special_bind_map_{};

    std::array<int, GENERAL_REG_NUM> general_regs_{};
    std::array<int, SPECIAL_REG_NUM> special_regs_{};
};

}  // namespace pimsim
