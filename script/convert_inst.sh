#!/bin/bash

# 遍历当前目录下所有以 "core_" 开头的文件
echo "Convert Core Code"
for file in test_data/core/core_test_data_*.json; do
    # 检查是否存在匹配的文件
    if [ -e "$file" ]; then
        # 构造新的文件名，将 "core" 替换为 "core_v2"
        new_file="${file/core/core_v2}"
        ./build_pc/InstConvert core "$file" "$new_file"
        echo "Convert: $file -> $new_file"
    fi
done

echo "Convert Chip Code"
for file in test_data/chip/chip_test_data_*.json; do
    # 检查是否存在匹配的文件
    if [ -e "$file" ]; then
        # 构造新的文件名，将 "chip" 替换为 "chip_v2"
        new_file="${file/chip/chip_v2}"
        ./build_pc/InstConvert chip "$file" "$new_file"
        echo "Convert: $file -> $new_file"
    fi
done