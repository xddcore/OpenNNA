/*FPGA卷积神经网络 PE单元HLS代码*/
/*作者:xddcore*/
/*05/10/2021*/


/*
 * fixup pe init value
 * xddcore 2023/07/12
 * */

/*
 * 每层网络为UINT8输入,UINT8输出;
 * 卷积核权重为Uint8
 */
#include "process_element.h"
#include <cstring>
data_t L1_output_fmap[L1_output_fmap_channel][L1_output_fmap_size][L1_output_fmap_size] = { 0 };
data_t L1_output_fmap_ReLu[L1_output_fmap_channel][L1_output_fmap_size][L1_output_fmap_size] = { 0 };

data_t L2_output_fmap[L2_output_fmap_channel][L2_output_fmap_size][L2_output_fmap_size] = { 0 };

data_t L3_output_fmap[L3_output_fmap_channel][L3_output_fmap_size][L3_output_fmap_size] = { 0 };
data_t L3_output_fmap_ReLu[L3_output_fmap_channel][L3_output_fmap_size][L3_output_fmap_size] = { 0 };


data_t L4_output_fmap[L4_output_fmap_channel][L4_output_fmap_size][L4_output_fmap_size] = { 0 };
data_t L4_flatten_output_map[L5_input_fmap_size] = { 0 };

data_t L5_output_fmap[L5_output_fmap_size] = {};
data_t L5_output_fmap_ReLu[L5_output_fmap_size] = {};

data_t L6_output_fmap[L6_output_fmap_size] = {};
/*核心函数*/
data_t Relu(data_t x)
{
	return (x < 0) ? (data_t)0 : x;
}
void L1_conv2d(
	data_t Kernel[L1_Kernel_number][L1_Kernel_channel][L1_Kernel_size][L1_Kernel_size],
	data_t bias[L1_bias_number],
	data_t input_fmap[L1_input_fmap_channel][L1_input_fmap_size][L1_input_fmap_size],
	data_t output_fmap[L1_output_fmap_channel][L1_output_fmap_size][L1_output_fmap_size]
)
{
	//int fmap_row, fmap_col, fmap_channel; //特征图扫描计算
	int kernel_row, kernel_col, kernel_channel, kernel_number; //卷积核扫描计算
	int shift_col_num;//卷积核列 横移次数
	int shift_row_num;//卷积核行 竖移次数
	int bias_number;//偏置个数,一个output fmap对应一个bias
	//以卷积核为单位进行扫描(扫描顺序由大至小|数量,通道,行,列)
	for (kernel_number = 0; kernel_number < L1_Kernel_number; kernel_number++)
	{
		for (kernel_channel = 0; kernel_channel < L1_Kernel_channel; kernel_channel++)
		{
			for (shift_col_num = 0; shift_col_num < (((L1_input_fmap_size + 2 * L1_conv_padding - L1_Kernel_size) / L1_conv_stride) + 1); shift_col_num++)
			{
				for (shift_row_num = 0; shift_row_num < (((L1_input_fmap_size + 2 * L1_conv_padding - L1_Kernel_size) / L1_conv_stride) + 1); shift_row_num++)
				{
					for (kernel_row = 0; kernel_row < L1_Kernel_size; kernel_row++)
					{
						for (kernel_col = 0; kernel_col < L1_Kernel_size; kernel_col++)
						{
							if ((kernel_row + kernel_col + kernel_channel) == 0)
								output_fmap[kernel_number][shift_col_num][shift_row_num] = bias[kernel_number];
							output_fmap[kernel_number][shift_col_num][shift_row_num] += Kernel[kernel_number][kernel_channel][kernel_row][kernel_col] * input_fmap[kernel_channel][kernel_row + shift_col_num * L1_conv_stride][kernel_col + shift_row_num * L1_conv_stride];
						}
					}
				}
			}
		}
	}
}

