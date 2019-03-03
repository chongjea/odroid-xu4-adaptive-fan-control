#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define TEMPERATURE_FILE "/sys/devices/virtual/thermal/thermal_zone0/temp"
#define FAN_MODE_FILE "/sys/devices/platform/pwm-fan/hwmon/hwmon0/automatic"
#define FAN_SPEED_FILE "/sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1"

#define TP1 63
#define TP2 70
#define TP3 75
#define TP4 80

static FILE *fp_temp, *fp_mode, *fp_speed;

int setFanMode(int mode)
{
	fp_mode=fopen(FAN_MODE_FILE, "wb");
	if (fp_mode==NULL)
	{
		printf("Can't open Fan Mode File\n");
		return -1;
	}
	if (mode==0)	//Manual
		fwrite("0\n", 1, 2, fp_mode);
	else if (mode==1)	//Automatic
		fwrite("1\n", 1, 2, fp_mode);
	fclose(fp_mode);
	fp_mode=NULL;
	return 0;
}

void cleanup(int signum)
{
	setFanMode(1);
	if (fp_temp!=NULL)
		fclose(fp_temp);
	if (fp_mode!=NULL)
		fclose(fp_mode);
	if (fp_speed!=NULL)
		fclose(fp_speed);
	fp_temp=fp_mode=fp_speed=NULL;
	printf("Killed by Signal %d\n", signum);
	exit(0);
}

int main()
{
	char tempA[8]={0,};
	int temp=0;
	char *fan_speed="0\n";
	time_t cool_start=time(NULL)-20; //For Cold-start
	time_t cool_time;
	int n=0;

	if (geteuid()!=0)
	{
		printf("You Must Run As ROOT\n");
		return 0;
	}

	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGILL, cleanup);
	signal(SIGABRT, cleanup);
	signal(SIGTERM, cleanup);

	fp_temp=fopen(TEMPERATURE_FILE, "rb");
	if (fp_temp==NULL)
	{
		printf("Can't open Temperature File\n");
		return 0;
	}
	fp_speed=fopen(FAN_SPEED_FILE, "wb");
	if (fp_speed==NULL)
	{
		printf("Can't open Fan Speed File\n");
		fclose(fp_temp);
		return 0;
	}
	if (setFanMode(0)!=0) //Set Fan Manual Mode
	{
		fclose(fp_temp);
		fclose(fp_speed);
		return 0;
	}

	while (1)
	{
		fread(tempA, 1, 8, fp_temp);
		temp=(tempA[0]-'0')*10 + (tempA[1]-'0');
		//printf("%d\n", temp);
		if (temp>=TP4)
		{
			fan_speed="255\n";
			n=4;
		}
		else if (temp>=TP3)
		{
			fan_speed="200\n";
			n=4;
		}
		else if (temp>=TP2)
		{
			fan_speed="120\n";
			n=4;
		}
		else if (temp>=TP1)
		{
			fan_speed="90\n";
			n=3;
			cool_start=time(NULL);
		}
		else
		{
			//Fan runs 10s
			cool_time=time(NULL)-cool_start;
			if (cool_time>=10)
			{
				fan_speed="0\n";
				n=2;
			}
		}

		fwrite(fan_speed, 1, n, fp_speed);
		fseek(fp_speed, 0, SEEK_SET);
		fseek(fp_temp, 0, SEEK_SET);
		sleep(2);
	}
}
