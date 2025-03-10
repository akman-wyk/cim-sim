//
// Created by wyk on 2024/7/5.
//

#include "memory_socket.h"

#include <iostream>

#include "core/local_memory_unit/local_memory_unit.h"

namespace pimsim {

void MemorySocket::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_unit_ = local_memory_unit;
}

std::vector<uint8_t> MemorySocket::readData(const pimsim::InstructionPayload &ins, int address_byte, int size_byte) {
    if (local_memory_unit_ == nullptr) {
        std::cerr << "Not yet bound local memory unit" << std::endl;
        return {};
    }
    return local_memory_unit_->read_data(ins, address_byte, size_byte, finish_read_);
}

void MemorySocket::writeData(const pimsim::InstructionPayload &ins, int address_byte, int size_byte,
                             std::vector<uint8_t> data) {
    if (local_memory_unit_ == nullptr) {
        std::cerr << "Not yet bound local memory unit" << std::endl;
        return;
    }
    local_memory_unit_->write_data(ins, address_byte, size_byte, std::move(data), finish_write_);
}

int MemorySocket::getLocalMemoryIdByAddress(int address_byte) const {
    return local_memory_unit_->getLocalMemoryIdByAddress(address_byte);
}

int MemorySocket::getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const {
    return local_memory_unit_->getMemoryDataWidthById(memory_id, access_type);
}

int MemorySocket::getMemorySizeById(int memory_id) const {
    return local_memory_unit_->getMemorySizeById(memory_id);
}

}  // namespace pimsim
