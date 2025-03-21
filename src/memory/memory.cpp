//
// Created by wyk on 2024/7/4.
//

#include "memory.h"

#include "address_space/address_space.h"
#include "ram.h"
#include "reg_buffer.h"

namespace cimsim {

Memory::Memory(const sc_core::sc_module_name& name, const RAMConfig &ram_config, const SimConfig &sim_config, Core *core,
               Clock *clk)
    : BaseModule(name, sim_config, core, clk), is_mount(false) {
    hardware_ = new RAM("ram", ram_config, sim_config, core, clk);
    as_offset_ = AddressSapce::getInstance().getMemoryAddressSpaceOffset(std::string{name});
    SC_THREAD(process);
}

Memory::Memory(const sc_core::sc_module_name& name, const RegBufferConfig &reg_buffer_config, const SimConfig &sim_config,
               Core *core, Clock *clk)
    : BaseModule(name, sim_config, core, clk), is_mount(false) {
    hardware_ = new RegBuffer("reg_buffer", reg_buffer_config, sim_config, core, clk);
    as_offset_ = AddressSapce::getInstance().getMemoryAddressSpaceOffset(std::string{name});
    SC_THREAD(process);
}

Memory::Memory(const sc_core::sc_module_name& name, MemoryHardware *memory_hardware, const SimConfig &sim_config, Core *core,
               Clock *clk)
    : BaseModule(name, sim_config, core, clk), is_mount(true) {
    hardware_ = memory_hardware;
    as_offset_ = AddressSapce::getInstance().getMemoryAddressSpaceOffset(hardware_->getMemoryName());
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

int Memory::getAddressSpaceOffset() const {
    return as_offset_;
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

void Memory::setMemoryID(int mem_id) {
    hardware_->setMemoryID(mem_id);
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

}  // namespace cimsim