void L2_maxpool2d(
	data_t input_fmap[L2_input_fmap_channel][L2_input_fmap_size][L2_input_fmap_size],
	data_t output_fmap[L2_output_fmap_channel][L2_output_fmap_size][L2_output_fmap_size]
)
{
	int poolkernel_row, poolkernel_col; //最大池化核扫描计算
	int shift_col_num;//卷积核列 横移次数
	int shift_row_num;//卷积核行 竖移次数
	int fmap_channel;//特征图通道
	data_t max_value;
	for (fmap_channel = 0; fmap_channel < L2_output_fmap_channel; fmap_channel++)
	{
		for (shift_col_num = 0; shift_col_num < (((L2_input_fmap_size - L2_MaxpoolKernel_size) / L2_MaxpoolKernel_stride) + 1); shift_col_num++)
		{
			for (shift_row_num = 0; shift_row_num < (((L2_input_fmap_size - L2_MaxpoolKernel_size) / L2_MaxpoolKernel_stride) + 1); shift_row_num++)
			{
				for (poolkernel_row = 0; poolkernel_row < L2_MaxpoolKernel_size; poolkernel_row++)
				{
					for (poolkernel_col = 0; poolkernel_col < L2_MaxpoolKernel_size; poolkernel_col++)
					{
						max_value = (poolkernel_row == 0 && poolkernel_col == 0) ? input_fmap[fmap_channel][poolkernel_row + shift_col_num * L2_MaxpoolKernel_stride][poolkernel_col + shift_row_num * L2_MaxpoolKernel_stride] : (input_fmap[fmap_channel][poolkernel_row + shift_col_num * L2_MaxpoolKernel_stride][poolkernel_col + shift_row_num * L2_MaxpoolKernel_stride] > max_value) ? input_fmap[fmap_channel][poolkernel_row + shift_col_num * L2_MaxpoolKernel_stride][poolkernel_col + shift_row_num * L2_MaxpoolKernel_stride] : max_value;
					}
				}
				output_fmap[fmap_channel][shift_col_num][shift_row_num] = max_value;
			}
		}
	}
}

void L3_conv2d(
	data_t Kernel[L3_Kernel_number][L3_Kernel_channel][L3_Kernel_size][L3_Kernel_size],
	data_t bias[L3_bias_number],
	data_t input_fmap[L3_input_fmap_channel][L3_input_fmap_size][L3_input_fmap_size],
	data_t output_fmap[L3_output_fmap_channel][L3_output_fmap_size][L3_output_fmap_size]
)
{
	//int fmap_row, fmap_col, fmap_channel; //特征图扫描计算
	int kernel_row, kernel_col, kernel_channel, kernel_number; //卷积核扫描计算
	int shift_col_num;//卷积核列 横移次数
	int shift_row_num;//卷积核行 竖移次数
	int bias_number;//偏置个数,一个output fmap对应一个bias
	//以卷积核为单位进行扫描(扫描顺序由大至小|数量,通道,行,列)
	for (kernel_number = 0; kernel_number < L3_Kernel_number; kernel_number++)
	{
		for (kernel_channel = 0; kernel_channel < L3_Kernel_channel; kernel_channel++)
		{
			for (shift_col_num = 0; shift_col_num < (((L3_input_fmap_size + 2 * L3_conv_padding - L3_Kernel_size) / L3_conv_stride) + 1); shift_col_num++)
			{
				for (shift_row_num = 0; shift_row_num < (((L3_input_fmap_size + 2 * L3_conv_padding - L3_Kernel_size) / L3_conv_stride) + 1); shift_row_num++)
				{
					for (kernel_row = 0; kernel_row < L3_Kernel_size; kernel_row++)
					{
						for (kernel_col = 0; kernel_col < L3_Kernel_size; kernel_col++)
						{
							if ((kernel_row + kernel_col + kernel_channel) == 0)
								output_fmap[kernel_number][shift_col_num][shift_row_num] = bias[kernel_number];
							output_fmap[kernel_number][shift_col_num][shift_row_num] += Kernel[kernel_number][kernel_channel][kernel_row][kernel_col] * input_fmap[kernel_channel][kernel_row + shift_col_num * L3_conv_stride][kernel_col + shift_row_num * L3_conv_stride];
						}
					}
				}
			}
		}
	}
}

