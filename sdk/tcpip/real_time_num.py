import cv2
import imutils
from imutils import contours
import numpy as np
import time
import socket

font = cv2.FONT_HERSHEY_DUPLEX  # 设置字体
# 模型加载
#model = tf.keras.models.load_model('./save/1660ti_tf2.1_py3.7/save_models/hdf5/1660ti_cnn_1.h5')
capture = cv2.VideoCapture(0)

#result
cnn_result = []

def get_roi(image):
    left_x,left_y,right_x,right_y = 0,0,0,0
    MAX_X,MIN_X,MAX_Y,MIN_Y = 0,0,0,0
    X_list,Y_list = [],[]

    for x in range(image.shape[0]):   # 图片的高
        for y in range(image.shape[1]):   # 图片的宽
            px = image[x,y]
            if(px == 255):
                 X_list.append(x)
                 Y_list.append(y)
    #外框数据有效
    if(X_list and Y_list):
        #print(max(X_list),min(X_list),max(Y_list),min(Y_list))
        MAX_X = max(X_list)
        MAX_Y = max(Y_list)
        MIN_X = min(X_list)
        MIN_Y = min(Y_list)
        #外框限制
        sp = image.shape
        MAX_X =  MAX_X + 10
        MAX_Y =  MAX_Y + 10
        MIN_X =  MIN_X - 10
        MIN_Y =  MIN_Y - 10
        if(MIN_X<0):
            MIN_X = 0
        if(MIN_Y<0):
            MIN_Y = 0

        if(MAX_X>sp[0]):
            MAX_X = sp[0]
        if(MAX_Y>sp[1]):
            MAX_Y = sp[1]
        return  (MAX_X,MAX_Y,MIN_X,MIN_Y,((MIN_Y,MIN_X)),((MAX_Y,MAX_X)))
    else:
        return (0,0,0,0,(0,0),(0,0))
