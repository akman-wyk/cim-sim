//
// Created by wyk on 2024/7/5.
//

#include "memory_socket.h"

#include "fmt/format.h"
#include "memory/memory_unit.h"
#include "network/switch.h"
#include "util/log.h"

namespace cimsim {

MemorySocket::~MemorySocket() {
    delete finish_read_;
    delete finish_write_;
    delete finish_load_;
    delete finish_store_;
    delete finish_load_trans_;
    delete finish_store_trans_;
}

void MemorySocket::bindLocalMemoryUnit(cimsim::MemoryUnit *local_memory_unit) {
    local_memory_unit_ = local_memory_unit;
    finish_read_ = new sc_event{};
    finish_write_ = new sc_event{};
}

void MemorySocket::bindSwitchAndGlobalMemory(Switch *_switch, int core_id, int global_memory_switch_id) {
    switch_ = _switch;
    core_id_ = core_id;
    global_memory_switch_id_ = global_memory_switch_id;
    finish_load_ = new sc_event{};
    finish_store_ = new sc_event{};
    finish_load_trans_ = new sc_event{};
    finish_store_trans_ = new sc_event{};
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

std::vector<uint8_t> MemorySocket::loadGlobal(const InstructionPayload &ins, int address_byte, int size_byte) {
    LOG(fmt::format("core id: {}, load global data start, pc: {}", core_id_, ins.pc));
    auto global_payload =
        std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                  .access_type = MemoryAccessType::read,
                                                                  .address_byte = address_byte,
                                                                  .size_byte = size_byte,
                                                                  .finish_access = *finish_load_});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = global_memory_switch_id_,
                                                                           .finish_network_trans = finish_load_trans_,
                                                                           .request_data_size_byte = 1,
                                                                           .request_payload = global_payload,
                                                                           .response_data_size_byte = size_byte,
                                                                           .response_payload = nullptr});
    switch_->transportHandler(network_payload);
    wait(*network_payload->finish_network_trans);

    LOG(fmt::format("core id: {}, load global data end, pc: {}", core_id_, ins.pc));
    return std::move(global_payload->data);
}

void MemorySocket::storeGlobal(const InstructionPayload &ins, int address_byte, int size_byte,
                               std::vector<uint8_t> data) {
    LOG(fmt::format("core id: {}, store global data start, pc: {}", core_id_, ins.pc));
    auto global_payload =
        std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                  .access_type = MemoryAccessType::write,
                                                                  .address_byte = address_byte,
                                                                  .size_byte = size_byte,
                                                                  .data = std::move(data),
                                                                  .finish_access = *finish_store_});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = global_memory_switch_id_,
                                                                           .finish_network_trans = finish_store_trans_,
                                                                           .request_data_size_byte = size_byte,
                                                                           .request_payload = global_payload,
                                                                           .response_data_size_byte = 1,
                                                                           .response_payload = nullptr});
    switch_->transportHandler(network_payload);
    wait(*network_payload->finish_network_trans);
    LOG(fmt::format("core id: {}, store global data end, pc: {}", core_id_, ins.pc));
}

int MemorySocket::getLocalMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const {
    return local_memory_unit_->getMemoryDataWidthById(memory_id, access_type);
}

int MemorySocket::getLocalMemorySizeById(int memory_id) const {
    return local_memory_unit_->getMemorySizeById(memory_id);
}

}  // namespace cimsim
