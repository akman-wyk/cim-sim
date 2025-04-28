import matplotlib.pyplot as plt
import matplotlib
import json

_profiling_json_file_path = '../report/profiling.json'
_time_line_space = 4
_time_line_width = 2

matplotlib.rcParams['font.sans-serif'] = ['SimSun']  # 设置中文字体为宋体
matplotlib.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题


def get_json(file_path):
    with open(file_path, 'r') as file:
        data = json.load(file)
    return data


def get_time_segments(module):
    time_segments = []
    for segment in module['time_segment_list']:
        time_segments.append((segment['start'], segment['end'] - segment['start']))
    return time_segments


def print_timing(j, name):
    module = j['instruction_profiling'][name]

    y_list = []
    y_labels = []
    y_tick = _time_line_space
    for k in module.keys():
        time_segments = get_time_segments(module[k])
        plt.broken_barh(time_segments, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#9AD6D2')
        y_list.append(y_tick)
        y_labels.append(k)
        y_tick += _time_line_space

    # computation = get_time_segments(module['computation'])
    # memory = get_time_segments(module['memory'])
    # control = get_time_segments(module['control'])
    # transport = get_time_segments(module['transport'])
    #
    # plt.broken_barh(computation, (3, 2), facecolors=('#9AD6D2'))
    # plt.broken_barh(memory, (7, 2), facecolors=('#FFCF9F'))
    # plt.broken_barh(control, (11, 2), facecolors=('#8AB3D2'))
    # plt.broken_barh(transport, (15, 2), facecolors=('#ADD395'))

    plt.ylim(0, y_tick)
    # plt.xlim(0, 200)
    plt.yticks(y_list, y_labels, fontsize=10.5)
    # plt.xticks([])
    # plt.grid()
    plt.tight_layout()

    plt.show()


if __name__ == '__main__':
    j = get_json(_profiling_json_file_path)
    print_timing(j, 'conv')
    # module = j['instruction_profiling']["conv.cim_compute"]
    # print(module.keys())

# plt.broken_barh([(110, 30), (150, 10)], (10, 9))
# plt.broken_barh([(10, 50), (100, 20), (130, 10)], (20, 9),
#                 facecolors=('r', 'g', 'b'))
# plt.ylim(5, 35)
# plt.xlim(0, 200)
# plt.yticks([15, 25], ['A', 'B'])
# plt.grid()
#
# plt.show()
