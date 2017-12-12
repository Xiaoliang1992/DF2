#include <stdio.h>/*标准输入输出定义*/
#include <string.h>
#include <ctype.h>
#include <errno.h>/*错误号定义*/
#include <string.h>
#include <unistd.h>/*Unix 标准函数定义*/
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>   
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/   
#include <math.h>

//=============载入自定义头文件================//
#include"uart.h"//设置串口
#include"Data_read.h"
#include"filter.h"

//==========================Parameters==========================//
float Ts = 100;// Control time in ms
float Fr12_range = 20.0, Mr12_range = 3.0, Fd12_range = 5.0; 
float Filter_para = 0.1;
float data_disconf_min = 0.05;
//=================================================================//	
struct timeval tstartc, tsendc;
float dt;
//缓存变量声明
unsigned char  Dataint_buf[50];
float Dataint[10];
float nr[2];
float Fr1, Fr2, Fd1, Fd2, Mr1, Mr2, thr1, thr2, Fr1_f, Fr2_f, Fd1_f, Fd2_f, Mr1_f, Mr2_f, thr1_f, thr2_f, Fr12, Fd12, Fall, P1, P2, Pall;
float d_Fr1 = 0, d_Fr2 = 0, d_Fd1 = 0, d_Fd2 = 0, d_Mr1 = 0, d_Mr2 = 0, nr1 = 0, nr2 = 0;
int k;//用于缓存区清零
int i;
int sum_clear = 0;
int n_save_points = 0;
int flag = 0, flag1 = 0;
float sys_time = 0.0;
float data_disconf = 1;


pthread_mutex_t Device_mutex ;
int sockfd,new_fd;
pthread_t thread1;//定义线程
char c_command;

data_instructor data_ins;

int fd_0;
int fd_data;
int fd_point;

char *dev_0 = "/dev/ttyUSB0";//Xbee to receive data from arduino mega 2560

FILE *data_save; // for all the data save
FILE *point_save; // for one point save
char data_save_str_0[30] = "./save/datas/data_save_";
char data_save_str[30];
char data_save_num[5] = "000"; 
char point_save_str_0[30] = "./save/points/point_save_";
char point_save_str[30];
char point_save_num[5] = "000"; 
char btxt[10] = ".txt";
char notes[200];
int file_i = 100;

void sys_setup();
void data_aquision();
void data_save_print();
void zero_calibration();
void save_poit();
void save_data();
void clear_buf();
void printf_data();
void control_command();
void data_save_init();
void point_save_init();

void thread_func1()//线程1：用于采集SBG陀螺仪数据
{
    while(1)
    {
        c_command = getchar();
        usleep(100000);
    }
	
}//线程1结束


int main(int argc, char *argv[])
{    

    sys_setup();
    if(pthread_create(&thread1,NULL,(void*)thread_func1,NULL) == -1)//创建线程
	{
		printf("create IP81 Thread error !\n");
		exit(1);
	}
    



    //zero_calibration();

    while(1)
    {
		gettimeofday(&tstartc,NULL);//采集时间点
		
        data_aquision();

        control_command();
        
		save_data();

        printf_data();

        clear_buf();

		dt = accurate_waiting(Ts, tstartc);
		sys_time = sys_time + dt;
    }


}//主函数结束



void sys_setup(void)
{
    data_save_init();
	point_save_init();
	pthread_mutex_init(&Device_mutex,NULL);//初始化互斥锁
	fd_0 = uart_operate(dev_0,115200);
    data_instructor_init(&data_ins);
	//usleep(50000);
}


