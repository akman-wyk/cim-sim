import matplotlib.pyplot as plt
import matplotlib
import json

_profiling_json_file_path = '../report/profiling_0.json'
_time_line_space = 1
_time_line_width = 0.5

matplotlib.rcParams['font.sans-serif'] = ['SimSun']  # 设置中文字体为宋体
matplotlib.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题


def get_json(file_path):
    with open(file_path, 'r') as file:
        data = json.load(file)
    return data


def get_time_segments(module, limit=0x7fffffff, delete=False):
    time_segments = []
    for segment in module['time_segment_list']:
        if segment['start'] < limit:
            if delete and segment['end'] > 144:
                time_segments.append((segment['start'] - 128, segment['end'] - segment['start']))
            else:
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

    plt.ylim(0, y_tick)
    # plt.xlim(0, 200)
    plt.yticks(y_list, y_labels, fontsize=10.5)
    # plt.xticks([])
    # plt.grid()
    plt.tight_layout()

    plt.show()


def get_label(key):
    if key.split('.')[-1] == 'ScalarUnit':
        return 'Scalar'
    elif key.split('.')[-1] == 'transpose_memory':
        return 'PPU0'
    elif key.split('.')[-1] == 'transpose_memory#1':
        return 'PPU1'
    return ''

def print_hardware(j):
    y_list = []
    y_labels = []
    y_tick = _time_line_space
    plt.figure(figsize=(6, 2))
    for k in j.keys():
        module = j[k]["timing"]
        y_label = get_label(k)
        if module["activity_time"] > 0 and y_label != '':
            time_segments = get_time_segments(module)
            plt.broken_barh(time_segments, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#9AD6D2')
            y_list.append(y_tick)
            y_labels.append(y_label)
            y_tick += _time_line_space

    plt.ylim(0, y_tick)
    plt.xlim(0, 2000)
    plt.yticks(y_list, y_labels, fontsize=10.5)
    # plt.xticks([])
    # plt.grid()
    plt.tight_layout()

    plt.show()


def print_profiling_0():
    j = get_json('../report/profiling_0.json')["hardware_profiling"]
    y_list = []
    y_labels = ['PPU', 'Scalar']
    y_tick = _time_line_space
    plt.figure(figsize=(4, 2))

    ipu_read = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory.read']['timing'], 5945, True)
    ipu_write = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory.write']['timing'], 5945, True)
    scalar = get_time_segments(j['Chip.Core Overview.ScalarUnit']['timing'], 5945, True)

    y_tick = _time_line_space
    plt.broken_barh(ipu_write, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#FFB061')
    plt.broken_barh(ipu_read, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#BAE4E1')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.broken_barh(scalar, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#9ECC82')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.ylim(0, y_tick)
    plt.xlim(0, 5804)
    plt.yticks(y_list, y_labels, fontsize=10.5)
    plt.yticks(y_list, y_labels, fontname='Times New Roman', fontsize=12, weight='bold')
    plt.xticks(fontname='Times New Roman', fontsize=10.5)
    plt.xlabel('Time (ns)', fontname='Times New Roman', fontsize=12, weight='bold')
    plt.tight_layout()

    # plt.show()
    plt.savefig("fig11-profiling_0.pdf", format='pdf', dpi=1000, bbox_inches='tight')
    plt.close()  # 可选：防止多次显示图像


def print_profiling_1():
    j = get_json('../report/profiling_1.json')["hardware_profiling"]
    y_list = []
    y_labels = ['PPU', 'Scalar']
    y_tick = _time_line_space
    plt.figure(figsize=(4, 2))

    ipu_read = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory.read']['timing'], 1753, True)
    ipu_write = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory.write']['timing'], 1753, True)
    scalar = get_time_segments(j['Chip.Core Overview.ScalarUnit']['timing'], 1753, True)

    y_tick = _time_line_space
    plt.broken_barh(ipu_write, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#FFB061')
    plt.broken_barh(ipu_read, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#7ECCC6')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.broken_barh(scalar, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#9ECC82')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.ylim(0, y_tick)
    plt.xlim(0, 1599)
    plt.yticks(y_list, y_labels, fontsize=10.5)
    plt.yticks(y_list, y_labels, fontname='Times New Roman', fontsize=12, weight='bold')
    plt.xticks(fontname='Times New Roman', fontsize=10.5)
    plt.xlabel('Time (ns)', fontname='Times New Roman', fontsize=12, weight='bold')
    plt.tight_layout()

    # plt.show()
    plt.savefig("fig11-profiling_1.pdf", format='pdf', dpi=1000, bbox_inches='tight')
    plt.close()  # 可选：防止多次显示图像


def print_profiling_2():
    j = get_json('../report/profiling_2.json')["hardware_profiling"]
    y_list = []
    y_labels = ['PPU0', 'PPU1', 'Scalar']
    y_tick = _time_line_space
    plt.figure(figsize=(5.18, 2))

    ipu0_read = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory.read']['timing'], 1240)
    ipu0_write = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory.write']['timing'], 968)
    ipu1_read = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory#1.read']['timing'], 1240)
    ipu1_write = get_time_segments(j['Chip.Core Overview.LocalMemoryUnit.transpose_memory#1.write']['timing'], 968)
    scalar = get_time_segments(j['Chip.Core Overview.ScalarUnit']['timing'], 1240)

    y_tick = _time_line_space
    plt.broken_barh(ipu0_write, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#FFB061')
    plt.broken_barh(ipu0_read, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#7ECCC6')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.broken_barh(ipu1_write, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#FFB061', label='PPU load')
    plt.broken_barh(ipu1_read, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#7ECCC6', label='PPU read')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.broken_barh(scalar, (y_tick - _time_line_width / 2, _time_line_width), facecolors='#9ECC82', label='Scalar ops')
    y_list.append(y_tick)
    y_tick += _time_line_space

    plt.ylim(0, y_tick)
    plt.xlim(0, 1240)
    plt.yticks(y_list, y_labels, fontname='Times New Roman', fontsize=12, weight='bold')
    plt.xticks(fontname='Times New Roman', fontsize=10.5)
    plt.xlabel('Time (ns)', fontname='Times New Roman', fontsize=12, weight='bold')
    plt.legend(loc='center left', bbox_to_anchor=(1, 0.5), fontsize=12, prop={'family': 'Times New Roman', 'weight': 'bold'})
    plt.tight_layout()

    # plt.show()
    plt.savefig("fig11-profiling_2.pdf", format='pdf', dpi=1000, bbox_inches='tight')
    plt.close()  # 可选：防止多次显示图像


if __name__ == '__main__':
    # print_profiling_0()
    print_profiling_1()
    # print_profiling_2()
    # j = get_json(_profiling_json_file_path)
    # # print_timing(j, 'conv')
    # print_hardware(j["hardware_profiling"])
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
