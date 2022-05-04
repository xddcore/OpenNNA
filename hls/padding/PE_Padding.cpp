#include "PE_Padding.h"

void PE_padding(
	/*PE控制寄存器*/
	reg_t* PE_Register,
	/*输入卷积核特征图*/
	data_t* PE_Input_fmap,
	/*PE输出特征图*/
	data_t* PE_Output_fmap
)
{
#pragma HLS INTERFACE m_axi bundle=INPUT_Reg depth=5 port=PE_Register offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Fmap depth=104857600 port=PE_Input_fmap offset=slave
#pragma HLS INTERFACE m_axi bundle=OUTPUT_Fmap depth=104857600 port=PE_Output_fmap offset=slave
#pragma HLS INTERFACE s_axilite bundle=CTRL_BUS port=return

	for (int i = 0; i < PE_Register[1]; i++)//input_fmap_channel
	{
		for (int j = 0; j < PE_Register[2]; j++)//output_fmap_size 行
		{
			for (int k = 0; k < PE_Register[2]; k++)//output_fmap_size 列
			{
				/*j遍历行,顶部和底部填充*/
				if ( j < PE_Register[4])//顶部padding
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = 0;
				}
				else if (j >= (PE_Register[4] + PE_Register[0]))//底部padding
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = 0;
				}
				/*k遍历列,左边和右边填充*/
				if (k < PE_Register[4])//左边padding
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = 0;
				}
				else if (k >= (PE_Register[4] + PE_Register[0]))//右边padding
				{
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = 0;
				}
				//输入fmap填充到中间
				if(j >= PE_Register[4] && j < (PE_Register[4] + PE_Register[0]) && k >= PE_Register[4] && k < (PE_Register[4] + PE_Register[0]))
					PE_Output_fmap[(i * PE_Register[2] * PE_Register[2]) + (j * PE_Register[2]) + k] = PE_Input_fmap[(i * PE_Register[0] * PE_Register[0]) + ((j-PE_Register[4]) * PE_Register[0]) + (k-PE_Register[4])];
			}
		}
	}
}
