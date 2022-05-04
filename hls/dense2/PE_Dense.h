#ifndef _PE_DENSE_H_
#define _PE_DENSE_H_
#include <ap_int.h>
#include <ap_fixed.h>

typedef ap_fixed<16,8,AP_RND,AP_SAT> data_t;//计算参数类型
typedef short reg_t;//控制寄存器类型

/*PE结构配置*/
#define MAX_PE_Register_NUM 6 //计算控制寄存器元素个数
#define MAX_PE_Kernel_size 3 //最大输入卷积核尺寸
#define MAX_PE_Kernel_channel 1024 //最大输入卷积核通道
#define MAX_PE_Kernel_number 1024 //最大输入卷积核数量
#define MAX_PE_Bias_number 1024 //最大输入卷积核偏置
#define MAX_PE_Input_fmap_channel 1024 //最大输入特征图通道
#define MAX_PE_Input_fmap_size 320 //最大输入特征图尺寸
#define MAX_PE_Outputput_fmap_channel 1024 //最大输出特征图通道
#define MAX_PE_Output_fmap_size 320 //最大输出特征图尺寸

void PE_dense(
	/*PE控制寄存器*/
	reg_t* PE_Register,
	/*PE全连接神经元权重*/
	data_t* PE_Neure_weights,
	/*PE全连接神经元偏置*/
	data_t* PE_Neure_bias,
	/*输入全连接PE的特征图*/
	data_t* PE_Input_fmap,
	/*全连接PE输出特征图*/
	data_t* PE_Output_fmap
);

#endif
