# OpenNNA:一个可根据FPGA资源自由裁剪的通用神经网络FPGA加速器
## 作者:xddcore
## Email:1034029664@qq.com
## Github:www.github.com/xddcore
## Version:1.2
## Date:23/10/2021

# 项目简介
**OpenNNA**(*Opensource Neural Network Accelerator*)计划使用Xilinx ZYNQ 7020平台实现通用神经网络加速器。实现对神经网络的计算加速。

# 项目特性
1. 支持**Conv2d卷积**，支持**Depthwish Conv2d卷积**，支持**MobileNETV2 Bottleneck结构**, 卷积核为**1x1或 3x3**,stride为**1或2**。
2. Same对称paddings (TensorFlow 在 stride=2同时尺寸为偶数时使用非对称 paddings)。
3. 特征图尺寸：特征图小于等于 320x320(WxH)，通道数在 1到 1024。
4. MaxPool(**2x2或4x4)** 和AveragePool(**2x2或 x4**)。
5. 任意逐元素激活函数(**ReLU, ReLU6, LeakyRelu, Sigmoid**)。
6. **xddcore说:以上的尺寸约束都可打破**。
7. 强大的**流水线控制器**和**标准的控制接口**，RAM和PE高度复用，尽可能的提升推理速度(面积铺开，咱就是全场最靓的崽）。
8. **可任意快速修改的网络结构**，在RAM足够的情况下，支持**无穷多层网络**。
9. 相比于传统GPU，**超低的功耗**。
10. **大三学生写的代码，拒绝繁琐语法，代码简单，可读性极佳**。
11. 以**Tensorflow**为核心的一系列python小脚本，折腾性极佳。
12. 多应用场景:**目标分类(√)|目标检测(√)|语音识别(√)|etc...|**，xddcore说:下到手写数字识别，上到自动驾驶，左到RoboMaster装甲板识别，又到人脸识别通通一站式搞定。**年轻人的手中握着通往未来的钥匙！**
13. xddcore说:准备上ZYNQ-7100了(**欢迎推荐合适的海外供货商**），后续描述RNN？还有更多特性，正在开发中...

# 文件夹
本文件夹放置c语言手写的前向推理过程。