// PE_Dense.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<ctime> //获取程序运行时间，评估性能
#include <windows.h>//头文件
using namespace std;

typedef float data_t;//计算参数类型
typedef char reg_t;//控制寄存器类型

#define MAX_PE_Register_NUM 6 //计算控制寄存器元素个数
#define MAX_PE_Kernel_size 3 //最大输入卷积核尺寸
#define MAX_PE_Kernel_channel 1024 //最大输入卷积核通道
#define MAX_PE_Kernel_number 1024 //最大输入卷积核数量
#define MAX_PE_Bias_number 1024 //最大输入卷积核偏置
#define MAX_PE_Input_fmap_channel 1024 //最大输入特征图通道
#define MAX_PE_Input_fmap_size 320 //最大输入特征图尺寸
#define MAX_PE_Outputput_fmap_channel 1024 //最大输出特征图通道
#define MAX_PE_Output_fmap_size 320 //最大输出特征图尺寸

/*推荐的全连接层计算参数量<=1000*10*10*1=10 0000*/\
/*以下参数与PE核计算无直接关系,可以修改,主要目的 约束数组大小,防止爆栈/爆堆*/
#define MAX_PE_Dense_Neure_Number 10 //最大全连接神经元个数
#define MAX_PE_Dense_Neure_Bias 10 //最大全连接神经元偏置(一个Neure->一个偏置)
#define MAX_PE_Dense_Neure_Input_size 100 
#define MAX_PE_Dense_Neure_Output_size 10 //最多只有1000个神经元,最多输出1000个参数(也就是如果用dense套softmax做分类任务的化,最多支持1000类)
#define MAX_PE_Dense_Neure_Input_channel 2//最大全连接输入通道数(也就是全连接层的参数量小于320*320*3=307200(fix point16的话,参数量小于600KB))

/*Register Define*/
//0:input_fmap_size
//1:input_fmap_channel
//2:output_fmap_size
//3:output_fmap_channel
//4:neure_number (神经元个数) //最大1024个
//5:neure_bias_bumber 神经元偏置个数

/*
PE 全连接层参数量通过 neure_number*(input_fmap_size*input_fmap_size*input_fmap_channel)得出
由于xddcore使用基于地址的DDR访问策略(PE加速只与PE控制寄存器设置有关,不与MAX_PE_xxx参数设置有关,
MAX_PE_xxx参数仅用于约束DDR3内存malloc,防止爆堆/爆栈)
!!!
!!!但是对于卷积加速PE来说,由于卷积加速采用流水线缓冲机制,为了提高程序易读性,内部的参数计算受到MAX_PE_xxx参数设置约束.!!!
!!!
所以,在全连接层访问中,只约束传入参数数量,不约束全连接层神经元数量和每个神经元计算的参数多少
请根据PC或ZYNQ PS端实际DDR内存大小加以约束,防止爆堆
*/
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
	int input_fmap_size; //输入一维特征图大小
	int weights_bias_number;//权重和偏置计数(一个neura->1 weights + 1 bias)
	for (weights_bias_number = 0; weights_bias_number < PE_Register[4]; weights_bias_number++)//neure_number
	{
		/*+bias*/
		/*非常重要:清空计算中间量 下面0,0,0位置等于权重,对于多次计算来说,清空了上一次的计算结果,避免影响本次计算*/
		PE_Output_fmap[weights_bias_number] = PE_Neure_bias[weights_bias_number];
		/**weights*/
		//对于全连接网络来说,它是一维的,所以input_fmap_size=input_fmap_size*input_fmap_size*input_fmap_channel
		//在这一步顺便将多唯索引压成一维来访问
		for (input_fmap_size = 0; input_fmap_size < (PE_Register[0]*PE_Register[0]* PE_Register[1]); input_fmap_size++)//对于全连接网络来说,它是一维的,所以input_fmap_size=input_fmap_size*input_fmap_size*input_fmap_channel
		{
			/*+bias*/
			/*非常重要:清空计算中间量 下面0,0,0位置等于权重,对于多次计算来说,清空了上一次的计算结果,避免影响本次计算*/
			//if (input_fmap_size == 0)
				//PE_Output_fmap[weights_bias_number] = PE_Neure_bias[weights_bias_number];
			PE_Output_fmap[weights_bias_number] += (PE_Input_fmap[input_fmap_size] * PE_Neure_weights[(weights_bias_number*(PE_Register[0] * PE_Register[0] * PE_Register[1]))+input_fmap_size]); //+ neure_bias[weights_bias_number];

		}
	}
}

