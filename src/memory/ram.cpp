//
// Created by wyk on 2024/7/4.
//

#include "ram.h"

#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

RAM::RAM(const sc_module_name &name, const std::string &mem_name, const RAMConfig &config, const BaseInfo &base_info)
    : MemoryHardware(name, base_info), config_(config), mem_name_(mem_name) {
    if (data_mode_ == +DataMode::real_data) {
        initialData();
    }

    energy_counter_.setStaticPowerMW(config_.static_power_mW);
    energy_counter_.addSubEnergyCounter("read", &read_energy_counter_);
    energy_counter_.addSubEnergyCounter("write", &write_energy_counter_);
}

sc_time RAM::accessAndGetDelay(cimsim::MemoryAccessPayload &payload) {
    if (payload.address_byte < 0 || payload.address_byte + payload.size_byte > config_.size_byte) {
        std::cerr << fmt::format("Core id: {}, Invalid memory access with ins NO.'{}': address {} overflow, size: {}, "
                                 "config size: {}",
                                 core_id_, payload.ins.pc, payload.address_byte, payload.size_byte, config_.size_byte)
                  << std::endl;
        return {0.0, SC_NS};
    }

    int process_times = IntDivCeil(payload.size_byte, config_.width_byte);
    double latency;
    if (payload.access_type == +MemoryAccessType::read) {
        latency = process_times * config_.read_latency_cycle * period_ns_;
        read_energy_counter_.addDynamicEnergyPJ(latency, config_.read_dynamic_power_mW,
                                                {.core_id = core_id_,
                                                 .ins_id = payload.ins.ins_id,
                                                 .inst_opcode = payload.ins.inst_opcode,
                                                 .inst_group_tag = payload.ins.inst_group_tag,
                                                 .inst_profiler_operator = mem_name_ + "_read"});

        if (data_mode_ == +DataMode::real_data) {
            payload.data.resize(payload.size_byte);
            std::copy_n(data_.begin() + payload.address_byte, payload.size_byte, payload.data.begin());
        }
    } else {
        latency = process_times * config_.write_latency_cycle * period_ns_;
        write_energy_counter_.addDynamicEnergyPJ(latency, config_.write_dynamic_power_mW,
                                                 {.core_id = core_id_,
                                                  .ins_id = payload.ins.ins_id,
                                                  .inst_opcode = payload.ins.inst_opcode,
                                                  .inst_group_tag = payload.ins.inst_group_tag,
                                                  .inst_profiler_operator = mem_name_ + "_write"});

        if (data_mode_ == +DataMode::real_data) {
            std::copy(payload.data.begin(), payload.data.end(), data_.begin() + payload.address_byte);
        }
    }

    return {latency, SC_NS};
}

void RAM::initialData() {
    data_ = std::vector<uint8_t>(config_.size_byte, 0);
    if (config_.has_image) {
        std::ifstream ifs;
        ifs.open(config_.image_file, std::ios::in | std::ios::binary);
        ifs.read(reinterpret_cast<char *>(data_.data()), config_.size_byte);
        ifs.close();
    }
}

int RAM::getMemoryDataWidthByte(MemoryAccessType access_type) const {
    return config_.width_byte;
}

int RAM::getMemorySizeByte() const {
    return config_.size_byte;
}

}  // namespace cimsim
