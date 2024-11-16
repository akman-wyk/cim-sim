//
// Created by wyk on 2024/7/4.
//

#include "reg_buffer.h"

#include "fmt/core.h"
#include "util/util.h"
#include "core/core.h"

namespace pimsim {

RegBuffer::RegBuffer(const char *name, const pimsim::RegBufferConfig &config, const pimsim::SimConfig &sim_config,
                     pimsim::Core *core, pimsim::Clock *clk)
    : MemoryHardware(name, sim_config, core, clk), config_(config) {
    if (data_mode_ == +DataMode::real_data) {
        initialData();
    }

    static_energy_counter_.setStaticPowerMW(config_.static_power_mW);
}

sc_core::sc_time RegBuffer::accessAndGetDelay(pimsim::MemoryAccessPayload &payload) {
    if (payload.address_byte < 0 || payload.address_byte + payload.size_byte > config_.size_byte) {
        std::cerr << fmt::format("Core id: {}, Invalid memory access with ins NO.'{}': address {} overflow, size: {}, config size: {}", 
                                core_->getCoreId(), payload.ins.pc, payload.address_byte, payload.size_byte, config_.size_byte)
                  << std::endl;
        return {0.0, sc_core::SC_NS};
    }

    if (payload.access_type == +MemoryAccessType::read) {
        int read_data_size_byte =
            (payload.size_byte <= config_.read_max_width_byte) ? payload.size_byte : config_.read_max_width_byte;
        int read_data_unit_cnt = IntDivCeil(read_data_size_byte, config_.rw_min_unit_byte);
        double read_dynamic_power_mW = config_.rw_dynamic_power_per_unit_mW * read_data_unit_cnt;
        read_energy_counter_.addDynamicEnergyPJ(period_ns_, read_dynamic_power_mW);

        if (data_mode_ == +DataMode::real_data) {
            payload.data.resize(payload.size_byte);
            std::copy_n(data_.begin() + payload.address_byte, payload.size_byte, payload.data.begin());
        }

        return {0, sc_core::SC_NS};
    } else {
        int write_data_size_byte =
            (payload.size_byte <= config_.write_max_width_byte) ? payload.size_byte : config_.write_max_width_byte;
        int write_data_unit_cnt = IntDivCeil(write_data_size_byte, config_.rw_min_unit_byte);
        double write_dynamic_power_mW = config_.rw_dynamic_power_per_unit_mW * write_data_unit_cnt;
        write_energy_counter_.addDynamicEnergyPJ(period_ns_, write_dynamic_power_mW);

        if (data_mode_ == +DataMode::real_data) {
            std::copy(payload.data.begin(), payload.data.end(), data_.begin() + payload.address_byte);
        }

        return {period_ns_, sc_core::SC_NS};
    }
}

EnergyReporter RegBuffer::getEnergyReporter() {
    EnergyReporter mem_energy_reporter{static_energy_counter_};
    mem_energy_reporter.addSubModule("read", EnergyReporter{read_energy_counter_});
    mem_energy_reporter.addSubModule("write", EnergyReporter{write_energy_counter_});
    return std::move(mem_energy_reporter);
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

}  // namespace pimsim