void L4_maxpool2d(
	data_t input_fmap[L4_input_fmap_channel][L4_input_fmap_size][L4_input_fmap_size],
	data_t output_fmap[L4_output_fmap_channel][L4_output_fmap_size][L4_output_fmap_size]
)
{
	int poolkernel_row, poolkernel_col; //最大池化核扫描计算
	int shift_col_num;//卷积核列 横移次数
	int shift_row_num;//卷积核行 竖移次数
	int fmap_channel;//特征图通道
	data_t max_value;
	for (fmap_channel = 0; fmap_channel < L4_output_fmap_channel; fmap_channel++)
	{
		for (shift_col_num = 0; shift_col_num < (((L4_input_fmap_size - L4_MaxpoolKernel_size) / L4_MaxpoolKernel_stride) + 1); shift_col_num++)
		{
			for (shift_row_num = 0; shift_row_num < (((L4_input_fmap_size - L4_MaxpoolKernel_size) / L4_MaxpoolKernel_stride) + 1); shift_row_num++)
			{
				for (poolkernel_row = 0; poolkernel_row < L4_MaxpoolKernel_size; poolkernel_row++)
				{
					for (poolkernel_col = 0; poolkernel_col < L4_MaxpoolKernel_size; poolkernel_col++)
					{
						max_value = (poolkernel_row == 0 && poolkernel_col == 0) ? input_fmap[fmap_channel][poolkernel_row + shift_col_num * L4_MaxpoolKernel_stride][poolkernel_col + shift_row_num * L4_MaxpoolKernel_stride] : (input_fmap[fmap_channel][poolkernel_row + shift_col_num * L4_MaxpoolKernel_stride][poolkernel_col + shift_row_num * L4_MaxpoolKernel_stride] > max_value) ? input_fmap[fmap_channel][poolkernel_row + shift_col_num * L4_MaxpoolKernel_stride][poolkernel_col + shift_row_num * L4_MaxpoolKernel_stride] : max_value;
					}
				}
				output_fmap[fmap_channel][shift_col_num][shift_row_num] = max_value;
			}
		}
	}
}

void L5_dense(
	data_t neure_weights[L5_neure_number][L5_input_fmap_size],
	data_t neure_bias[L5_neure_number],
	data_t input_fmap[L5_input_fmap_size],
	data_t output_fmap[L5_output_fmap_size]
)
{
	int input_fmap_size; //输入一维特征图大小
	int weights_bias_number;//权重和偏置计数(一个neura->1 weights + 1 bias)
	for (weights_bias_number = 0; weights_bias_number < L5_neure_number; weights_bias_number++)
	{
		for (input_fmap_size = 0; input_fmap_size < L5_input_fmap_size; input_fmap_size++)
		{
			if (input_fmap_size == 0)
				output_fmap[weights_bias_number] = neure_bias[weights_bias_number];
			output_fmap[weights_bias_number] += (input_fmap[input_fmap_size] * neure_weights[weights_bias_number][input_fmap_size]);
		}
	}
}

void L6_dense(
	data_t neure_weights[L6_neure_number][L6_input_fmap_size],
	data_t neure_bias[L6_neure_number],
	data_t input_fmap[L6_input_fmap_size],
	data_t output_fmap[L6_output_fmap_size]
)
{
	int input_fmap_size; //输入一维特征图大小
	int weights_bias_number;//权重和偏置计数(一个neura->1 weights + 1 bias)
	for (weights_bias_number = 0; weights_bias_number < L6_neure_number; weights_bias_number++)
	{
		for (input_fmap_size = 0; input_fmap_size < L6_input_fmap_size; input_fmap_size++)
		{
			if (input_fmap_size == 0)
				output_fmap[weights_bias_number] = neure_bias[weights_bias_number];
			output_fmap[weights_bias_number] += (input_fmap[input_fmap_size] * neure_weights[weights_bias_number][input_fmap_size]);
		}
	}
}

