#include "PE_Pool.h"

void PE_pool(
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
	int poolkernel_row, poolkernel_col; //最大池化核扫描计算
	int shift_col_num;//卷积核列 横移次数
	int shift_row_num;//卷积核行 竖移次数
	int fmap_channel;//特征图通道
	data_t max_value;//在MaxPool时存储最大值,在AveragePool存储均值
	for (fmap_channel = 0; fmap_channel < PE_Register[3]; fmap_channel++)//output_fmap_channel
	{
		for (shift_col_num = 0; shift_col_num < (((PE_Register[0] - PE_Register[4]) / PE_Register[5]) + 1); shift_col_num++)//(((input_fmap_size - pool_Kernel_size) / pool_Kernel_stride) + 1)
		{
			for (shift_row_num = 0; shift_row_num < (((PE_Register[0] - PE_Register[4]) / PE_Register[5]) + 1); shift_row_num++)//(((input_fmap_size - pool_Kernel_size) / pool_Kernel_stride) + 1)
			{
				for (poolkernel_row = 0; poolkernel_row < PE_Register[4]; poolkernel_row++)//pool_Kernel_size
				{
					for (poolkernel_col = 0; poolkernel_col < PE_Register[4]; poolkernel_col++)//pool_Kernel_size
					{
						if(PE_Register[6] ==0)//Maxpool模式
							max_value = (poolkernel_row == 0 && poolkernel_col == 0) ? PE_Input_fmap[(fmap_channel*PE_Register[0]*PE_Register[0]) + ((poolkernel_row + shift_col_num * PE_Register[5]) * PE_Register[0]) + (poolkernel_col + shift_row_num * PE_Register[5])] : (PE_Input_fmap[(fmap_channel*PE_Register[0]*PE_Register[0]) + ((poolkernel_row + shift_col_num * PE_Register[5]) * PE_Register[0]) + (poolkernel_col + shift_row_num * PE_Register[5])] > max_value) ? PE_Input_fmap[(fmap_channel*PE_Register[0]*PE_Register[0]) + ((poolkernel_row + shift_col_num * PE_Register[5]) * PE_Register[0]) + (poolkernel_col + shift_row_num * PE_Register[5])] : max_value;
						else if (PE_Register[6] == 1)//Average Pool模式
						{
							//赋初值
							if ((poolkernel_row == 0 && poolkernel_col == 0))
								max_value = PE_Input_fmap[(fmap_channel * PE_Register[0] * PE_Register[0]) + ((poolkernel_row + shift_col_num * PE_Register[5]) * PE_Register[0]) + (poolkernel_col + shift_row_num * PE_Register[5])];
							else//求累加值
								max_value += PE_Input_fmap[(fmap_channel * PE_Register[0] * PE_Register[0]) + ((poolkernel_row + shift_col_num * PE_Register[5]) * PE_Register[0]) + (poolkernel_col + shift_row_num * PE_Register[5])];
						}
					}
				}
				if (PE_Register[6] == 0)//Maxpool模式
					PE_Output_fmap[(fmap_channel*PE_Register[2]*PE_Register[2]) + (shift_col_num*PE_Register[2]) + shift_row_num] = max_value;
				else if(PE_Register[6] == 1)//Average Pool模式
					PE_Output_fmap[(fmap_channel * PE_Register[2] * PE_Register[2]) + (shift_col_num * PE_Register[2]) + shift_row_num] = (max_value/(PE_Register[4]*PE_Register[4]));//取平均
			}
		}
	}
}