data_t PE_Input_fmap[MAX_PE_Dense_Neure_Input_channel][MAX_PE_Dense_Neure_Input_size][MAX_PE_Dense_Neure_Input_size] = {
		{
			{1,1,1},
			{1,1,1},
			{1,1,1},
		},
		{
			{2,2,2},
			{2,2,2},
			{2,2,2},
		},
};
data_t PE_Neure_weights[MAX_PE_Dense_Neure_Number][MAX_PE_Dense_Neure_Input_size* MAX_PE_Dense_Neure_Input_size*MAX_PE_Dense_Neure_Input_channel] = {//MAX_PE_Input_fmap_size*MAX_PE_Input_fmap_size*MAX_PE_Dense_Neure_Input_channel
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},//每个神经元总共处理18个数据
	{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}//每个神经元总共处理18个数据
};
//一维全连接参数数量=全连接神经元数量*每个神经元偏置数量
data_t PE_Neure_weights_2[MAX_PE_Dense_Neure_Number*(MAX_PE_Dense_Neure_Input_size * MAX_PE_Dense_Neure_Input_size * MAX_PE_Dense_Neure_Input_channel)];

data_t PE_Neure_bias[MAX_PE_Dense_Neure_Bias] = {
	1,2,
};

data_t PE_Input_fmap_2[(MAX_PE_Dense_Neure_Input_size * MAX_PE_Dense_Neure_Input_size * MAX_PE_Dense_Neure_Input_channel)];
data_t PE_Output_fmap[MAX_PE_Dense_Neure_Output_size];

int main()
{
	/*Register Define*/
	//0:input_fmap_size
	//1:input_fmap_channel
	//2:output_fmap_size
	//3:output_fmap_channel
	//4:neure_number (神经元个数) //最大1024个
	//5:neure_bias_bumber 神经元偏置个数
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	LARGE_INTEGER t2;
	double dt;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&t1);

	reg_t neure_number = 2; //(神经元个数) //最大1024个
	reg_t neure_bias_bumber = 2; //神经元偏置个数
	reg_t input_fmap_size = 3; //输入特征图大小
	reg_t input_fmap_channel = 2; //输入特征图通道
	reg_t output_fmap_size = neure_number; //全连接层输出数量等于神经元个数(一个神经元输出一个参数)
	reg_t output_fmap_channel = 1; //全连接神经元输出数据永远为一通道 1维数据,保留这个参数是为了和其他PE统一

	reg_t Register[MAX_PE_Register_NUM];
	Register[0] = input_fmap_size;
	Register[1] = input_fmap_channel;
	Register[2] = output_fmap_size;
	Register[3] = output_fmap_channel;
	Register[4] = neure_number;
	Register[5] = neure_bias_bumber;

	for (int i = 0; i < Register[1]; i++)//input_fmap_channel
	{
		for (int j = 0; j < Register[0]; j++)//input_fmap_size
		{
			for (int k = 0; k < Register[0]; k++)//input_fmap_size
			{
				PE_Input_fmap_2[i * Register[0] * Register[0] + j * Register[0] + k] = PE_Input_fmap[i][j][k];
			}
		}
	}

	for (int i = 0; i < Register[4]; i++)//neure_number
	{
		for (int j = 0; j < (Register[0]*Register[0]*Register[1]); j++)//input_fmap_size=input_fmap_size*input_fmap_size*input_fmap_channel
		{
			PE_Neure_weights_2[(i*Register[0]*Register[0]*Register[1])+j] = PE_Neure_weights[i][j];
		}
	}

	PE_dense(
		/*PE控制寄存器*/
		Register,
		/*PE全连接神经元权重*/
		PE_Neure_weights_2,
		/*PE全连接神经元偏置*/
		PE_Neure_bias,
		/*输入全连接PE的特征图*/
		PE_Input_fmap_2,
		/*全连接PE输出特征图*/
		PE_Output_fmap
	);

	for (int i = 0; i < Register[4]; i++)//output_fmap_channel
	{
		printf("output_fmap:%f||", PE_Output_fmap[i]);
	}
	QueryPerformanceCounter(&t2);
	printf("success\n");
	dt = (t2.QuadPart - t1.QuadPart) / (double)nFreq.QuadPart;
	cout << "Running time :" << dt * 1000000 << "us" << endl;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
