#include <stdlib.h>
#include <stdio.h>



int Arduino_read(int fd, unsigned char *Dataint_inbuff, float *Dataint)
{
    recieve_data(fd, Dataint_inbuff, 40);
    int i = 0;
    int j =0;
    int k = 0;
    int flag = 0;
    unsigned char sum = 0;
	
    for (k = 0;k<=20;k++)
    {
        if (Dataint_inbuff[k] == 0xDC)
        {
	    for(j=k;j<=18+k;j++)
                {
                    sum = sum + Dataint_inbuff[j];
                    //printf("%x ",Dataint_inbuff[j]);
                }
                //printf("\n");
	//printf("sum = %d, Dataint_inbuff[k+19] = %d\n",sum, Dataint_inbuff[k+19]);
		if (sum == Dataint_inbuff[k+19])
		{
			flag = 1;
            		break;
		}
            
        }

    }
    if (!flag) return 0;
    // Calculate F1 F2 D1 D2 M1 M2
    for (i = 0; i<=5; i++)  
    {
    *(Dataint+i) = (float)((Dataint_inbuff[k+1+i*2] << 8) + Dataint_inbuff[k+2 + i*2]);
	//printf("*(Dataint+0)=%f\n",*(Dataint+0));

    }
    //printf("Fr1_d = %f\n",*(Dataint+0));
    //printf("Fr1 = %f\n",(*(Dataint+0)/1023*5 - 2.5)/2.5*20);
    // Throttle For S1
    *(Dataint+6) = (float)(Dataint_inbuff[k+13]);
    // RPM for S1
    *(Dataint+7) = (float)((Dataint_inbuff[k+14] << 8) + Dataint_inbuff[k+15]);
	
    // Throttle For S2
    *(Dataint+8) = (float)(Dataint_inbuff[k+16]);
    // RPM for S2
    *(Dataint+9) = (float)((Dataint_inbuff[k+17] << 8) + Dataint_inbuff[k+18]);
    return 1;
}


float FMint2float(float FMint, float range)
{
    float FMfloat = (FMint/1024*5 - 2.5)/2.5*range;
    return FMfloat;
}
