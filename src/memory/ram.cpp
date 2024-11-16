//
// Created by wyk on 2024/7/4.
//

#include "ram.h"

#include "../../packages/fmt/include/fmt/core.h"
#include "../util/util.h"
#include "core/core.h"

namespace pimsim {

RAM::RAM(const char *name, const pimsim::RAMConfig &config, const pimsim::SimConfig &sim_config, pimsim::Core *core,
         pimsim::Clock *clk)
    : MemoryHardware(name, sim_config, core, clk), config_(config) {
    if (data_mode_ == +DataMode::real_data) {
        initialData();
    }

    static_energy_counter_.setStaticPowerMW(config_.static_power_mW);
}

sc_core::sc_time RAM::accessAndGetDelay(pimsim::MemoryAccessPayload &payload) {
    if (payload.address_byte < 0 || payload.address_byte + payload.size_byte > config_.size_byte) {
        std::cerr << fmt::format("Core id: {}, Invalid memory access with ins NO.'{}': address {} overflow, size: {}, config size: {}", 
                                core_->getCoreId(), payload.ins.pc, payload.address_byte, payload.size_byte, config_.size_byte)
                  << std::endl;
        return {0.0, sc_core::SC_NS};
    }

    int process_times = IntDivCeil(payload.size_byte, config_.width_byte);
    double latency;
    if (payload.access_type == +MemoryAccessType::read) {
        latency = process_times * config_.read_latency_cycle * period_ns_;
        read_energy_counter_.addDynamicEnergyPJ(latency, config_.read_dynamic_power_mW);

        if (data_mode_ == +DataMode::real_data) {
            payload.data.resize(payload.size_byte);
            std::copy_n(data_.begin() + payload.address_byte, payload.size_byte, payload.data.begin());
        }
    } else {
        latency = process_times * config_.write_latency_cycle * period_ns_;
        write_energy_counter_.addDynamicEnergyPJ(latency, config_.write_dynamic_power_mW);

        if (data_mode_ == +DataMode::real_data) {
            std::copy(payload.data.begin(), payload.data.end(), data_.begin() + payload.address_byte);
        }
    }

    return {latency, sc_core::SC_NS};
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

EnergyReporter RAM::getEnergyReporter() {
    EnergyReporter mem_energy_reporter{static_energy_counter_};
    mem_energy_reporter.addSubModule("read", EnergyReporter{read_energy_counter_});
    mem_energy_reporter.addSubModule("write", EnergyReporter{write_energy_counter_});
    return std::move(mem_energy_reporter);
}

int RAM::getMemoryDataWidthByte(MemoryAccessType access_type) const {
    return config_.width_byte;
}

int RAM::getMemorySizeByte() const {
    return config_.size_byte;
}

}  // namespace pimsim
