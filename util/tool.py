import os


def process_test_config(network):
    data_dir = fr'D:\code\cim-sim\data\{network}\dense'
    layer_name_list = []
    for item in os.listdir(data_dir):
        if os.path.isdir(os.path.join(data_dir, item)):
            layer_name_list.append(item)
    layer_name_list.sort(key=lambda x: int(x.split("_")[0]))

    layer_config = '"layer_config": [\n'
    for layer in layer_name_list:
        if 'dw' in layer:
            continue
        layer_config += '\t{'
        layer_config += f'"sub_dir_name": "{layer}"'
        layer_config += '},\n'
    layer_config += ']'

    print(layer_config)


if __name__ == '__main__':
    process_test_config('MobileNetV2')
