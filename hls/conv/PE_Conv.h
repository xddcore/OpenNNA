#ifndef _PE_CONV_H_
#define _PE_CONV_H_
#include <ap_int.h>
#include <ap_fixed.h>

typedef ap_fixed<16,8,AP_RND,AP_SAT> data_t;//计算参数类型
typedef short reg_t;//控制寄存器类型

/*PE结构配置*/
#define MAX_PE_Register_NUM 13 //计算控制寄存器元素个数
#define MAX_PE_Kernel_size 3 //最大输入卷积核尺寸
#define MAX_PE_Kernel_channel 1024 //最大输入卷积核通道
#define MAX_PE_Kernel_number 1024 //最大输入卷积核数量
#define MAX_PE_Bias_number 1024 //最大输入卷积核偏置
#define MAX_PE_Input_fmap_channel 1024 //最大输入特征图通道
#define MAX_PE_Input_fmap_size 320 //最大输入特征图尺寸
#define MAX_PE_Outputput_fmap_channel 1024 //最大输出特征图通道
#define MAX_PE_Output_fmap_size 320 //最大输出特征图尺寸

#define MAX_BUFFER_Input_fmap_size 3 //每个输入矩阵Buffer一次最多只计算MAX_Kernel_size*MAX_Kernel_size*MAX_Input_fmap_channel(也就是一次数据Buffer只提取卷积核覆盖的地方
#define MAX_BUFFER_Output_fmap_size 1 //卷积核每滑动一次生成一通道fmap上的一个点
#define MAX_BUFFER_Bias_number 1 //每个buffer只算一个卷积核,一个卷积核只有一个bias

void PE_conv2d(
	/*PE控制寄存器*/
	 reg_t *PE_Register,
	/*输入卷积核权重*/
	 data_t *PE_Kernel,
	/*输入卷积核偏置*/
	 data_t *PE_Bias,
	/*输入卷积核特征图*/
	 data_t *PE_Input_fmap,
	/*PE输出特征图*/
	 data_t *PE_Output_fmap
);

#endif