int process_element(
		data_t *input_fmap[L1_input_fmap_size*L1_input_fmap_size],
		data_t *output_fmap[L6_output_fmap_size]
					)
{
#pragma HLS INTERFACE m_axi port=input_fmap depth=1024 offset=slave bundle=INPUT
#pragma HLS INTERFACE m_axi port=output_fmap depth=10 offset=slave bundle=OUTPUT
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

	data_t L1_input_fmap[L1_input_fmap_channel][L1_input_fmap_size][L1_input_fmap_size];
	memcpy(L1_input_fmap,(const data_t*)input_fmap,L1_input_fmap_size*L1_input_fmap_size*sizeof(data_t));
	/*计算Conv1结果*/
	L1_conv2d(L1_Kernel, L1_bias, L1_input_fmap, L1_output_fmap);
	/*将Conv1结果用ReLu激励函数激励*/
	for (int i = 0; i < L1_output_fmap_channel; i++)
	{
		for (int j = 0; j < L1_output_fmap_size; j++)
		{
			for (int k = 0; k < L1_output_fmap_size; k++)
			{
				L1_output_fmap_ReLu[i][j][k] = Relu(L1_output_fmap[i][j][k]);
			}
		}
	}

	/*计算最大池化结果*/
	L2_maxpool2d(L1_output_fmap_ReLu, L2_output_fmap);
	/*计算Conv3结果*/
	L3_conv2d(L3_Kernel, L3_bias, L2_output_fmap, L3_output_fmap);
	/*将Conv3结果用ReLu激励函数激励*/
	for (int i = 0; i < L3_output_fmap_channel; i++)
	{
		for (int j = 0; j < L3_output_fmap_size; j++)
		{
			for (int k = 0; k < L3_output_fmap_size; k++)
			{
				L3_output_fmap_ReLu[i][j][k] = Relu(L3_output_fmap[i][j][k]);
			}
		}
	}
	/*计算最大池化结果*/
	L4_maxpool2d(L3_output_fmap_ReLu, L4_output_fmap);
	/*将最大池化结果进行Flatten*/
	int flatten_i = 0;//flatten 操作参数
	for (int i = 0; i < L4_output_fmap_size; i++)
	{
		for (int j = 0; j < L4_output_fmap_size; j++)
		{
			for (int k = 0; k < L4_output_fmap_channel; k++)
			{
				L4_flatten_output_map[flatten_i] = L4_output_fmap[k][i][j];//(为了配合dense权重形状)按通道方向展开, 顺序第一通道第一行第一列,第二通道第一行第一列
				flatten_i++;
			}
		}
	}
	/*计算Dense5结果*/
	L5_dense(L5_neure_weights, L5_neure_bias, L4_flatten_output_map, L5_output_fmap);
	/*将Dense5结果用ReLu激励函数激励*/
	for (int i = 0; i < L5_output_fmap_size; i++)
	{
		L5_output_fmap_ReLu[i] = Relu(L5_output_fmap[i]);
	}
	/*计算Dense6结果*/
	L6_dense(L6_neure_weights, L6_neure_bias, L5_output_fmap_ReLu, L6_output_fmap);
	/*输出Dense6结果*/
	memcpy(output_fmap,L6_output_fmap,L6_output_fmap_size*sizeof(data_t));
	/*计算Dense6结果放入sigmoid函数,因为需要y=exp(x)函数,fpga不好计算,所以放入arm端进行*/

	return 0;

}
