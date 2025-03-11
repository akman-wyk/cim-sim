#!/bin/bash
start_time=$(date +%s)

echo "Test 0.8V alexnet"
./NetworkSimulator ../test_config/0p8V/alexnet.json
echo "Test 0.8V efficient"
./NetworkSimulator ../test_config/0p8V/efficient.json
echo "Test 0.8V mobilev2"
./NetworkSimulator ../test_config/0p8V/mobilev2.json
echo "Test 0.8V resnet18"
./NetworkSimulator ../test_config/0p8V/resnet18.json
echo "Test 0.8V vgg19"
./NetworkSimulator ../test_config/0p8V/vgg19.json

end_time=$(date +%s)
cost_time=$[ $end_time-$start_time ]
echo "共耗时: $(($cost_time/60))min $(($cost_time%60))s"
