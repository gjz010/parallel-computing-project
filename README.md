# 并行计算MPI作业

构建
--------
在MPI环境下：
```
make
```

在神威环境下运行所有测试用例：

```
./run_all_testcases_sunway.sh
```



Bench1 点对点带宽测试
--------

```
用法： ./bench1 -s [包大小] -c [重复实验次数] -g [分组]
其中分块的表示方法： 组1:组2:组3
块的表示方法：节点1,节点2,节点3
```

程序会尽可能地将各个块内部的通信带宽并行进行测试；对于块间节点的带宽，出于保守考虑（例如，各个块之间为星形连接）我们逐一进行测试。

程序会输出三个矩阵：发送矩阵、接收矩阵和混合矩阵。发送矩阵的第i行第j列表示从第i个节点到第j个节点的发送带宽，接收矩阵的第i行第j列表示从第i个节点接收第j个节点数据的接收带宽。
混合矩阵的上三角是接收矩阵，下三角是发送矩阵。
前两个矩阵会被二进制地保存在`bench1.out`中。

Bench2 集合通信带宽测试
--------
```
用法： ./bench2 方案1 方案2 ...
其中方案的表示方法：方法,包大小,节点1,节点2,节点3,...
方法可以是bcast/gather/reduce/allreduce/scan/alltoall中的一种
```

程序会根据选定的方法、包带宽和节点集合进行带宽测试。在进行bcast/gather/reduce操作的时候，节点1会被视为根节点。

Bench3 MPIIO带宽测试
--------
```
用法： ./bench3 文件名 方案1 方案2 ...
其中方案为以下两种之一
聚合读写带宽 c,节点1,块大小1,节点2,块大小2,...
单独读写带宽 操作1,节点1,块大小1:操作2,节点2,块大小2:...
操作可以为r或w中一种
```

Bench3会以下方式进行测试：首先被选中的所有进程被线性对应到文件中相应大小的块，各个进程所操作的块互不重叠。
在测量聚合读写带宽时，首先会测量一次聚合写带宽，然后会测量一次聚合度带宽；
在测量单独读写带宽时，一个方案内的所有操作（以操作、节点、块大小组成的三元组表示），无论读写，会被同时执行。

可以使用`mpirun`或者`bsub`，指定同一节点开启的进程数。

RR 程序并行化
--------

我们采用以下的方法进行并行化：注意到

![defabr](https://latex.codecogs.com/gif.latex?%5Cleft%5C%7B%5Cbegin%7Bmatrix%7D%20%5Calpha_%7Bij%7D%3D%5Csum_%7B0%5Cleqslant%20k%20%3Cn%7D%5Csum_%7B0%5Cleqslant%20i%2Cj%20%3C%20m%7D%5Csum%20u_%7Bik%7D%5E2%5C%5C%20%5Cbeta_%7Bij%7D%3D%5Csum_%7B0%5Cleqslant%20k%20%3Cn%7D%5Csum_%7B0%5Cleqslant%20i%2Cj%20%3C%20m%7D%5Csum%20u_%7Bjk%7D%5E2%5C%5C%20%5Cgamma_%7Bij%7D%3D%5Csum_%7B0%5Cleqslant%20k%20%3Cn%7D%5Csum_%7B0%5Cleqslant%20i%2Cj%20%3C%20m%7D%5Csum%20u_%7Bik%7Du_%7Bjk%7D%5C%5C%20%5Cend%7Bmatrix%7D%5Cright.)

其中α和β可以只计算一维以节约计算量；此外，三个求和都可以简单地对n分治。我们的RR即采用了这两种优化方法对算法进行并行化，不同的节点对于不同的n进行并行计算，最后使用reduce进行求和。
