#include "PE_Activation.h"
#include "math.h"
void PE_activation(
	/*PE控制寄存器*/
	reg_t* PE_Register,
	/*输入卷积核特征图*/
	data_t* PE_Input_fmap,
	/*PE输出特征图*/
	data_t* PE_Output_fmap
)
{
#pragma HLS INTERFACE m_axi bundle=INPUT_Reg depth=6 port=PE_Register offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Fmap depth=104857600 port=PE_Input_fmap offset=slave
#pragma HLS INTERFACE m_axi bundle=OUTPUT_Fmap depth=104857600 port=PE_Output_fmap offset=slave
#pragma HLS INTERFACE s_axilite bundle=CTRL_BUS port=return
	for (int i = 0; i < PE_Register[1]; i++)//input_fmap_channel
	{
		for (int j = 0; j < PE_Register[2]; j++)//output_fmap_size 行
		{
			for (int k = 0; k < PE_Register[2]; k++)//output_fmap_size 列
			{
				if (PE_Register[4] == 0)//ReLu
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] > 0 ? PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] : (data_t)0;
				}
				else if (PE_Register[4] == 1)//ReLU6 防止低精度时,比如Fixed point时,整数部分溢出
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] > 0 ? (PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k]<6 ? PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k]:(data_t)6): (data_t)0;
				}
				else if (PE_Register[4] == 2)//LeakyReLU max(0.01x,x)
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] > 0 ? PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] : (data_t)(((data_t)0.01)*PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k]);
				}
				//else if (PE_Register[4] == 3)//Sigmoid
				//{
				//	PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = (data_t)1.0/((data_t)1.0+(data_t)exp((float)(-PE_Input_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k])));
				//}
			}
		}
	}
}
