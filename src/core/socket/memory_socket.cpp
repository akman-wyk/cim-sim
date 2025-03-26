//
// Created by wyk on 2024/7/5.
//

#include "memory_socket.h"

#include "fmt/format.h"
#include "memory/memory_unit.h"

namespace cimsim {

MemorySocket::~MemorySocket() {
    delete finish_read_;
    delete finish_write_;
}

void MemorySocket::bindLocalMemoryUnit(cimsim::MemoryUnit *local_memory_unit) {
    local_memory_unit_ = local_memory_unit;
    finish_read_ = new sc_event{};
    finish_write_ = new sc_event{};
}

std::vector<uint8_t> MemorySocket::readLocal(const cimsim::InstructionPayload &ins, int address_byte, int size_byte) {
    auto payload = std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                             .access_type = MemoryAccessType::read,
                                                                             .address_byte = address_byte,
                                                                             .size_byte = size_byte,
                                                                             .finish_access = *finish_read_});
    local_memory_unit_->access(payload);
    return std::move(payload->data);
}

void MemorySocket::writeLocal(const cimsim::InstructionPayload &ins, int address_byte, int size_byte,
                              std::vector<uint8_t> data) {
    auto payload = std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                             .access_type = MemoryAccessType::write,
                                                                             .address_byte = address_byte,
                                                                             .size_byte = size_byte,
                                                                             .data = std::move(data),
                                                                             .finish_access = *finish_write_});
    local_memory_unit_->access(payload);
}

int MemorySocket::getLocalMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const {
    return local_memory_unit_->getMemoryDataWidthById(memory_id, access_type);
}

int MemorySocket::getLocalMemorySizeById(int memory_id) const {
    return local_memory_unit_->getMemorySizeById(memory_id);
}

}  // namespace cimsim
