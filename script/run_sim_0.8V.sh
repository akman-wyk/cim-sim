#!/bin/bash

# 配置文件的数组
models=("alexnet" "efficient" "mobilev2" "resnet18" "vgg19")
configs=("../test_config/2025_paper/alexnet.json" "../test_config/2025_paper/efficient.json" "../test_config/2025_paper/mobilev2.json" "../test_config/2025_paper/resnet18.json" "../test_config/2025_paper/vgg19.json")

# 用于记录每次耗时的数组
times=()

# 循环遍历所有模型配置
for i in ${!models[@]}; do
  model=${models[$i]}
  config=${configs[$i]}

  # 开始时间
  start_time=$(date +%s)
  echo "Test 0.8V $model"

  # 运行测试
  ./NetworkSimulator $config

  # 结束时间
  end_time=$(date +%s)

  # 计算耗时
  cost_time=$((end_time - start_time))
  times+=($cost_time)

  # 输出每次测试耗时
  echo "$model 共耗时: $(($cost_time / 60))min $(($cost_time % 60))s"
done

# 打印总耗时
echo "所有测试结束，统计各模型的耗时："
for i in ${!models[@]}; do
  model=${models[$i]}
  cost_time=${times[$i]}
  echo "$model: $(($cost_time / 60))min $(($cost_time % 60))s"
done
