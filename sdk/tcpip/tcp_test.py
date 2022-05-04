import os 
import tensorflow as tf 
from PIL import Image  #注意Image,后面会用到
import matplotlib.pyplot as plt 
import numpy as np
import socket
import sys

total_num = 0 #发送的图片数量
correct_num = 0#正确图片数量
cwd='./Fnt0_9/'
classes=[] #标签存储
label = 0
 
receive_count : int = 0
 
def start_tcp_client(ip,port):
        global total_num,correct_num,label
        ###create socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print("Begin Test FPGA Neural Network Acclectration!")
        failed_count = 0
        while True:
                try:
                    print("start connect to FPGA server ")
                    s.connect((ip,port))
                    break
                except socket.error:
                    failed_count += 1
                    print("fail to connect to FPGA server %d times" % failed_count)
                    if failed_count == 100: return
 
        # send and receive
        while True:
            print("connect success")
 
            #get the socket send buffer size and receive buffer size
            s_send_buffer_size = s.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
            s_receive_buffer_size = s.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
 
            print("client TCP send buffer size is %d" % s_send_buffer_size)
            print("client TCP receive buffer size is %d" %s_receive_buffer_size)
 
            receive_count = 0
            #遍历数据集文件夹发送测试图片  
            for root, dirs, files in os.walk(cwd):
                print(dirs) #当前路径下所有子目录
                classes = dirs; #获取标签名
                break;
            for index,name in enumerate(classes):
                class_path = cwd + name + '/'
                
                for img_name in os.listdir(class_path): 
                    img_path=class_path+img_name #每一个图片的地址
                    image_value = tf.io.read_file(img_path)
                    #解码为tensor
                    image_value_decode = tf.io.decode_jpeg(image_value,channels = 1)
                    image_value_32b = tf.image.resize(image_value_decode, (32,32))#改变像素值为32*32
                    #tensor转array
                    #print(image_value_32b)
                    #print(type(image_value_32b))
                    image_value_32b_numpy = image_value_32b.numpy()
                    #归一化[0,1] float发送 1float = 4bytes
                    image_value_32b_numpy = ((image_value_32b_numpy)/255.0).astype(np.float32)#.astype(np.uint8)
                    print(image_value_32b_numpy.shape)
                    #print(image_value_32b_numpy)
                    '''
                    for i in range(0,32):
                        for j in range(0,32):
                            img_raw = image_value_32b_numpy[i][j][0].tobytes()#将图片转化为二进制格式
                            s.send(img_raw)
                    '''

                    img_raw = image_value_32b_numpy.tobytes()#将图片转化为二进制格式
                    '''
                    print(image_value_32b_numpy[0][0][0])
                    print(image_value_32b_numpy[0][0][0].tobytes())
                    print(len(image_value_32b_numpy[0][0][0].tobytes()))
                    while True:
                        pass
                        '''
                    for i in range(1,5):#4个数据包,每个数据包1k,整个图片4kb float32 归一化 !!低地址存高字节
                        s.send(img_raw[1024*(i-1):1024*i])#发送图像矩阵
                        print("Packet %d Sended!"%(i))
                        print(1024*(i-1),1024*i)
                        print("send len is : [%d]" % len(img_raw))#(32*32*1)*4 bytes = 1024 *4 byts = 4KB
                        packet_flag = -1#结果返回标志
                        #第四个数据包是最后一个数据包,不用应答and i!=4
                        while(packet_flag==-1 and i!=4):#等待FPGA确认接收到这个1K数据包则发送下一个数据包
                            msg = s.recv(1024)#接收来自FPGA 1字节验证数据,用于表示已经成功接收到1KB数据包
                            print(msg)
                            packet_flag = 1
                            
                        
                    Result_flag = -1#结果返回标志
                    while(Result_flag==-1):
                        msg = s.recv(1024)#接收来自FPGA 1字节验证数据,用于表示已经成功接收到这张图片
                        Result_flag = 1
                        Result = int.from_bytes(msg,byteorder='little',signed=False)#bytes 转int byteorder 大端还是小端
                        print("receive data(bytes):",msg)
                        print("Result(int):",Result)
                        if(label == Result):#验证结果是否正确
                            correct_num = correct_num + 1
                      
                    print(str(total_num)+" Test array have been sent!")    
                    total_num = total_num + 1     
                label = label + 1#标签序号+1            
            s.close()
            print("total_num:%d,correct_num:%d"%(total_num,correct_num))
            print("ACC:",correct_num/total_num)#输出正确率
            break;
       
if __name__=='__main__':
        start_tcp_client('192.168.3.136',7)#
      