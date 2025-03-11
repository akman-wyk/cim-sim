#!/bin/bash
start_time=$(date +%s)

./LayerSimulator ../config/2024dac/config.json ../test_data/2024dac/resnet-2024-11-15T23-38.json -r -s ../report/2024dac.txt

end_time=$(date +%s)
cost_time=$[ $end_time-$start_time ]
echo "共耗时: $(($cost_time/60))min $(($cost_time%60))s"