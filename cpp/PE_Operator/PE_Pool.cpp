// PE_Pool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<ctime> //获取程序运行时间，评估性能
#include <windows.h>//头文件
using namespace std;

typedef float data_t;//计算参数类型
typedef char reg_t;//控制寄存器类型

#define MAX_PE_Register_NUM 7 //计算控制寄存器元素个数
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
//4:pool_kernel_size (池化核大小) //2*2或4*4
//5:kernel_stride 池化核步长
//6:pool_mode 池化方式(0:MaxPool,1:Average Pool)

void PE_pool(
	/*PE控制寄存器*/
	reg_t* PE_Register,
	/*输入卷积核特征图*/
	data_t* PE_Input_fmap,
	/*PE输出特征图*/
	data_t* PE_Output_fmap
)
{
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
	//4:pool_kernel_size (池化核大小) //2*2或4*4
	//5:kernel_stride 池化核步长
	//6:pool_mode 池化方式(0:MaxPool,1:Average Pool)
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	LARGE_INTEGER t2;
	double dt;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&t1);

	reg_t input_fmap_size = 7; //输入特征图大小
	reg_t input_fmap_channel = 2; //输入特征图通道
	reg_t pool_kernel_size = 4; //池化核大小
	reg_t kernel_stride = 1; //池化核步长
	reg_t output_fmap_size = (((input_fmap_size - pool_kernel_size) / kernel_stride) + 1); //池化输出特征图数量 //输出特征图数量
	reg_t output_fmap_channel = 2; //输出特征图通道
	reg_t pool_mode = 0; //池化方式(0:MaxPool,1:Average Pool)

	reg_t Register[MAX_PE_Register_NUM];
	Register[0] = input_fmap_size;
	Register[1] = input_fmap_channel;
	Register[2] = output_fmap_size;
	Register[3] = output_fmap_channel;
	Register[4] = pool_kernel_size;
	Register[5] = kernel_stride;
	Register[6] = pool_mode;

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

	PE_pool(
		/*PE控制寄存器*/
		Register,
		/*输入卷积核特征图*/
		PE_Input_fmap_2,
		/*PE输出特征图*/
		PE_Output_fmap
	);

	for (int i = 0; i < Register[3]; i++)//output_fmap_channel
	{
		printf("PE Output fmap channel:%d\n\n", i);
		for (int j = 0; j < Register[2]; j++)//output_fmap_size
		{
			for (int k = 0; k < Register[2]; k++)//output_fmap_size
			{
				printf("output_fmap:%f||", PE_Output_fmap[i * Register[2] * Register[2] + j * Register[2] + k]);
			}
			printf("\n");
		}
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
