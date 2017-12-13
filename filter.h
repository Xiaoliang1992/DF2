#include <stdlib.h>
#include <stdio.h>



//======# accurate_waiting #=======//
//# 精确延时函数，可指定起始计时点
float accurate_waiting(float Ts, struct timeval tstartc_1)
{
	long int dt_1;
	struct timeval tstopc_1;
	usleep((int)(Ts*1000*0.95));//先等待足够长的时间，另外1%的时间用于精确控制
	while(1)
	{
		usleep(Ts*1000*0.00001);//精确度为0.01%
		gettimeofday(&tstopc_1,NULL);//采集时间点
		dt_1=(tstopc_1.tv_sec-tstartc_1.tv_sec)*1000000+tstopc_1.tv_usec-tstartc_1.tv_usec;//计算时间间隔
		if(dt_1-Ts*1000>0) {break;}//超过采样周期便跳出程序
	}
    return ((float)dt_1)/1000000;
}

//---first order filter to reduce noise
float First_order_filter(float Filter_para, float data_f, float data,  float dt)
{
    float d_data, a;
    a = 1 / Filter_para;
    d_data = -a * data_f + a * data;
    data_f = data_f + d_data*dt;
    return data_f;
}

typedef struct 
{
	int data_is_ok;
	float snr;
	float d_mean_0;
	float d_mean_1;
	float s_disconf;
	float datas[60];
	int num;
	int i;
} data_instructor;


void data_instructor_init(data_instructor *); 
int data_instructor_insert(float, data_instructor *);
float data_instructor_cal(float, data_instructor *);

void data_instructor_init(data_instructor * data_ins)
{
	data_ins->data_is_ok = 0;
	data_ins->snr = 0;
	data_ins->s_disconf = 1;
	data_ins->d_mean_0 = 0;
	data_ins->d_mean_1 = 0;
	int i = 0;
	data_ins->num = 50;
}

int data_instructor_insert(float data, data_instructor * data_ins)
{
	
	(data_ins->datas)[data_ins->i] = data;
	if (data_ins->i >= data_ins->num -1)
	{
		data_ins->i = 0;
		return 1;
	}
	data_ins->i += 1;
	return -1;
}

float data_instructor_cal(float data, data_instructor * data_ins)
{
	int insert_is_ok = 0;
	int i = 0;
	float sum = 0;
	insert_is_ok = data_instructor_insert(data, data_ins);
	if (insert_is_ok) 
	{
		data_ins->d_mean_1 = data_ins->d_mean_0; 
		for(i = 0; i <= data_ins->num-1; i++) sum += (data_ins->datas)[i];
		data_ins->d_mean_0 = sum/data_ins->num;
		data_ins->s_disconf = fabs(data_ins->d_mean_0 - data_ins->d_mean_1 + 0.0001)/fabs(data_ins->d_mean_0 + 0.0001);
	}
	return data_ins->s_disconf;
}
