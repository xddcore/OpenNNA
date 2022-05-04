#include "PE_Conv.h"

/*Weights Buffer以卷积核为单元*/
/*对每个卷积核的计算进行控制,更能提高灵活度和性能*/
/*Fmap Buffer以一个卷积核输出的一个fmap的数据为单元*/
/*Register Define*/
//0:Kernel_size
//1:Kernel_channel
//2:Kernel_number
//3:Kernel_bias_number
//4:conv_stride
//5:conv_padding
//6:input_fmap_size
//7:input_fmap_channel
//8:output_fmap_size
//9:output_fmap_channel
//10:Calculate Mode (计算模式:普通卷积,深度卷积,逐点卷积,MobileNET V2卷积

void PE_conv2d_cal(
	reg_t Register[MAX_PE_Register_NUM],
	data_t BUFFER_Kernel[MAX_PE_Kernel_channel][MAX_PE_Kernel_size][MAX_PE_Kernel_size],
	data_t BUFFER_Bias[MAX_BUFFER_Bias_number],
	data_t BUFFER_Input_fmap[MAX_PE_Input_fmap_channel][MAX_BUFFER_Input_fmap_size][MAX_BUFFER_Input_fmap_size],
	data_t BUFFER_Output_fmap[MAX_BUFFER_Output_fmap_size]
)
{
	int kernel_row, kernel_col, kernel_channel, kernel_number; //卷积核扫描计算
	int shift_col_num;//卷积核列 横移次数
	int shift_row_num;//卷积核行 竖移次数
	int bias_number;//偏置个数,一个output fmap对应一个bias
	//以卷积核为单位进行扫描(扫描顺序由大至小|数量,通道,行,列)
	LOOP_cal_Data:
	for (kernel_channel = 0; kernel_channel < ((Register[10]==0)?Register[1]: 1); kernel_channel++)//0:普通卷积Kernel_channel=Kernel_channel(卷积通道) 1:逐通道卷积Kernel_channel=1 1个卷积核只有第一个通道卷积第一个通道的fmap//Kernel_number(卷积核数量)
	{
		for (kernel_row = 0; kernel_row < Register[0]; kernel_row++)//Kernel_size
		{
			for (kernel_col = 0; kernel_col < Register[0]; kernel_col++)//Kernel_size
			{
				/*+bias*/
				/*非常重要:清空计算中间量 下面0,0,0位置等于权重,对于多次计算来说,清空了上一次的计算结果,避免影响本次计算*/
				if ((kernel_row + kernel_col + kernel_channel) == 0)
				{
					BUFFER_Output_fmap[MAX_BUFFER_Output_fmap_size-1] = BUFFER_Bias[MAX_BUFFER_Bias_number-1];
				}
				/**weight*/
				BUFFER_Output_fmap[MAX_BUFFER_Output_fmap_size-1] += BUFFER_Kernel[((Register[10] == 0) ? kernel_channel : 0)][kernel_row][kernel_col] * BUFFER_Input_fmap[kernel_channel][kernel_row][kernel_col];
			}
		}
	}
}
/*读取一个BUFFER的输入特征图和1个Kernel的数据*/
void PE_load_Data_Weights_Buffer(
	/*从DDR里面取的一维数据*/
	reg_t PE_Register[MAX_PE_Register_NUM],
	reg_t Reg_Inside_Kernel_number,//缓冲的卷积核序号
	reg_t shift_row,//input fmap竖移方向坐标 也就是行坐标
	reg_t shift_col,//input fmap横移方向坐标 也就是列坐标
	data_t PE_Kernel[MAX_PE_Kernel_number * MAX_PE_Kernel_channel * MAX_PE_Kernel_size * MAX_PE_Kernel_size],
	data_t PE_Bias[MAX_PE_Bias_number],
	data_t PE_Input_fmap[MAX_PE_Input_fmap_channel * MAX_PE_Input_fmap_size * MAX_PE_Input_fmap_size],
	/*生成的正常维度Buffer数据*/
	data_t BUFFER_Kernel[MAX_PE_Kernel_channel][MAX_PE_Kernel_size][MAX_PE_Kernel_size],
	data_t BUFFER_Bias[MAX_BUFFER_Bias_number],
	data_t BUFFER_Input_fmap[MAX_PE_Input_fmap_channel][MAX_BUFFER_Input_fmap_size][MAX_BUFFER_Input_fmap_size]
)
{
	LOOP_Load_Data:
	for (int i = 0; i < PE_Register[1]; i++)//Kernel_channel
	{
		for (int j = 0; j < PE_Register[0]; j++)//Kernel_size
		{
			for (int k = 0; k < PE_Register[0]; k++)//Kernel_size
			{
				BUFFER_Bias[MAX_BUFFER_Bias_number - 1] = PE_Bias[Reg_Inside_Kernel_number];
				BUFFER_Kernel[i][j][k] = PE_Kernel[(Reg_Inside_Kernel_number*PE_Register[1]*PE_Register[0]*PE_Register[0]) + (i*PE_Register[0]*PE_Register[0]) + (j*PE_Register[0]) + (k)];
				BUFFER_Input_fmap[i][j][k] = PE_Input_fmap[(shift_col * PE_Register[4]) + (shift_row * PE_Register[4] * PE_Register[6]) + (i*PE_Register[6]*PE_Register[6]) + (j*PE_Register[6]) + (k)];
			}
		}
	}
}
/*存储计算结果到PE输出的方fmap中(计算结果存储位置与shift_col和shift_row相关*/
void PE_store_Output_fmap(
	/*PE控制寄存器*/
	reg_t PE_Register[MAX_PE_Register_NUM],
	reg_t Reg_Inside_Kernel_number,//结果的来源(卷积核序号)-<决定存放到PE输出特征图的通道>
	reg_t shift_row,//input fmap竖移方向坐标 也就是行坐标
	reg_t shift_col,//input fmap横移方向坐标 也就是列坐标
	/*Buffer算出来的数据(一个点)*/
	data_t BUFFER_Output_fmap[MAX_BUFFER_Output_fmap_size],
	/*PE输出的计算结果*/
	data_t PE_Output_fmap[MAX_PE_Outputput_fmap_channel * MAX_PE_Output_fmap_size * MAX_PE_Output_fmap_size]
)
{
	PE_Output_fmap[(Reg_Inside_Kernel_number*PE_Register[8]*PE_Register[8]) + (shift_row*PE_Register[8])+ (shift_col)] = BUFFER_Output_fmap[MAX_BUFFER_Output_fmap_size - 1];
}
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
)
{
#pragma HLS INTERFACE m_axi bundle=INPUT_Reg depth=13 port=PE_Register offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Kernel depth=9216 port=PE_Kernel offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Bias depth=1024 port=PE_Bias offset=slave
#pragma HLS INTERFACE m_axi bundle=INPUT_Fmap depth=104857600 port=PE_Input_fmap offset=slave
#pragma HLS INTERFACE m_axi bundle=OUTPUT_Fmap depth=104857600 port=PE_Output_fmap offset=slave
#pragma HLS INTERFACE s_axilite bundle=CTRL_BUS port=return

	/*双缓冲*/
	/*Buffer1*/
	data_t BUFFER1_Kernel[MAX_PE_Kernel_channel][MAX_PE_Kernel_size][MAX_PE_Kernel_size];
	data_t BUFFER1_Bias[MAX_BUFFER_Bias_number];
	data_t BUFFER1_Input_fmap[MAX_PE_Input_fmap_channel][MAX_BUFFER_Input_fmap_size][MAX_BUFFER_Input_fmap_size];
	data_t BUFFER1_Output_fmap[MAX_BUFFER_Output_fmap_size];
	/*Buffer2*/
	data_t BUFFER2_Kernel[MAX_PE_Kernel_channel][MAX_PE_Kernel_size][MAX_PE_Kernel_size];
	data_t BUFFER2_Bias[MAX_BUFFER_Bias_number];
	data_t BUFFER2_Input_fmap[MAX_PE_Input_fmap_channel][MAX_BUFFER_Input_fmap_size][MAX_BUFFER_Input_fmap_size];
	data_t BUFFER2_Output_fmap[MAX_BUFFER_Output_fmap_size];
	//(等价于i)reg_t Reg_Inside_Kernel_number = 0;//结果的来源(卷积核序号)-<决定存放到PE输出特征图的通道>
	//(等价于j)reg_t shift_row = 0; //input fmap竖移坐标
	//(等价于k)reg_t shift_col = 0; //input fmap横移坐标

	LOOP_Kernel_number:
	for (reg_t Reg_Inside_Kernel_number = 0; Reg_Inside_Kernel_number < PE_Register[2]; Reg_Inside_Kernel_number++)//Kernel_number
	{
		/*缓冲控制位*/
		bool BUFFER_CONTROL = true;
		bool BUFFER_START = true;//每个卷积核开始时填充BUFFER
		if (BUFFER_START == true)
		{
			/*从DDR里面取一个Buffer1的数据*/
			PE_load_Data_Weights_Buffer(
				/*从DDR里面取的一维数据*/
				PE_Register,
				Reg_Inside_Kernel_number,//缓冲的卷积核序号
				0,//input fmap竖移方向坐标 也就是行坐标
				0,//input fmap横移方向坐标 也就是列坐标
				PE_Kernel,
				PE_Bias,
				PE_Input_fmap,
				/*生成的正常维度Buffer数据*/
				BUFFER1_Kernel,
				BUFFER1_Bias,
				BUFFER1_Input_fmap
			);
		}
		LOOP_Shift_row:
		for (reg_t shift_row = 0; shift_row < PE_Register[8]; shift_row++)//output_fmap_size
		{
			LOOP_Shift_col:
			for (reg_t shift_col = 0; shift_col < PE_Register[8]; shift_col++)//output_fmap_size
			{
				/*切换加载数据*/
				/*index +1不会造成越界后的风险,因为最后一个buffer只是被越界访问,并没有被越界计算存储*/
				/*但是为了保险,防止FPGA硬件电路越界访问BRAM造成不确定错误,需要添加if语句防止越界访问的产生*/
				if (BUFFER_CONTROL)
				{
					/*从DDR里面取一个Buffer2的数据*/
					if((shift_col + 1) != PE_Register[8])//防止越界访问
					PE_load_Data_Weights_Buffer(
						/*从DDR里面取的一维数据*/
						PE_Register,
						Reg_Inside_Kernel_number,//缓冲的卷积核序号
						shift_row,//input fmap竖移方向坐标 也就是行坐标
						shift_col + 1,//input fmap横移方向坐标 也就是列坐标
						PE_Kernel,
						PE_Bias,
						PE_Input_fmap,
						/*生成的正常维度Buffer数据*/
						BUFFER2_Kernel,
						BUFFER2_Bias,
						BUFFER2_Input_fmap
					);
				}
				else
				{
					/*从DDR里面取一个Buffer1的数据*/
					if ((shift_col + 1) != PE_Register[8])//防止越界访问
					PE_load_Data_Weights_Buffer(
						/*从DDR里面取的一维数据*/
						PE_Register,
						Reg_Inside_Kernel_number,//缓冲的卷积核序号
						shift_row,//input fmap竖移方向坐标 也就是行坐标
						shift_col + 1,//input fmap横移方向坐标 也就是列坐标
						PE_Kernel,
						PE_Bias,
						PE_Input_fmap,
						/*生成的正常维度Buffer数据*/
						BUFFER1_Kernel,
						BUFFER1_Bias,
						BUFFER1_Input_fmap
					);
				}

				if (BUFFER_CONTROL)
				{
					/*计算一个Buffer1的数据*/
					PE_conv2d_cal(PE_Register, BUFFER1_Kernel, BUFFER1_Bias, BUFFER1_Input_fmap, BUFFER1_Output_fmap);
					/*存储一个Buffer1的数据回DDR*/
					PE_store_Output_fmap(
						PE_Register,
						Reg_Inside_Kernel_number,//结果的来源(卷积核序号)-<决定存放到PE输出特征图的通道>
						shift_row,//input fmap横移坐标
						shift_col,//input fmap竖移坐标
						/*Buffer算出来的数据(一个点)*/
						BUFFER1_Output_fmap,
						/*PE输出的计算结果*/
						PE_Output_fmap
					);
					BUFFER_CONTROL = false;
				}
				else
				{
					/*计算一个Buffer2的数据*/
					PE_conv2d_cal(PE_Register, BUFFER2_Kernel, BUFFER2_Bias, BUFFER2_Input_fmap, BUFFER2_Output_fmap);
					/*存储一个Buffer2的数据回DDR*/
					PE_store_Output_fmap(
						PE_Register,
						Reg_Inside_Kernel_number,//结果的来源(卷积核序号)-<决定存放到PE输出特征图的通道>
						shift_row,//input fmap横移坐标
						shift_col,//input fmap竖移坐标
						/*Buffer算出来的数据(一个点)*/
						BUFFER2_Output_fmap,
						/*PE输出的计算结果*/
						PE_Output_fmap
					);
					BUFFER_CONTROL = true;
				}
			}
		}
	}
}
