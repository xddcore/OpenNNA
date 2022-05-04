#include "PE_Dense.h"


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
)
{
#pragma HLS INTERFACE m_axi bundle=INPUT_Reg depth=6 port=PE_Register offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Fmap depth=104857600 port=PE_Input_fmap offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Neure_weights depth=307200 port=PE_Neure_weights offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUTNeure_bias depth=1024 port=PE_Neure_bias offset=slave
#pragma HLS INTERFACE m_axi bundle=OUTPUT_Fmap depth=104857600 port=PE_Output_fmap offset=slave
#pragma HLS INTERFACE s_axilite bundle=CTRL_BUS port=return
	//int input_fmap_size; //输入一维特征图大小
	//int weights_bias_number;//权重和偏置计数(一个neura->1 weights + 1 bias)
	for (int weights_bias_number = 0; weights_bias_number < PE_Register[4]; weights_bias_number++)//neure_number
	{
		/*+bias*/
		/*非常重要:清空计算中间量 下面0,0,0位置等于权重,对于多次计算来说,清空了上一次的计算结果,避免影响本次计算*/
		PE_Output_fmap[weights_bias_number] = PE_Neure_bias[weights_bias_number];
		/**weights*/
		//对于全连接网络来说,它是一维的,所以input_fmap_size=input_fmap_size*input_fmap_size*input_fmap_channel
		//在这一步顺便将多唯索引压成一维来访问
		for (int input_fmap_size = 0; input_fmap_size < (PE_Register[0]*PE_Register[0]* PE_Register[1]); input_fmap_size++)//对于全连接网络来说,它是一维的,所以input_fmap_size=input_fmap_size*input_fmap_size*input_fmap_channel
		{
			/*+bias*/
			/*非常重要:清空计算中间量 下面0,0,0位置等于权重,对于多次计算来说,清空了上一次的计算结果,避免影响本次计算*/
			//if (input_fmap_size == 0)
			//	PE_Output_fmap[weights_bias_number] = PE_Neure_bias[weights_bias_number];
			PE_Output_fmap[weights_bias_number] += (PE_Input_fmap[input_fmap_size] * PE_Neure_weights[(weights_bias_number*(PE_Register[0] * PE_Register[0] * PE_Register[1]))+input_fmap_size]); //+ neure_bias[weights_bias_number];

		}
	}
}
