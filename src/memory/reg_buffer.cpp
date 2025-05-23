//
// Created by wyk on 2024/7/4.
//

#include "reg_buffer.h"

#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

RegBuffer::RegBuffer(const sc_module_name &name, const std::string &mem_name, const cimsim::RegBufferConfig &config,
                     const BaseInfo &base_info)
    : MemoryHardware(name, base_info), config_(config), mem_name_(mem_name) {
    if (data_mode_ == +DataMode::real_data) {
        initialData();
    }

    energy_counter_.setStaticPowerMW(config_.static_power_mW);
    energy_counter_.addSubEnergyCounter("read", &read_energy_counter_);
    energy_counter_.addSubEnergyCounter("write", &write_energy_counter_);
}

sc_time RegBuffer::accessAndGetDelay(cimsim::MemoryAccessPayload &payload) {
    if (payload.address_byte < 0 || payload.address_byte + payload.size_byte > config_.size_byte) {
        std::cerr << fmt::format("Core id: {}, Invalid memory access with ins NO.'{}': address {} overflow, size: {}, "
                                 "config size: {}",
                                 core_id_, payload.ins.pc, payload.address_byte, payload.size_byte, config_.size_byte)
                  << std::endl;
        return {0.0, SC_NS};
    }

    if (payload.access_type == +MemoryAccessType::read) {
        int read_data_size_byte =
            (payload.size_byte <= config_.read_max_width_byte) ? payload.size_byte : config_.read_max_width_byte;
        int read_data_unit_cnt = IntDivCeil(read_data_size_byte, config_.rw_min_unit_byte);
        double read_dynamic_power_mW = config_.rw_dynamic_power_per_unit_mW * read_data_unit_cnt;
        read_energy_counter_.addDynamicEnergyPJ(period_ns_, read_dynamic_power_mW,
                                                {.core_id = core_id_,
                                                 .ins_id = payload.ins.ins_id,
                                                 .inst_opcode = payload.ins.inst_opcode,
                                                 .inst_group_tag = payload.ins.inst_group_tag,
                                                 .inst_profiler_operator = mem_name_ + "_read"});

        if (data_mode_ == +DataMode::real_data) {
            payload.data.resize(payload.size_byte);
            std::copy_n(data_.begin() + payload.address_byte, payload.size_byte, payload.data.begin());
        }

        return {0, SC_NS};
    } else {
        int write_data_size_byte =
            (payload.size_byte <= config_.write_max_width_byte) ? payload.size_byte : config_.write_max_width_byte;
        int write_data_unit_cnt = IntDivCeil(write_data_size_byte, config_.rw_min_unit_byte);
        double write_dynamic_power_mW = config_.rw_dynamic_power_per_unit_mW * write_data_unit_cnt;
        write_energy_counter_.addDynamicEnergyPJ(period_ns_, write_dynamic_power_mW,
                                                 {.core_id = core_id_,
                                                  .ins_id = payload.ins.ins_id,
                                                  .inst_opcode = payload.ins.inst_opcode,
                                                  .inst_group_tag = payload.ins.inst_group_tag,
                                                  .inst_profiler_operator = mem_name_ + "write"});

        if (data_mode_ == +DataMode::real_data) {
            std::copy(payload.data.begin(), payload.data.end(), data_.begin() + payload.address_byte);
        }

        return {period_ns_, SC_NS};
    }
}

void RegBuffer::initialData() {
    data_ = std::vector<uint8_t>(config_.size_byte, 0);
    if (config_.has_image) {
        std::ifstream ifs;
        ifs.open(config_.image_file, std::ios::in | std::ios::binary);
        ifs.read(reinterpret_cast<char *>(data_.data()), config_.size_byte);
        ifs.close();
    }
}

int RegBuffer::getMemoryDataWidthByte(MemoryAccessType access_type) const {
    return access_type == +MemoryAccessType::read ? config_.read_max_width_byte : config_.write_max_width_byte;
}

int RegBuffer::getMemorySizeByte() const {
    return config_.size_byte;
}

}  // namespace cimsim