def get_big_roi(image):
    global cnn_result
    i = 0
    small_roi_num = 0
    big_roi_result = 0
    number = 0
    number_sum = 0
        #图像大小
    sp = image.shape
    #边缘检测
    edged = cv2.Canny(image, 50, 200, 255)
    #在边缘检测map中发现轮廓
    cnts = cv2.findContours(edged.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    cnts = imutils.grab_contours(cnts)
    #所有轮廓
    digitCnts = []
    #处理后的轮廓
    digits = []
    #lunkuozuobiao
    digits_position = []
    #经过神经网络判断的轮廓
    digits_finish = []
    #从左到右对轮廓排序
    digitCnts = sorted(cnts, key=cv2.contourArea, reverse=True)
    for c in digitCnts:
        (x, y, w, h) = cv2.boundingRect(c)
        if (w >= 5 and w<=300) and (h >= 15 and h<=300):
            digits.append(c)

    #i = 0
    #筛选所有大ROI中的一个大ROI
    for c in digits:
	    # 获取ROI区域
        (x1, y1, w1, h1) = cv2.boundingRect(c)
        x1 = x1 - 10
        y1 = y1 - 10
        w1 = w1 + 20
        h1 = h1 + 20
        if(x1 < 0):
            x1 = 0
        if(y1 < 0):
            y1 = 0
        if(w1 > sp[0]):
            w1 = sp[0]
        if(h1 > sp[1]):
            h1 = sp[1]
        #roi = thresh[y:y + h, x:x + w]
        #画框
        cv2.rectangle(frame, (x1,y1),(x1+w1,y1+h1),(255, 255, 255), 1, 4)
        cv2.rectangle(Roi_image, (x1,y1),(x1+w1,y1+h1),(255, 255, 255), 1, 4)
        #思路1对应的X坐标升序(从左往右)默认升序
        #cnn_result.sort(key=lambda number: number[1])
        #思路2对应的X坐标降序(从右往左)
        cnn_result.sort(key=lambda number: number[1],reverse=True)
        #筛选出一个大ROI里面的小ROI
        for result in cnn_result:
            
            if((result[1]>x1) and (result[2]>y1) and((result[1]+result[3])<x1+w1)and((result[2]+result[4])<y1+h1)):
                #思路1:不解除完整数字，而是算X坐标偏移递增
                #cv2.putText(frame, str(result[0]), (x1 + 15 * i,y1), font, 1, (0, 255, 0), 1,)
                #i = i + 1
                #思路2:解出完整数字
                number = number + result[0]*10**i
                i = i + 1
        i = 0
        #ROI矩形添加结果
        cv2.putText(frame, str(number), (x1,y1), font, 1, (0, 255, 0), 1,)
        #所有识别的数字求和
        number_sum = number_sum + number
        number = 0
    cnn_result.clear()
    #求和结果
    cv2.putText(frame, "SUM:"+str(number_sum), (20,45), font, 1, (0, 255, 0), 1,)
    number_sum = 0
        #small_roi_num = i
        #cv2.putText(frame, str(big_roi_result), (x1,y1), font, 1, (0, 255, 0), 1,)
        #i = 0
    #big_roi_result = 0
        #ROI n
        #cv2.rectangle(Roi_image, (x1,y1),(x1+w1,y1+h1),(255, 255, 255), 1, 4)

def get_small_roi(image):
    #图像大小
    sp = image.shape
    #边缘检测
    edged = cv2.Canny(image, 50, 200, 255)
    #在边缘检测map中发现轮廓
    cnts = cv2.findContours(edged.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    cnts = imutils.grab_contours(cnts)
    #所有轮廓
    digitCnts = []
    #处理后的轮廓
    digits = []
    #lunkuozuobiao
    digits_position = []
    #经过神经网络判断的轮廓
    digits_finish = []
    #从左到右对轮廓排序
    digitCnts = sorted(cnts, key=cv2.contourArea, reverse=True)
    for c in digitCnts:
        (x, y, w, h) = cv2.boundingRect(c)
        if (w >= 5 and w<=200) and (h >= 15 and h<=200):
            digits.append(c)

    #i = 0
    for c in digits:
	    # 获取ROI区域
        (x1, y1, w1, h1) = cv2.boundingRect(c)
        x1 = x1 - 10
        y1 = y1 - 10
        w1 = w1 + 20
        h1 = h1 + 20
        if(x1 < 0):
            x1 = 0
        if(y1 < 0):
            y1 = 0
        if(w1 > sp[0]):
            w1 = sp[0]
        if(h1 > sp[1]):
            h1 = sp[1]
        #roi = thresh[y:y + h, x:x + w]
        #画框
        #cv2.rectangle(frame, (x1,y1),(x1+w1,y1+h1),(0, 255, 0), 1, 4)
        #ROI n
        cv2.rectangle(Roi_image, (x1,y1),(x1+w1,y1+h1),(0, 255, 0), 2, 6)
        #cv2.rectangle(frame, (x2,y2),(x2+w2,y2+h2),(0, 255, 0), 1, 4)
        #roi =  image[(y-10):(y+h+10),(x-10):(x+w+10)]
        roi =  cv2.resize(image[y1:y1+h1,x1:x1+w1],(32,32))
        
        cnn_model(roi,x1,y1,w1,h1)
    return Roi_image
        #如果镜头固定 可尝试以下抗晃动算法，提升帧率
        #
        #digits_position = (y,y+h,x,x+w)
        #if((y+30>y>y-20) or (x+30>x>x-30) or (y+h+30>y>x+h-30) or (x+w+30>x>x+w-30)):
        #if digits_position  not in digits_finish:
            #cnn_model(roi,(x,y))
            #for i in range(-20,20):#晃动屏蔽
                #
                #digits_finish.append((y+i,y+h+i,x+i,x+w+i))
                #
                #digits_finish.append((y-i,y+h-i,x+i,x+w+i))
                #
                #digits_finish.append((y+i,y+h+i,x-i,x+w-i))
                #
                #digits_finish.append((y,y+h,x-i,x+w-i))
                #
                #digits_finish.append((y+i,y+h+i,x,x+w))
                #
                #digits_finish.append((y,y+h,x+i,x+w+i))
                #
                #digits_finish.append((y-i,y+h-i,x,x+w))
                #print(str(i))
        #else:
            #pass

def cnn_model(image_data,x1,y1,w1,h1):
    global cnn_result,s
    image_value_32b_numpy = ((image_data)/255.0).astype(np.float32)#.astype(np.uint8)
    img_raw = image_value_32b_numpy.tobytes()#将图片转化为二进制格式
    for i in range(1,5):#4个数据包,每个数据包1k,整个图片4kb float32 归一化 !!低地址存高字节
        s.send(img_raw[1024*(i-1):1024*i])#发送图像矩阵
        packet_flag = -1#结果返回标志
        #第四个数据包是最后一个数据包,不用应答and i!=4
        while(packet_flag==-1 and i!=4):#等待FPGA确认接收到这个1K数据包则发送下一个数据包
            msg = s.recv(1024)#接收来自FPGA 1字节验证数据,用于表示已经成功接收到1KB数据包
            packet_flag = 1                   
    Result_flag = -1#结果返回标志
    while(Result_flag==-1):
        msg = s.recv(1024)#接收来自FPGA 1字节验证数据,用于表示已经成功接收到这张图片
        Result_flag = 1
        Result = int.from_bytes(msg,byteorder='little',signed=False)#bytes 转int byteorder 大端还是小端    
    #添加结果
    cnn_result.append([Result,x1,y1,w1,h1])

def isOverlap(Rect1,Rect2): #x,y,w,h,Rect1左边 Rect2右边
    x,y,w,h = 0,0,0,0

    if (Rect1[0] + Rect1[2]  > Rect2[0] and
        Rect2[0] + Rect2[2]  > Rect1[0] and
        Rect1[1] + Rect1[3] > Rect2[1] and
        Rect2[1] + Rect2[3] >Rect1[1]
       ):
        
        if(Rect1[0] < Rect2[0]):
            x = Rect1[0]
        else:
            x = Rect2[0]
        if(Rect1[1] < Rect2[1]):
            y = Rect1[1]
        else:
            y = Rect2[1]
        if(Rect1[3]>Rect2[3]):
            h = Rect1[3]
        else:
            h = Rect2[3]
        #x = Rect1[0] 
        #y = Rect1[1]
        mix_len = ((Rect1[0] + Rect1[2]) - Rect2[0])
        w = (Rect1[0] + Rect1[2] - mix_len) + Rect2[2]
        return x,y,w,h #x,y,w,h 重叠取最大矩形
    else:
        return 0,0,0,0;
        
        
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("Begin Test FPGA Neural Network Acclectration!")
failed_count = 0
while True:
    try:
        print("start connect to FPGA server ")
        s.connect(('192.168.3.136',7))
        break
    except socket.error:
        failed_count += 1
        print("fail to connect to FPGA server %d times" % failed_count)
        #if failed_count == 100: return
                    
while(True):
    start_time = time.time() # start time of the loop
    #ROI
    Roi_image = np.zeros([480, 720, 3], np.uint8)
    # 获取一帧
    ret, frame = capture.read()
    #print(frame.shape)
     # 水平反转
    #frame = cv2.flip(frame, 0) 
    # 将这帧转换为灰度图
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    #ret, binary = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_TRIANGLE)
    binary =  cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,cv2.THRESH_BINARY, 25, 10)
    #cv2.bitwise_not(binary,binary_nagetive)
    binary_nagetive_0 = binary#255 - binary
    binary_nagetive_1 = cv2.medianBlur(binary_nagetive_0,5)

    #x_max,y_max,x_min,y_min,x_point,y_point = get_roi(binary_nagetive_1)

    #if((x_min != x_max)and(y_min != y_max)):
    if 1:
        #binary_nagetive_roi = binary_nagetive_1[x_min:x_max,y_min:y_max]
        #连通域计算
        #num_labels, labels, stats, centers = cv2.connectedComponentsWithStats(binary, connectivity=8, ltype=cv2.CV_32S)
        #print(stats)
        
        #for t in range(1,num_labels,1):
            #x,y,w,h,area = stats[t]
            #cv2.rectangle(frame, (x,y),(x+w,y+h),(255, 0, 0), 1, 4)
        #ROI获取
        get_big_roi(get_small_roi(binary_nagetive_1))
        FPS = (1.0 // (time.time() - start_time))
        cv2.putText(frame, "FPS:"+str(FPS), (20,20), font, 1, (0, 255, 0), 1,)
        cv2.imshow('frame1', frame)
        cv2.imshow('frame2', binary_nagetive_1)
        cv2.imshow('frame3', Roi_image)
        if cv2.waitKey(1) == ord('q'):
            cv2.destroyAllWindows()
            break
    else:
        print("正在提取数字...")

