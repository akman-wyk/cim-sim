//
// Created by wyk on 2024/7/4.
//

#include "memory.h"

#include "ram.h"
#include "reg_buffer.h"

namespace pimsim {

Memory::Memory(const char *name, const RAMConfig &ram_config, const AddressSpaceConfig &addressing,
               const SimConfig &sim_config, Core *core, Clock *clk)
    : BaseModule(name, sim_config, core, clk), is_mount(false), addressing_(addressing) {
    hardware_ = new RAM(name, ram_config, sim_config, core, clk);
    SC_THREAD(process);
}

Memory::Memory(const char *name, const RegBufferConfig &reg_buffer_config, const AddressSpaceConfig &addressing,
               const SimConfig &sim_config, Core *core, Clock *clk)
    : BaseModule(name, sim_config, core, clk), is_mount(false), addressing_(addressing) {
    hardware_ = new RegBuffer(name, reg_buffer_config, sim_config, core, clk);
    SC_THREAD(process);
}

Memory::Memory(const char *name, MemoryHardware *memory_hardware, const AddressSpaceConfig &addressing,
               const SimConfig &sim_config, Core *core, Clock *clk)
    : BaseModule(name, sim_config, core, clk), is_mount(true), addressing_(addressing) {
    hardware_ = memory_hardware;
    SC_THREAD(process);
}

Memory::~Memory() {
    if (!is_mount) {
        delete hardware_;
    }
}

void Memory::access(std::shared_ptr<MemoryAccessPayload> payload) {
    access_queue_.emplace(std::move(payload));
    start_process_.notify();
}

int Memory::getAddressSpaceBegin() const {
    return addressing_.offset_byte;
}

int Memory::getAddressSpaceEnd() const {
    return addressing_.offset_byte + addressing_.size_byte;
}

int Memory::getMemoryDataWidthByte(MemoryAccessType access_type) const {
    return hardware_->getMemoryDataWidthByte(access_type);
}

int Memory::getMemorySizeByte() const {
    return hardware_->getMemorySizeByte();
}

bool Memory::isMount() const {
    return is_mount;
}

EnergyReporter Memory::getEnergyReporter() {
    return hardware_->getEnergyReporter();
}

void Memory::process() {
    while (true) {
        while (access_queue_.empty()) {
            wait(start_process_);
        }
        auto payload_ptr = access_queue_.front();
        access_queue_.pop();

        sc_core::sc_time access_delay = hardware_->accessAndGetDelay(*payload_ptr);
        if (payload_ptr->ins.unit_type != +ExecuteUnitType::scalar) {
            wait(access_delay);
        }
        payload_ptr->finish_access.notify();
    }
}

}  // namespace pimsim