void data_aquision(void)
{
    flag1 = Arduino_read(fd_0, Dataint_buf, Dataint);
	if(flag1)// The data is OK?
	{
		Fr1 = fabs(FMint2float(Dataint[0], Fr12_range) - d_Fr1);
		Fr2 = fabs(FMint2float(Dataint[1], Fr12_range) - d_Fr2);
		Fd1 = fabs(FMint2float(Dataint[2], Fd12_range) - d_Fd1);
		Fd2 = fabs(FMint2float(Dataint[3], Fd12_range) - d_Fd2);
		Mr1 = fabs(FMint2float(Dataint[4], Mr12_range) - d_Mr1);
		Mr2 = fabs(FMint2float(Dataint[5], Mr12_range) - d_Mr2);
		thr1 = (float)(Dataint[6])/255 + 0.0;
		thr2 = (float)(Dataint[8])/255 + 0.0;
		nr1 = 60 / (Dataint[7]/1000000*14 + 0.0001);
		nr2 = 60 / (Dataint[9]/1000000*14 + 0.0001);
		if (nr1 > 50000) nr1 = 0;
		if (nr2 > 50000) nr2 = 0;

		// To filt the data
		if(flag) 
		{
		    Fr1_f = First_order_filter(Filter_para, Fr1_f, Fr1, dt);
		    Fr2_f = First_order_filter(Filter_para, Fr2_f, Fr2, dt);
		    Fd1_f = First_order_filter(Filter_para, Fd1_f, Fd1, dt);
		    Fd2_f = First_order_filter(Filter_para, Fd2_f, Fd2, dt);
		    Mr1_f = First_order_filter(Filter_para, Mr1_f, Mr1, dt);
		    Mr2_f = First_order_filter(Filter_para, Mr2_f, Mr2, dt);
		}
		else
		{
		    Fr1_f = Fr1;
		    Fr2_f = Fr2;
		    Fd1_f = Fd1;
		    Fd2_f = Fd2;
		    Mr1_f = Mr1;
		    Mr2_f = Mr2;
		    flag = 1;
		}
        Fr12 = Fr1_f + Fr2_f;
        Fd12 = Fd1_f + Fd2_f;
        Fall = Fr12 + Fd12;
        P1 = Fr1*nr1/9549*1000;
        P2 = Fr2*nr2/9549*1000;
        Pall = P1 + P2;
	}

    data_disconf = data_instructor_cal(Fr12, &data_ins);
}

void clear_buf(void)
{
    // ====================== data_save_print() ==========================================//
    
	for(k=0;k<50;k++) //缓存区清零
    Dataint_buf[k]= 0;		
    
}

void zero_calibration(void)
{
	printf("auto triming is started ......\n");
	sum_clear = 0;
	for (i = 0;i < 20;i++)
	{
	    flag1 = Arduino_read(fd_0, Dataint_buf, Dataint);
	    d_Fr1 = FMint2float(Dataint[0], Fr12_range);
        d_Fr2 = FMint2float(Dataint[1], Fr12_range);
        d_Fd1 = FMint2float(Dataint[2], Fd12_range);
        d_Fd2 = FMint2float(Dataint[3], Fd12_range);
        d_Mr1 = FMint2float(Dataint[4], Mr12_range);
        d_Mr2 = FMint2float(Dataint[5], Mr12_range);
        sum_clear += flag1;
        if (sum_clear > 5) break;
		usleep(85000);
	}
}

void save_poit(void)
{
	fprintf(point_save, "%.3f %.0f %.0f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", sys_time, nr1, nr2, Fr1_f, Fr2_f, Fd1_f, Fd2_f, Mr1_f, Mr2_f, data_disconf);
    n_save_points += 1;
}

void save_data(void)
{
	fprintf(data_save, "%.3f %.0f %.0f %.2f %.2f %.2f %.2f %.2f %.2f\n", sys_time, nr1, nr2, Fr1_f, Fr2_f, Fd1_f, Fd2_f, Mr1_f, Mr2_f);
}

