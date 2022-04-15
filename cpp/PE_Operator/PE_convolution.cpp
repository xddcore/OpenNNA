// PE_convolution.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<ctime> //获取程序运行时间，评估性能
#include <windows.h>//头文件
using namespace std;
typedef float data_t;//计算参数类型
typedef char reg_t;//控制寄存器参数类型

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
/*Conv2d*/
//#define Kernel_size 3 //卷积核大小
//#define Kernel_channel 1 //卷积核通道
//#define Kernel_number 16 //卷积核数量
//#define Kernel_bias_number 16  //偏置数量
//#define conv_stride 1//卷积核步长
//#define conv_padding 0//卷积前填充,在边缘上填充n圈0(未实现)

//#define input_fmap_size 32 //输入特征图大小
//#define input_fmap_channel 1 //输入特征图通道
//#define output_fmap_size (((input_fmap_size+2*conv_padding-Kernel_size)/ conv_stride)+1) //输出特征图数量
//#define output_fmap_channel 16 //输出特征图通道

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
//10:Calculate Mode (计算模式:0:普通卷积,1:逐通道卷积)
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
		for (reg_t shift_row = 0; shift_row < PE_Register[8]; shift_row++)//output_fmap_size |-1是因为buffer2 已经先一步+1,防止越界访问
		{
			for (reg_t shift_col = 0; shift_col < PE_Register[8]; shift_col++)//output_fmap_size|-1是因为buffer2 已经先一步+1,防止越界访问
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

data_t PE_Kernel[MAX_PE_Kernel_number][MAX_PE_Kernel_channel][MAX_PE_Kernel_size][MAX_PE_Kernel_size] = {
	{
		{
			{1,1,1},//0 channel 
			{1,1,1},
			{1,1,1},
		},
		{
			{2,2,2},
			{2,2,2},
			{2,2,2},
		},
	},
	{
		{
			{3,3,3},//0 channel 
			{3,3,3},
			{3,3,3},
		},
		{
			{4,4,4},
			{4,4,4},
			{4,4,4},
		},
	},
};
data_t PE_Bias[MAX_PE_Bias_number] = { 1.0,2.0 };
data_t PE_Input_fmap[MAX_PE_Input_fmap_channel][MAX_PE_Input_fmap_size][MAX_PE_Input_fmap_size] = {
		{
			{1,1,1,2,2,2,3},
			{1,1,1,2,2,2,3},
			{1,1,1,2,2,2,3},
			{4,4,4,5,5,5,6},
			{4,4,4,5,5,5,6},
			{4,4,4,5,5,5,6},
			{7,7,7,8,8,8,9},
		},
		{
			{1,1,1,2,2,2,3},
			{1,1,1,2,2,2,3},
			{1,1,1,2,2,2,3},
			{4,4,4,5,5,5,6},
			{4,4,4,5,5,5,6},
			{4,4,4,5,5,5,6},
			{7,7,7,8,8,8,9},
		},
};

data_t PE_Kernel_2[MAX_PE_Kernel_number * MAX_PE_Kernel_channel * MAX_PE_Kernel_size * MAX_PE_Kernel_size];
data_t PE_Input_fmap_2[MAX_PE_Input_fmap_channel * MAX_PE_Input_fmap_size * MAX_PE_Input_fmap_size];
data_t PE_Output_fmap[MAX_PE_Outputput_fmap_channel * MAX_PE_Output_fmap_size * MAX_PE_Output_fmap_size];


int main()
{
	/*Register Define*/
	//0:Kernel_size
	//1:Kernel_channel
	//2:Kernel_number
	//3:Kernel_bias_number
	//4:conv_stride
	//5:conv_padding //废弃不用,在PS里面或者专门的Padding IP Core核进行Padding
	//6:input_fmap_size
	//7:input_fmap_channel
	//8:output_fmap_size
	//9:output_fmap_channel
	/*计算模式废弃不用,
	深度可分离卷积分为逐通道卷积(一个卷积核只有一个chanel,卷积一维数据)和逐点卷积(合并特征,提升维度)
	逐通道卷积可以调用n次ip core实现(n=输入通道数) !!!效率太低
	逐点卷积都可直接用1*1卷积核实现.
	计算模式可以由ps驱动
	Mobilenet v2的卷积结构称为bottleneck layer(瓶颈层,是一种inverted residual block(倒置残差模块，先进行channel扩张，然后进行channel缩减) 
	expansion layer 和projection layer的深维和降维可以直接用1*1的卷积核实现.
	expansion layer主要是为了解决Relu激活函数导致低维度数据丢失问题
	projection layer减少了计算量,然后加上expansion layer之前的输入,不要非线性函数
	*/
	/**/
	//10:Calculate Mode (计算模式:0:普通卷积,1:逐通道卷积)
	clock_t startTime, endTime;
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	LARGE_INTEGER t2;
	double dt;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&t1);
	reg_t Kernel_size = 3; //卷积核大小
	reg_t Kernel_channel = 2; //卷积核通道
	reg_t Kernel_number = 2; //卷积核数量
	reg_t Kernel_bias_number = 2;  //偏置数量
	reg_t conv_stride = 1;//卷积核步长
	reg_t conv_padding = 0;//卷积前填充,在边缘上填充n圈0(未实现)//废弃不用,在PS里面或者专门的Padding IP Core核进行Padding

	reg_t input_fmap_size = 7; //输入特征图大小
	reg_t input_fmap_channel = 2; //输入特征图通道
	reg_t output_fmap_size = (((input_fmap_size + 2 * conv_padding - Kernel_size) / conv_stride) + 1); //输出特征图数量
	reg_t output_fmap_channel = 2; //输出特征图通道
	reg_t calculate_mode = 1; //(计算模式:0:普通卷积,1:逐通道卷积)

	reg_t Register[MAX_PE_Register_NUM];
	Register[0] = Kernel_size;
	Register[1] = Kernel_channel;
	Register[2] = Kernel_number;
	Register[3] = Kernel_bias_number;
	Register[4] = conv_stride;
	Register[5] = conv_padding;//废弃不用,在PS里面或者专门的Padding IP Core核进行Padding
	Register[6] = input_fmap_size;
	Register[7] = input_fmap_channel;
	Register[8] = output_fmap_size;
	Register[9] = output_fmap_channel;
	Register[10] = calculate_mode;

	//reg_t Reg_Inside_Kernel_number = 0;//结果的来源(卷积核序号)-<决定存放到PE输出特征图的通道>
	//reg_t shift_row = 0; //input fmap竖移坐标
	//reg_t shift_col = 0; //input fmap横移坐标

	for (int i = 0; i < Register[2]; i++)//Kernel_number
	{
		for (int j = 0; j < Register[1]; j++)//Kernel_channel
		{
			for (int k = 0; k < Register[0]; k++)//Kernel_size
			{
				for (int w = 0; w < Register[0]; w++)//Kernel_size
				{
					//多维度矩阵转一维度
					PE_Kernel_2[(i*Register[1]*Register[0]*Register[0]) + (j*Register[0]*Register[0]) + (k*Register[0]) + (w)] = PE_Kernel[i][j][k][w];
				}
			}
		}
	}
	
	for (int i = 0; i < Register[7]; i++)//input_fmap_channel
	{
		for (int j = 0; j < Register[6]; j++)//input_fmap_size
		{
			for (int k = 0; k < Register[6]; k++)//input_fmap_size
			{
				PE_Input_fmap_2[i* Register[6]* Register[6] + j * Register[6] + k] = PE_Input_fmap[i][j][k];
			}
		}
	}
	//startTime = clock();//计时开始
	PE_conv2d(Register, PE_Kernel_2, PE_Bias, PE_Input_fmap_2, PE_Output_fmap);
	//endTime = clock();//计时结束
	//printf("\n\n\n");
	for (int i = 0; i < Register[9]; i++)//output_fmap_channel
	{
		//printf("PE Output fmap channel:%d\n\n", i);
		for (int j = 0; j < Register[8]; j++)//output_fmap_size
		{
			for (int k = 0; k < Register[8]; k++)//output_fmap_size
			{
				//printf("Ofmap:%f", PE_Output_fmap[i* Register[8]*Register[8]+j* Register[8]+k]);
				//PE_Output_fmap[i * Register[8] * Register[8] + j * Register[8] + k] = PE_Output_fmap[i * Register[8] * Register[8] + j * Register[8] + k];
			}
			//printf("\n");
		}
	}
	QueryPerformanceCounter(&t2);
	printf("success\n");
	dt = (t2.QuadPart - t1.QuadPart) / (double)nFreq.QuadPart;
	//printf("the running time is :%fs\n", (double)(endTime - startTime) / (double)CLOCKS_PER_SEC); //秒
	cout << "Running time :" << dt * 1000000 << "us" << endl;
	/*1.探究缓冲机制建立与Result Store*/
	/*2.探究不同的卷积核计算模式*/
	/*3.64位并行*/
}


