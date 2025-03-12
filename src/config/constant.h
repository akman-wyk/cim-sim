//
// Created by wyk on 2025/3/12.
//

#pragma once

namespace cimsim {

static constexpr int GENERAL_REG_NUM = 32;
static constexpr int SPECIAL_REG_NUM = 32;

static constexpr int WORD_BYTE_SIZE = 4;

static constexpr int SIMD_MAX_INPUT_NUM = 4;
static constexpr int SIMD_MAX_OPCODE = 255;
static constexpr int SIMD_INSTRUCTION_OPCODE_BIT_LENGTH = 8;

static constexpr int BYTE_TO_BIT = 8;
static constexpr unsigned char BYTE_MAX_VALUE = 0xff;

using BitmapCellType = unsigned int;
static constexpr int BITMAP_CELL_BIT_WIDTH = sizeof(BitmapCellType) * BYTE_TO_BIT;
static constexpr int LOG2_BITMAP_CELL_BIT_WIDTH = __builtin_ctz(BITMAP_CELL_BIT_WIDTH);
static constexpr int BITMAP_CELL_BIT_WIDTH_MASK = ((1 << LOG2_BITMAP_CELL_BIT_WIDTH) - 1);

static constexpr int LOCAL_MEMORY_COUNT_MAX = 32;
static constexpr int MEMORY_BITMAP_SIZE = (LOCAL_MEMORY_COUNT_MAX - 1) / BITMAP_CELL_BIT_WIDTH + 1;

}  // namespace cimsim
