#!/bin/bash
start_time=$(date +%s)

echo "Test 0.72V alexnet"
./NetworkSimulator ../test_config/0p72V/alexnet.json
echo "Test 0.72V efficient"
./NetworkSimulator ../test_config/0p72V/efficient.json
echo "Test 0.72V mobilev2"
./NetworkSimulator ../test_config/0p72V/mobilev2.json
echo "Test 0.72V resnet18"
./NetworkSimulator ../test_config/0p72V/resnet18.json
echo "Test 0.72V vgg19"
./NetworkSimulator ../test_config/0p72V/vgg19.json

end_time=$(date +%s)
cost_time=$[ $end_time-$start_time ]
echo "共耗时: $(($cost_time/60))min $(($cost_time%60))s"
