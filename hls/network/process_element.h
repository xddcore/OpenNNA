#ifndef _PROCESS_ELEMENT_H_
#define _PROCESS_ELEMENT_H_

#include <ap_int.h>
#include <ap_fixed.h>

typedef ap_fixed<16,8,AP_RND,AP_SAT> data_t;

/*L1 Conv2d*/
#define L1_Kernel_size 3 //卷积核大小
#define L1_Kernel_channel 1 //卷积核通道
#define L1_Kernel_number 16 //卷积核数量
#define L1_bias_number 16  //偏置数量
#define L1_input_fmap_size 32 //输入特征图大小
#define L1_input_fmap_channel 1 //输入特征图通道
#define L1_output_fmap_size (((L1_input_fmap_size+2*L1_conv_padding-L1_Kernel_size)/ L1_conv_stride)+1)+L1_output_padding //输出特征图数量+输出Padding
#define L1_output_fmap_channel 16 //输出特征图通道
#define L1_conv_padding 0//卷积前填充,在边缘上填充n圈0(未实现)
#define L1_conv_stride 1//卷积核步长
#define L1_output_padding 0//输出padding //还需要修改卷积核的一个三目运算符条件

/*L2 Maxpool2D*/
#define L2_MaxpoolKernel_size 2 //池化核大小
#define L2_MaxpoolKernel_stride 2//池化核步长
#define L2_input_fmap_size 30 //输入特征图大小
#define L2_input_fmap_channel 16 //输入特征图通道
#define L2_output_fmap_size (((L2_input_fmap_size-L2_MaxpoolKernel_size)/ L2_MaxpoolKernel_stride)+1) //输出特征图数量
#define L2_output_fmap_channel 16 //输出特征图通道

/*L3 Conv2D*/
#define L3_Kernel_size 3 //卷积核大小
#define L3_Kernel_channel 16 //卷积核通道
#define L3_Kernel_number 32 //卷积核数量
#define L3_bias_number 32  //偏置数量
#define L3_input_fmap_size 15 //输入特征图大小
#define L3_input_fmap_channel 16 //输入特征图通道
#define L3_output_fmap_size (((L3_input_fmap_size+2*L3_conv_padding-L3_Kernel_size)/ L3_conv_stride)+1)+L3_output_padding //输出特征图数量+输出Padding
#define L3_output_fmap_channel 32 //输出特征图通道
#define L3_conv_padding 0//卷积前填充,在边缘上填充n圈0(未实现)
#define L3_conv_stride 1//卷积核步长
#define L3_output_padding 0//输出padding //还需要修改卷积核的一个三目运算符条件

/*L4 Maxpool2D*/
#define L4_MaxpoolKernel_size 2 //池化核大小
#define L4_MaxpoolKernel_stride 2//池化核步长
#define L4_input_fmap_size 13 //输入特征图大小
#define L4_input_fmap_channel 32 //输入特征图通道
#define L4_output_fmap_size (((L4_input_fmap_size-L4_MaxpoolKernel_size)/ L4_MaxpoolKernel_stride)+1) //输出特征图数量
#define L4_output_fmap_channel 32 //输出特征图通道

/*L5 Dense*/
#define L5_input_fmap_size 1152 //全连接层输入特征图大小
#define L5_neure_number 64 //神经元数量
#define L5_output_fmap_size 64 //输出特征图大小

/*L6 Dense*/
#define L6_input_fmap_size  64 //全连接层输入特征图大小
#define L6_neure_number  10 //神经元数量
#define L6_output_fmap_size  10 //输出特征图大小

#include "conv1_weights_bias.h"
#include "conv3_weights_bias.h"
#include "dense5_weights_bias.h"
#include "dense6_weights_bias.h"
int process_element(
		data_t *input_fmap[L1_input_fmap_size*L1_input_fmap_size],
		data_t *output_fmap[L6_output_fmap_size]
					);

#endif
