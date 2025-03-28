//
// Created by wyk on 2025/3/28.
//

#pragma once
#include <unordered_set>

#include "config/config.h"
#include "core/execute_unit/payload.h"
#include "core/payload.h"

namespace cimsim {

class LocalDedicatedDataPath {
public:
    static int getMemoryPairUniqueId(int memory1_id, int memory2_id);

public:
    explicit LocalDedicatedDataPath(const LocalDedicatedDataPathConfig& config);

    bool containsMemoryPair(int memory1_id, int memory2_id) const;

    [[nodiscard]] unsigned int getId() const;

private:
    unsigned int id_;
    std::unordered_set<int> memory_pair_id_set_{};
};

class DataPathManager {
public:
    explicit DataPathManager(const std::vector<LocalDedicatedDataPathConfig>& local_dedicated_data_path_configs);

    [[nodiscard]] DataPathPayload decodeTransferInsDataPath(int src_mem_id, int dst_mem_id) const;

private:
    std::vector<LocalDedicatedDataPath> local_dedicated_data_path_list_;
};

}  // namespace cimsim
