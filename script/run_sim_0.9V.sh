#!/bin/bash
start_time=$(date +%s)

echo "Test 0.9V alexnet"
./NetworkSimulator ../test_config/alexnet_0.9V.json
echo "Test 0.9V efficient"
./NetworkSimulator ../test_config/efficient_0.9V.json
echo "Test 0.9V mobilev2"
./NetworkSimulator ../test_config/mobilev2_0.9V.json
echo "Test 0.9V resnet18"
./NetworkSimulator ../test_config/resnet18_0.9V.json
echo "Test 0.9V vgg19"
./NetworkSimulator ../test_config/vgg19_0.9V.json

end_time=$(date +%s)
cost_time=$[ $end_time-$start_time ]
echo "共耗时: $(($cost_time/60))min $(($cost_time%60))s"