void printf_data(void)
{
    printf("\ec");
    printf("----------------------- Ducted Fan testing program -------------------------\n");
    printf(" Time: %3.1fs, dt = %.3fs\n",sys_time, dt);
    printf(" Speed(RPM): nr1 = %4.0f, nr2 = %4.0f\n\n", nr1, nr2);
    printf(" Throttle(%%): thr1 = %2.0f%%, thr2 = %2.0f%%\n\n",thr1*100, thr2*100);
    printf(" Force(kg): Fr1 = %.2f, Fr2 = %.2f, Fd1 = %.2f, Fd2 = %.2f\n", Fr1_f, Fr2_f, Fd1_f, Fd2_f );
    printf("            Frotor(%.2f) +  Fduct(%.2f) = Fall(%.2f) with Fduct/Fall = %.2f\n\n", Fr12, Fd12, Fall, Fd12/(Fr12+0.01));
    printf(" Moment(Nm): Mr1 = %.2f, Mr2 = %.2f\n\n", Mr1_f, Mr2_f);
    printf(" Power(W): P1(%3.0f) + P2(%3.0f) = Pall(%3.0f)\n\n",P1, P2, Pall);
    printf(" Number of saved single pionts = %d\n",n_save_points);
    printf(" Data confidence = %.2f\n",1 - data_disconf);
    if (data_disconf < data_disconf_min) printf(" Single point save status: Yes\n");
    else printf(" Recommended single point save status: No\n");
    printf("-----------------------------------------------------------------------------\n");
    printf(" How to use:\n");
    printf(" q + enter: Quit this program and save all data\n");
    printf(" s + enter: Save single point data\n");
    printf(" c + enter: Reset data\n");
    printf("-----------------------------------------------------------------------------\n");
    //printf("time: %3.2f, nr1 = %4.0f, nr2 = %4.0f, Fr1 = %.2f, Fr2 = %.2f, Fd1 = %.2f, Fd2 = %.2f, Mr1 = %.2f, Mr2 = %.2f\n",sys_time, nr1,nr2, Fr1_f, Fr2_f, Fd1_f, Fd2_f, Mr1_f, Mr2_f);
    //printf("asdadsadsadsadsadas\n");
}

void control_command(void)
{
    if (c_command == 'q') 
    {
        c_command = '0';
        printf("\ec");
        printf("Test data has been restored, please take notes and press enter!\n");
        printf("Notes: ");
        fprintf(point_save,"-------------------------- Notes --------------------------------\n");
        fprintf(point_save,"sys_time, nr1, nr2, Fr1_f, Fr2_f, Fd1_f, Fd2_f, Mr1_f, Mr2_f, data_disconf\n");
        for (i = 0;i <= 200;i++)
        {
            notes[i] = getchar();
            if (i>0)
            {
                if (notes[i] == 10)
                {
                    fprintf(point_save,"%s\n",notes);
                    break;
                }
            }
            
        }
        exit(0);
    }
    
    if (c_command == 's') 
    {
        c_command = '0';
        save_poit();
    }

    if (c_command == 'c')
    {
        c_command = '0';
        //zero_calibration();
    }
}

void data_save_init(void)
{
     for(i = 0;i<99;i++)
    {
        // To build data_save_100 ... 101
        for(k=0;k<30;k++) data_save_str[k]= 0;		
        sprintf(data_save_num, "%d",file_i);
        strcat(data_save_str,data_save_str_0);
        strcat(data_save_str,data_save_num);
        strcat(data_save_str, btxt);
        
        fd_data = open(data_save_str, O_NONBLOCK);
        if(fd_data < 0) 
        {
            data_save = fopen(data_save_str, "w");
            file_i = 100;
            break;
        }
        file_i += 1;
    }
}

void point_save_init(void)
{
     for(i = 0;i<99;i++)
    {
        // To build data_save_100 ... 101
        for(k=0;k<30;k++) point_save_str[k]= 0;		
        sprintf(point_save_num, "%d",file_i);
        strcat(point_save_str,point_save_str_0);
        strcat(point_save_str,point_save_num);
        strcat(point_save_str, btxt);
        
        fd_point = open(point_save_str, O_NONBLOCK);
        if(fd_point < 0) 
        {
            point_save = fopen(point_save_str, "w");
            file_i = 100;
            break;
        }
        file_i += 1;
    }
}