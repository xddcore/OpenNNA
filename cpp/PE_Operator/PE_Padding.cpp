// PE_Padding.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<ctime> //获取程序运行时间，评估性能
#include <windows.h>//头文件
using namespace std;

typedef float data_t;//计算参数类型
typedef char reg_t;//控制寄存器类型

#define MAX_PE_Register_NUM 5 //计算控制寄存器元素个数
#define MAX_PE_Kernel_size 3 //最大输入卷积核尺寸
#define MAX_PE_Kernel_channel 1024 //最大输入卷积核通道
#define MAX_PE_Kernel_number 1024 //最大输入卷积核数量
#define MAX_PE_Bias_number 1024 //最大输入卷积核偏置
#define MAX_PE_Input_fmap_channel 1024 //最大输入特征图通道
#define MAX_PE_Input_fmap_size 320 //最大输入特征图尺寸
#define MAX_PE_Outputput_fmap_channel 1024 //最大输出特征图通道
#define MAX_PE_Output_fmap_size 320 //最大输出特征图尺寸

/*Register Define*/
//0:input_fmap_size
//1:input_fmap_channel
//2:output_fmap_size
//3:output_fmap_channel
//4:padding (fmap填充几圈0) //因为输入是要求W和H相同的,所以只能对称填充

void PE_padding(
	/*PE控制寄存器*/
	reg_t* PE_Register,
	/*输入卷积核特征图*/
	data_t* PE_Input_fmap,
	/*PE输出特征图*/
	data_t* PE_Output_fmap
)
{
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

data_t PE_Input_fmap_2[MAX_PE_Input_fmap_channel * MAX_PE_Input_fmap_size * MAX_PE_Input_fmap_size];
data_t PE_Output_fmap[MAX_PE_Outputput_fmap_channel * MAX_PE_Output_fmap_size * MAX_PE_Output_fmap_size];

int main()
{
	/*Register Define*/
	//0:input_fmap_size
	//1:input_fmap_channel
	//2:output_fmap_size
	//3:output_fmap_channel
	//4:padding (fmap填充几圈0) //因为输入是要求W和H相同的,所以只能对称填充
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	LARGE_INTEGER t2;
	double dt;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&t1);
	reg_t padding = 1; //fmap填充几圈0
	reg_t input_fmap_size = 7; //输入特征图大小
	reg_t input_fmap_channel = 2; //输入特征图通道
	reg_t output_fmap_size = (input_fmap_size + 2 * padding) ; //输出特征图数量
	reg_t output_fmap_channel = 2; //输出特征图通道

	reg_t Register[MAX_PE_Register_NUM];
	Register[0] = input_fmap_size;
	Register[1] = input_fmap_channel;
	Register[2] = output_fmap_size;
	Register[3] = output_fmap_channel;
	Register[4] = padding;

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

	PE_padding(
		/*PE控制寄存器*/
		Register,
		/*输入卷积核特征图*/
		PE_Input_fmap_2,
		/*PE输出特征图*/
		PE_Output_fmap
	);

	for (int i = 0; i < Register[3]; i++)//output_fmap_channel
	{
		//printf("PE Output fmap channel:%d\n\n", i);
		for (int j = 0; j < Register[2]; j++)//output_fmap_size
		{
			for (int k = 0; k < Register[2]; k++)//output_fmap_size
			{
				//printf("output_fmap:%f||", PE_Output_fmap[i * Register[2] * Register[2] + j * Register[2] + k]);
			}
			//printf("\n");
		}
	}
	//printf("success");
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
