#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix 标准函数定义*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX 终端控制定义*/
#include     <errno.h>      /*错误号定义*/


#define FALSE -1
#define TRUE 0


/***************************************************************************************/
/*
*@brief  打开串口
*@param  char *Dev 串口文件所在路径
*@param  speed  类型 int  串口速度
*@return  void
*/
int OpenDev(char *Dev)
{
	int	fd = open( Dev, O_RDWR|O_NONBLOCK );         //| O_NOCTTY | O_NDELAY O_RDWR只读或神马	
	if (-1 == fd)	
	{ 			
		perror("Can't Open Serial Port");
		return -1;		
	}	
	else	
		return fd;
}



/***************************************************************************************/
/**
*@brief  设置串口通信速率
*@param  fd     类型 int  打开串口的文件句柄
*@param  speed  类型 int  串口速度
*@return  void
*/
int speed_arr[] = { B115200,B38400, B19200, B9600, B4800, B2400, B1200, B300,//B他妈的是什么意思
          };
int name_arr[] = {115200,38400,  19200,  9600,  4800,  2400,  1200,  300};

void set_speed(int fd, int speed)
{
  int   i; 
  int   status; 
  struct termios   Opt;
  tcgetattr(fd, &Opt); //tcgetattr函数用于获取与终端相关的参数。参数fd为终端的文件描述符，返回的结果保存在termios 结构体中，该结构体一般包
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) { 
    if  (speed == name_arr[i]) {     
      tcflush(fd, TCIOFLUSH);     
      cfsetispeed(&Opt, speed_arr[i]);  
      cfsetospeed(&Opt, speed_arr[i]);   
      status = tcsetattr(fd, TCSANOW, &Opt);  
      if  (status != 0) {        
        perror("tcsetattr fd1");  
        return;     
      }    
      tcflush(fd,TCIOFLUSH);   
    }  
  }
}





/***************************************************************************************/
/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄
*@param  databits 类型  int 数据位   取值 为 7 或者8
*@param  stopbits 类型  int 停止位   取值为 1 或者2
*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
int set_Parity_MT(int fd,int databits,int stopbits,int parity)
{ 
	struct termios options; 
	if  ( tcgetattr( fd,&options)  !=  0) { 
		perror("SetupSerial 1");     
		return(FALSE);  
	}
	options.c_cflag &= ~CSIZE; 
	switch (databits) /*设置数据位数*/
	{   
	case 7:		
		options.c_cflag |= CS7; 
		break;
	case 8:     
		options.c_cflag |= CS8;
		break;   
	default:    
		fprintf(stderr,"Unsupported data size\n"); return (FALSE);  
	}
	switch (parity) 
	{   
		case 'n':
		case 'N':    
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */ 
			break;  
		case 'o':   
		case 'O':     
			options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/  
			options.c_iflag |= INPCK;             /* Disnable parity checking */ 
			break;  
		case 'e':  
		case 'E':   
			options.c_cflag |= PARENB;     /* Enable parity */    
			options.c_cflag &= ~PARODD;   /* 转换为偶效验*/     
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S': 
		case 's':  /*as no parity*/   
		    	options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			options.c_iflag |= INPCK;
			break;  
		default:   
			fprintf(stderr,"Unsupported parity\n");    
			return (FALSE);  
		}  
	/* 设置停止位*/  
	switch (stopbits)
	{   
		case 1:    
			options.c_cflag &= ~CSTOPB;  
			break;  
		case 2:    
			options.c_cflag |= CSTOPB;  
		        break;
		default:    
			 fprintf(stderr,"Unsupported stop bits\n");  
			 return (FALSE); 
	} 

	/*
	options.c_iflag=IGNPAR; //忽略帧错误和奇偶校验错误
        options.c_oflag=0; //Raw模式输出
        options.c_lflag=0; //不处理，原始数据输入
        options.c_cc[VMIN]=0; //无最小读取字符
        options.c_cc[VTIME]=0;//无等待时间
	*/

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//0x03传送过来，原始模式
	options.c_iflag &= ~(ICRNL | INLCR| IGNCR);////清bit位 关闭字符映射 0x0a 0x0d,使得0x0a和0x0d可以传送过来
	options.c_iflag &= ~(IXON | IXOFF | IXANY); ////清bit位 关闭流控字符 0x11 0x13，道理同上
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] =0;//150; // 设置超时15 seconds
	options.c_cc[VMIN] = 0; //Update the options and do it NOW
	fcntl(fd,F_SETFL,FNDELAY);
	if (tcsetattr(fd,TCSANOW,&options) != 0)   
	{ 
		perror("SetupSerial 3");   
		return (FALSE);  
	} 
	return (TRUE);  
}

/*===========@ recieve_data @==============*/
//串口接收函数
int recieve_data(int fd, unsigned char *buf,int count)//unsigned char
{
	int status=0;
	status=read(fd, buf, count);	
	//printf("recieve status is %d\n",status);
	tcflush(fd,TCIFLUSH);//接收端缓存清除命令
	return status;
}

/*===========@ recieve_data @==============*/
//串口发送函数
int send_data(int fd,unsigned char *buf,int count)
{
	int status=0;
	status=write(fd,buf,count);
	//tcflush(fd,TCOFLUSH);//发送端缓存清除命令
	return status;
}

/*===========@ recieve_data @==============*/
//串口设置函数
int uart_operate(char *dev_1,int baudrate)
{	
	int fd_1;
	fd_1 = OpenDev(dev_1);//这里返回的是句柄函数
	if(fd_1==FALSE) return 0;
	set_speed(fd_1,baudrate);//设置波特率
	if (set_Parity_MT(fd_1,8,1,'N') == FALSE)  //set_Parity_MT（）里是指定到了指定的字节数才返回读取的数据//草都没用
	{
		printf("Set Parity Error\n");
		exit (0);
	}
	return fd_1;
}

/*===========@ clearbuf @==============*/
//缓存清空函数
void clearbuf(int fd,char *buf, int N)
{
	int i=0;
	for(i=0;i<=N;i++)
	{
		*(buf+i)=0;
	}
}
