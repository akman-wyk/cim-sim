import matplotlib
import matplotlib.pyplot as plt
import numpy as np

def set_label(rects):
    for rect in rects:
        height = rect.get_height() # 获取⾼度
        plt.text(x = rect.get_x() + rect.get_width()/2, # ⽔平坐标
                 y = height + 0.5, # 竖直坐标
                 s = height, # ⽂本
                 ha = 'center') # ⽔平居中

labels = ['G1', 'G2', 'G3', 'G4', 'G5','G6'] # 级别
men_means = np.random.randint(20,35,size = 6)
women_means = np.random.randint(20,35,size = 6)

if __name__ == '__main__':
    x = np.arange(len(men_means))
    print(x)

    plt.figure(figsize=(9,6))

    width = 0.3

    rects1 = plt.bar(x - width/2, men_means, width) # 返回绘图区域对象
    rects2 = plt.bar(x + width/2, women_means, width)

    # 设置标签标题，图例
    plt.ylabel('Scores')
    plt.title('Scores by group and gender')
    plt.xticks(x,labels)
    plt.legend(['Men','Women'])

    # 添加注释

    set_label(rects1)
    set_label(rects2)

    plt.tight_layout() # 设置紧凑布局
    # plt.savefig('./分组带标签柱状图.png')3
    plt.show()
