//
// Created by wyk on 2025/3/28.
//

#include "data_path_manager.h"

#include "address_space/address_space.h"

namespace cimsim {

int LocalDedicatedDataPath::getMemoryPairUniqueId(int memory1_id, int memory2_id) {
    int max_memory_id = std::max(memory1_id, memory2_id);
    int min_memory_id = std::min(memory1_id, memory2_id);
    return ((min_memory_id << LOG2_CEIL_LOCAL_MEMORY_COUNT_MAX) | max_memory_id);
}

LocalDedicatedDataPath::LocalDedicatedDataPath(const LocalDedicatedDataPathConfig& config) : id_(config.id) {
    auto as = AddressSapce::getInstance();
    for (auto& mem_p : config.memory_pair_list) {
        int memory1_id = as.getMemoryId(getDuplicateMemoryName(mem_p.memory_1.name, mem_p.memory_1.duplicate_id));
        int memory2_id = as.getMemoryId(getDuplicateMemoryName(mem_p.memory_2.name, mem_p.memory_2.duplicate_id));
        memory_pair_id_set_.emplace(getMemoryPairUniqueId(memory1_id, memory2_id));
    }
}

bool LocalDedicatedDataPath::containsMemoryPair(int memory1_id, int memory2_id) const {
    if (memory1_id < 0 || memory2_id < 0) {
        return false;
    }
    return memory_pair_id_set_.count(getMemoryPairUniqueId(memory1_id, memory2_id)) == 1;
}

unsigned int LocalDedicatedDataPath::getId() const {
    return id_;
}

DataPathManager::DataPathManager(const std::vector<LocalDedicatedDataPathConfig>& local_dedicated_data_path_configs) {
    for (auto& local_dedicated_data_path_config : local_dedicated_data_path_configs) {
        local_dedicated_data_path_list_.emplace_back(local_dedicated_data_path_config);
    }
}

DataPathPayload DataPathManager::decodeTransferInsDataPath(int src_mem_id, int dst_mem_id) const {
    for (auto& local_dedicated_data_path : local_dedicated_data_path_list_) {
        if (local_dedicated_data_path.containsMemoryPair(src_mem_id, dst_mem_id)) {
            return {.type = DataPathType::local_dedicated_data_path,
                    .local_dedicated_data_path_id = local_dedicated_data_path.getId()};
        }
    }
    return {.type = DataPathType::intra_core_bus, .local_dedicated_data_path_id = 0};
}

}  // namespace cimsim
