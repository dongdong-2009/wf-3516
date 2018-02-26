#include "stdafx.h"
#include "ive_main.h"
#include "zlg7290.h"

#include "lkt.h"
#include "hiGpioReg.h"
#include "ht1382.h"

#include "pkDetectCV.h"
#include <signal.h>
#include <linux/fb.h>
#include "sample_comm.h"
#include "hifb.h"
#include "loadbmp.h"
#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"
#define GPIO10_BASE   0x20170000
#define GPIO10_DATA   GPIO10_BASE + 0x3FC
#define GPIO10_4      GPIO10_BASE + 0x40
#define GPIO10_DIR    GPIO10_BASE + 0x400

#define  MUXCTRL_UART2_RXD  0x200F0088
#define  MUXCTRL_UART2_TXD  0x200F008C
#define  MUXCTRL_GPIO3_0    0x200F012C

static HI_BOOL s_bStopSignal = true;
extern SysInfo * g_pSysInfo;
extern VPSS_GRP g_VpssGrp ;
extern int *g_sock;

typedef struct {
	unsigned char year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
}Timer;
char g_activate_time[20];
////////////////////////////////////////////////////////////////////////////////////////////////////////////
int time_waiter = 30;
int testFlag = 0;    //测试模式--1表示为设置模式
int testTimer = 0;
int g_testTimerValue = 2; //测试模式下时间延时计数
int testTimerStatus = 1;


bool running = 1;       //正常运行标志位
volatile sig_atomic_t test_over = 0;
volatile sig_atomic_t closeThread_flag = 0;
bool activate_ok = false;
bool keyTimer = 0; //测试模式下用于按键激活标示
int keyTimerStatus = 0;
bool keyVal = 0;
bool activateUserInfoFlag = 0; //是否有配置用户激活信息

bool g_show_full_screen = 0;//是否显示全局画面

int g_no_target_ispgain = 250;
int g_no_target_again = 400;
int g_no_target_shutter = 30000;
//volatile sig_atomic_t flag_key_end_exe = 1;
////////////////////////////////////////////////////////////////////////////////
int g_period_type_int = -1;
int g_activate_number = 0;
int lkt4200_fd ;

int g_randData = 0;
bool g_all_open_led = 0;//0-没有目标的时候 一高一低     1-全开(全高)

uint8_t *fbp = NULL;
struct fb_fix_screeninfo finfo ;
struct fb_var_screeninfo vinfo;
#define WIDTH_720              720
#define HEIGHT_576             576
int fbfd ;
//----vo----
VB_CONF_S       stVbConf;

#define KEY_NUM 2  //按键个数
int fd_key;
unsigned char key_array[KEY_NUM] = {0};


int m_CVBSW = WIDTH_720;
int m_CVBSH = HEIGHT_576;



//#define VERSION_20171218
//

void RaspiCamDisExit(void)
{
	
	munmap(fbp,finfo.smem_len);
	close(fbfd);	
}

void RaspiCamDisInit(void){
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    HI_U32 u32VoFrmRate;
	VO_PUB_ATTR_S stPubAttr;
	VO_LAYER VoLayer = 2;
	
    SIZE_S  stSize;
	stPubAttr.enIntfSync = VO_OUTPUT_PAL;
	stPubAttr.enIntfType = VO_INTF_CVBS;
	stPubAttr.u32BgColor = 0x0000FF;

    stLayerAttr.bClusterMode = HI_FALSE;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    int s32Ret = SAMPLE_COMM_VO_GetWH(stPubAttr.enIntfSync, &stSize.u32Width, \
									  &stSize.u32Height, &u32VoFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get vo wh failed with %d!\n", s32Ret);
    }
	
    memcpy(&stLayerAttr.stImageSize, &stSize, sizeof(stSize));

    stLayerAttr.u32DispFrmRt = 30 ;
    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.stDispRect.u32Width = stSize.u32Width;
    stLayerAttr.stDispRect.u32Height = stSize.u32Height;

    s32Ret = SAMPLE_COMM_VO_StartDev(SAMPLE_VO_DEV_DSD0, &stPubAttr);
	
    if (HI_SUCCESS != s32Ret)
    {
		SAMPLE_PRT("start vo dev failed with %d!\n", s32Ret);
    }

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr, HI_TRUE);
    if (HI_SUCCESS != s32Ret)
    {
		SAMPLE_PRT("start vo layer failed with %d!\n", s32Ret);
	}
	
	fbfd = open("/dev/fb0", O_RDWR, 0);
    if (fbfd < 0)
    {
        SAMPLE_PRT("open %s failed!\n", "/dev/fb0");
    }
	
	HIFB_ALPHA_S stAlpha;
	stAlpha.bAlphaEnable = HI_TRUE;
	stAlpha.u8Alpha0 = 0xff;
	stAlpha.u8Alpha1 = 0xff;
	if (ioctl(fbfd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
	{
		SAMPLE_PRT("Set alpha failed!\n");
		return HI_NULL;
	}
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        SAMPLE_PRT("Get variable screen info failed!\n");
        close(fbfd);
        return HI_NULL;
    }
	
	//printf("\nx is %d y is %d\n",vinfo.xres,vinfo.yres);
	vinfo.xres_virtual = WIDTH_720;
	vinfo.yres_virtual = HEIGHT_576;
	vinfo.xres = WIDTH_720;
	vinfo.yres = HEIGHT_576;
	
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) < 0)
    {
        SAMPLE_PRT("Put variable screen info failed!\n");
        close(fbfd);
        return HI_NULL;
    }
	
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        exit(EXIT_FAILURE);
    }
	fbp = mmap(0,
			   finfo.smem_len,
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED,
			   fbfd,
			   0);
	
	if((int)fbp == -1)
	{
		printf("fb0 can not open\n");
		return -1;
	}
}


void RaspiCamCvShowImage(unsigned char* pixelp, int width, int height)
{ 
	if (pixelp == NULL)
	{
		return;
	}
	int vo_w = WIDTH_720;
	int vo_h = HEIGHT_576;
	int y = 0 ;
	int fb_offset = 0 ;
	int image_offset = 0 ;
	int r = 0 ;
    int g = 0 ;
    int b = 0 ;	

	int r_mask = (1 << vinfo.red.length) - 1;
    int g_mask = (1 << vinfo.green.length) - 1;
    int b_mask = (1 << vinfo.blue.length) - 1;

    int bytes_per_pixel = vinfo.bits_per_pixel / 8;
	
	{
		for(y = 0 ; y < vo_h; y++)
		{
			int x ;
			
			fb_offset = (vinfo.xres) * y * 2 ;
			
			for(x = 0; x < vo_w; x++)
			{

				r = ((*pixelp)    * r_mask / 0xFF);
				g = ((*(pixelp))  * g_mask / 0xFF);
				b = ((*(pixelp))  * b_mask / 0xFF);

				*((uint16_t *)(fbp+fb_offset)) = (b<<10)|
												 (g<<5) |
												 (r<<0);

				fb_offset += 2 ;
				pixelp    += 1 ;
				
			}
			
		}
	}	
	
}




unsigned char showKeyNumber(unsigned char key_number)
{
	if (key_number == 4)
		return 1;
	else if (key_number == 12)
		return 2;
	else if (key_number == 20)
		return 3;
	else if (key_number == 3)
		return 4;
	else if (key_number == 11)
		return 5;
	else if (key_number == 19)
		return 6;
	else if (key_number == 2)
		return 7;
	else if (key_number == 10)
		return 8;
	else if (key_number == 18)
		return 9;
	else if (key_number == 1)
		return 10;
	else if (key_number == 9)
		return 0;
	else if (key_number == 17)
		return 11;
}

bool verifyActivateCodeNoClock(int *activate_code)
{
#ifndef _MSC_VER
	if (-1 == access("/sbin/.s",0))
	{
		return 0;
	}
	char serial_code[50];
	int fd1 = open("/sbin/.s", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	int ch = read(fd1, serial_code, 8);
	close(fd1);
	
	memset(activate_code, 0, sizeof(int)*6);
	generateActivateCode(serial_code, activate_code);
	return 1;
#endif
	
}




void* thread_key_control()
{
	//
#ifndef YG_BOARD	

	/*	memset(g_activate_time,0, 20);
	 int diff_day = getPeriodDayOver(lkt4200_fd, g_period_type_int, timer, g_activate_time);
	 if(diff_day == 0)
	 {
	  printf("activate code timeout!\n");
	  g_period_type_int = 0; //激活码过期
	 }
	 printf("the spare time is : %d\n", diff_day);
	*/	
	bool have_key_flag = 0;
	//system("himm 0x200F0070 1;himm 0x200F0074 1");
	unsigned char ret = zlg7290check();
	if (1 == ret )
		have_key_flag = 1;
#ifndef _HAVECLOCK_
	if(0 == have_key_flag)
		return;
#endif //_HAVECLOCK_
	while(1)
	{
		if( 1 == closeThread_flag)
			break;
		if(have_key_flag) //有按键键盘接入
		{
			unsigned char key_number = zlg7290Read();
			//printf("key_number: %d\n", key_number);
			while (key_number > 0)
			{
				usleep(200000);
				unsigned char cur_key_number = zlg7290Read();

				if (key_number != cur_key_number)
				{
					break;
				}
			}
			if (key_number > 0)
			{
				unsigned char show_key_number = showKeyNumber(key_number);
				//input_activate_code[input_code_num++] = show_key_number;
				//
				if (show_key_number == 11)
				{
					test_over = 1;
					break;
				}
			}
			
		}

		if(g_period_type_int > 0)
		{
#ifdef _HAVECLOCK_	
			//实时监测试用时间
			char timer[50]={0};
			getTime(timer);
			//printf("timer:%s, g_activate_time:%s\n",timer, g_activate_time);
			int diff_day = diffDay(lkt4200_fd, timer, g_activate_time, g_period_type_int);
			if(diff_day == 0)
			{
				//printf("activate code timeout!\n");
				g_period_type_int = 0; //激活码过期
				test_over = 1;
				break;
			}
#endif //_HAVECLOCK_
			//printf("the spare time is : %d\n", diff_day);
		}
		usleep(200000);
	}
#endif //YG_BOARD
}


void testOverFun() //测试结束
{
	SAMPLE_COMM_ISP_Stop();
	usleep(500000);
	//return
	RaspiCamDisInit();
	usleep(500000);
	gpioClr(13,3);	
	gpioClr(13,4);
	
	
	
	int act_num = 0;
	char MyStr[5];
	int fd1 = open("/sbin/.actnum", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	int ret = read(fd1, MyStr, 5);
	if(ret >= 0 )
	{
		act_num = atoi(MyStr);
	}
	//printf("act_num:%d\n", act_num);
	close(fd1);
	if (act_num > 14)
	{
		return;
	}
	


	int text_num = 0;
	char text_buf[12];
	strcpy(text_buf, "- - - - - -");

	unsigned char  *target_images_data = NULL;
	target_images_data = keyShowNumberByCVBS(text_buf);
	RaspiCamCvShowImage(target_images_data, m_CVBSW, m_CVBSH);
	int input_activate_code[6];
	int input_code_num=0;
	memset(input_activate_code, 1, sizeof(int)*6);
	while (1)
	{
		unsigned char key_number = zlg7290Read();
		//printf("key_number:%d\n", key_number);
		while (key_number > 0)
		{
			usleep(200000);
			unsigned char cur_key_number = zlg7290Read();

			if (key_number != cur_key_number)
			{
				break;
			}
		}
		if (key_number > 0)
		{
			unsigned char show_key_number = showKeyNumber(key_number);
			input_activate_code[input_code_num++] = show_key_number;
			//printf("key number: %d\n", show_key_number);
			char str[1];
			sprintf(str, "%d", show_key_number);
			if (text_num < 11)
			{
				text_buf[text_num++] = str[0];
				text_num++;

				target_images_data = keyShowNumberByCVBS(text_buf);

			}
			if (show_key_number == 10) //取消
			{
				text_num = 0;
				input_code_num = 0;
				memset(input_activate_code, 1, sizeof(int)*6);
				strcpy(text_buf, "- - - - - -");
				target_images_data = keyShowNumberByCVBS(text_buf);
			}
			if (show_key_number == 11) //确认
			{
#ifdef _HAVECLOCK_				
				if (0 > g_period_type_int)
				{
					//printf("oh my god\n");
					system("rm -rf /app");
					system("rm -rf /usr/lib");
					system("reboot");
					
					return;
				}
				
				char timer[50]={0};
				getTime(timer);
				int success_flag =verifyActivateCode(lkt4200_fd, input_activate_code, timer, &g_period_type_int, &g_activate_number);
#else
				int activate_code[6];
				bool exit_flag = verifyActivateCodeNoClock(activate_code);
				if (0 == exit_flag)
				{
					system("rm -rf /app");
					system("rm -rf /usr/lib");
					system("reboot");
					return;
				}
				bool success_flag = 0;
				int i=0;
				for (; i < 6; i++)
				{
					//printf("ack: %d, %d\n", activate_code[i], input_activate_code[i]);
					if (activate_code[i] != input_activate_code[i])
					{
						success_flag = 1;
						break;
					}
				}
#endif	//_HAVECLOCK_			
				if (0 == success_flag)
				{
#ifdef _HAVECLOCK_	
					int period_day = getPeriodDayByPeriodType(g_period_type_int);
					int activateType = getMachineActivateType(g_period_type_int);
					if(activateType == 1)
					{
						target_images_data = activatePeriodNum(NULL);
					}
					else
					{
						char act_buf[3];
						sprintf(act_buf, "%d", period_day);
						target_images_data = activatePeriodNum(act_buf);
					}
#else
					target_images_data = activateSuccess(NULL);
#ifdef _LKT4200_
					deviceActivate(lkt4200_fd);
#endif //_LKT4200_
#endif //_HAVECLOCK_
					RaspiCamCvShowImage(target_images_data, m_CVBSW, m_CVBSH);

					if (0 == running)
					{
						testTimerStatus = 0;
						running = 1;
						activate_ok = true;
					}
					text_num = 0;
					input_code_num = 0;
					memset(input_activate_code, 1, sizeof(int)*6);

					break;
				}
				else
				{
					int act_num = 0;
					
					fd1 = open("/sbin/.actnum", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
					int ret = read(fd1, MyStr, 5);
					if(ret >= 0 )
					{
						act_num = atoi(MyStr);
					}
					close(fd1);
					
					act_num++;
					
					char MyStr1[20] = { '\0' };
					int len = sprintf(MyStr1, "%d\n", act_num);
					fd1 = open("/sbin/.actnum", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
					write(fd1, MyStr1, len - 1);
					int sync = fsync(fd1);
					close(fd1);

					
					
					if (act_num > 14)
					{
						// printf("Oh My Goal!!! \n");
						system("rm -rf /app");
						system("rm -rf /usr/lib");
						system("reboot");
						break;
					}
					
					char act_buf[3];
					sprintf(act_buf, "%d", 15-act_num);
					target_images_data = activateFailureNum(act_buf);
					RaspiCamCvShowImage(target_images_data, m_CVBSW, m_CVBSH);
					
					sleep(3);

					text_num = 0;
					input_code_num = 0;
					memset(input_activate_code, 1, sizeof(int)*6);
					strcpy(text_buf, "- - - - - -");
					target_images_data = keyShowNumberByCVBS(text_buf);
				}
			}
			//	target_images_data = activateFailureNum("15");
			//	target_images_data = activateSuccess(NULL);

			RaspiCamCvShowImage(target_images_data, m_CVBSW, m_CVBSH);

		}
		usleep(200000);
	}
	
	
	system("opencam.sh");
	s_bStopSignal = true;
	//printf("success...\n");
}


void printSerialCode()
{
#ifndef _MSC_VER
	char *str = cmd_system("hostid | cut -f 7");
	char serial[80]=" 00000000";
	strcat(serial, str);
	char serial_code[8];
	int activate_code[6];
	memset(serial_code, 0, 8);
	memset(activate_code, 0, sizeof(int)*6);
	//generateActivateCode(serial, serial_code, activate_code);
	//printf("serial code: %s\n", serial_code);
	/*	int i = 0;
	 for (; i < 6; ++i)
	 {
		
	  printf("avtivate code11:%d\n", activate_code[i]);
	 }
	 */	
#endif
}

void sigalrm_fn(int sig)
{
	//printf("sigalarm .......\n"); 
	if (testTimerStatus)
	{
		testTimer++;
		if (testTimer >= g_testTimerValue)
		{
#ifdef _PRINTF
			printf("test over\n");
#endif
			testTimerStatus = 0;
			test_over = 1;
			//running = 0;
			// testTimer = 0 ;
			alarm(0);
			//system(" fbi -T 2 -d /dev/fb0 -noverbose -a  /app/test.png");
		}
		else{
			alarm(60);
		}

	}

#ifdef _KEY_ACTIVATE
	if (keyTimerStatus)
	{
		keyTimerStatus = 0;
		keyTimer = 1;
	}
#endif
	
#ifdef _PRINTF
	printf("activate is over!\n");
#endif
}



void signal_key(int signum)
{
    
    //unsigned int i;

    read(fd_key, &key_array, sizeof(key_array));

    
	if( (key_array[0] == 0) || (key_array[1] == 1) )
	{
#ifdef _PRINTF
		printf("key1 down\n");
		printf("key2 down\n");
#endif
		keyVal = 1;
		alarm(10);
		keyTimerStatus = 1;
	}
	else if( (key_array[0] == 0x80) || (key_array[1] == 0x81) )
	{
#ifdef _PRINTF
		printf("key1 up\n");
		printf("key2 up\n");
#endif
		if (keyVal)
		{
			keyVal = 0;
		
			if (keyTimer)
			{
				alarm(0);
				keyTimer = 0;
				
				
				unsigned char  *target_images_data = activateSuccess(NULL);
#ifdef _LKT4200_
				deviceActivate(lkt4200_fd);
#endif //_LKT4200_

				RaspiCamCvShowImage(target_images_data, m_CVBSW, m_CVBSH);
				sleep(5);
				if (0 == running)
				{
					testTimerStatus = 0;
					running = 1;
					activate_ok = true;
				}

#ifdef _PRINTF
				printf("activate is ok!\n");
#endif
				
				system("opencam.sh");
				s_bStopSignal = true;
			}
		}

	}


}




void initIIC()
{
#ifdef VERSION_20170802
	system("himm 0x200F0070 1;himm 0x200F0074 1");
#endif
#ifdef VERSION_20171218
	system("himm  0x200F0060 2;himm  0x200F0064 2");
#endif
}

//按键IO初始化
void keyInit()
{ 
#ifdef _KEY_ACTIVATE
	system("rmmod btn_drv.ko");
	system("insmod btn_drv.ko");
	int flag;
		   
	fd_key = open("/dev/buttons", O_RDWR);
	if (fd_key < 0)
	{
		printf("can't open!\n");
	}
	signal(SIGIO, signal_key);
	fcntl(fd_key, F_SETOWN, getpid());
	flag = fcntl(fd_key, F_GETFL);
	fcntl(fd_key, F_SETFL, flag|FASYNC);
#endif
}

	   
//时钟芯片初始化	   
void clockInit()
{
#ifndef YG_BOARD
	//printf("clockInit--1\n");
	ht1382Init();
	//printf("clockInit--2\n");
#endif
}

void setClockInitTime()
{
#ifndef YG_BOARD
	Timer t ;
	t.second  = 0x59;//秒
	t.minute  = 0x04;//分
	t.hour    = 0x22 ;//时
	t.day = 0x23;//日
	t.month=0x06;//月
	t.year=0x15;//年
	setTime(&t);
#endif
	//char timer[100]={0};
    //getTime(timer);
	//printf("timer:%s\n",timer);
}

void init4200()
{
	
	char buf[1024];
	int len =  0,j=0;
	unsigned char cmd_write[64] = {0x00, 0x84, 0x00, 0x00, 0x08};
	unsigned char cmdStart[5] = {0x80,0x08,0x00,0x00,0x01};
	unsigned char cmdGetId[1] = {0x04};
	unsigned char cmdData[9] = {0x01,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
	unsigned char readCmd[5] = {0x00,0xc0,0x00,0x00,0x08};
	unsigned char Re_buf[20]  ;
	//管脚配置
	reg_write(MUXCTRL_UART2_RXD,0x01);
	reg_write(MUXCTRL_UART2_TXD,0x01);
	reg_write(MUXCTRL_GPIO3_0,0x01);

	lkt4200_fd = lktSerialOpen("/dev/ttyAMA2",9600);
	lkt4200_init(lkt4200_fd);
	checkLKT4200(lkt4200_fd);
	
	
	
	unsigned char temp1 = randNum();
	g_randData = temp1 ^ 0xAE;
#ifdef _LKT4200_
	unsigned char temp = lkt4200ks(lkt4200_fd, temp1);
	//printf("temp: %d, temp1:%d\n", temp, temp1);
	if (((temp ^ 0xAE) - temp1) * 20 != 0)
	{
		system("reboot");
	}
	g_randData = temp;
#endif
	setParamRandNum(g_randData);
	//clearMem(lkt4200_fd); //////////////////////////////////////////////////////
#ifdef _LKT4200_	
#ifdef _HAVECLOCK_	
	char path[] = "/app/actInfo.txt";
	
	
	int ret = getPeriodType(lkt4200_fd, &g_period_type_int, &g_activate_number);
	//printf("g_period_type_int : %d, g_activate_number:%d\n", g_period_type_int, g_activate_number);
	if( ret < 0)//还没有配置激活信息进4200
	{
		
		if (-1 == access(path,0))
		{
			//激活配置文件没有找到---程序不执行检测
			running = 0;
		}
		else
		{
			printf("Write configuration activation informationing\n");
			int ret = processPeriodInfo(lkt4200_fd, path);
			g_period_type_int = 0;
			
			char hex_hostid[4] = {0x11,0x12,0x13,0x14};
			readBCDHostId(hex_hostid);
			setId(hex_hostid);
			setClockInitTime();
			//printf("write:0x%02X 0x%02X 0x%02X 0x%02X\n",hex_hostid[0],hex_hostid[1],hex_hostid[2],hex_hostid[3]);
			system("rm -rf /app/actInfo.txt");
			printf("write configuration end!\n");
			//verifyActivateCode(lkt4200_fd, "", "2017-09-04-23-02-45");
		}
	}
	else
	{
		char readIdBuf[4]={0};
		memset(readIdBuf,0, 4);
		getId(readIdBuf);
		//printf("0x%02X 0x%02X 0x%02X 0x%02X\n",readIdBuf[0],readIdBuf[1],readIdBuf[2],readIdBuf[3]);
		ret = compareHostId(readIdBuf);
		if(ret < 0)
		{
			//printf("oh my god!\n");
			system("rm -rf /app");
			system("rm -rf /usr/lib");
			system("reboot");
			while(1){usleep(200000);}
			//return;
			
		}
		//EX_DAY0, EX_DAY1, EX_DAY3, EX_DAY5, EX_DAY7, EX_DAY30, EX_DAY356, EX_DAY_FOREVER
		//已结有注册---判断试用类型
		int activateType = getMachineActivateType(g_period_type_int);
		if(0 == activateType)
			g_period_type_int = 0;
		else if(1 == activateType)
		{
#ifdef _PRINTF
			printf("device is activated...\n");
#endif
			testTimerStatus = 0;
			return; //已经永久激活
		}
		else if(2 == activateType)
		{
			char timer[20]={0};
			getTime(timer);
			//printf("timer:%s\n", timer);

			memset(g_activate_time,0, 20);
			int diff_day = getPeriodDayOver(lkt4200_fd, g_period_type_int, timer, g_activate_time);
			if(diff_day == 0)
			{
				//printf("activate code timeout!\n");
				g_period_type_int = 0; //激活码过期
			}
			//printf("the spare time is : %d\n", diff_day);
			//printf("g_period_type_int : %d, g_activate_time:%s\n", g_period_type_int, g_activate_time);
		}
		//printf("sizeof(int): %d\n", sizeof(int));
	}
	if(g_period_type_int == 0)
	{
		signal(SIGALRM, sigalrm_fn);
		alarm(60);
	}

	pthread_t key_tid;
	start_task(&key_tid, thread_key_control, (void*)NULL);
#else
	if (deviceTestActivate(lkt4200_fd) == 0)
	{
		testTimerStatus = 0;
	}
	else
	{	
		
		keyInit();
		
		
		unsigned char ret = zlg7290check();
		if (1 == ret )
		{
			pthread_t key_tid;
	    	start_task(&key_tid, thread_key_control, (void*)NULL);
		}
		signal(SIGALRM, sigalrm_fn);
		alarm(60);
	}
#endif
#endif

	//while(1){usleep(200000);}
}


void initIO()
{
//	printf("gpio test\n");

	
#ifdef YG_BOARD
	system("himm 0x201c0400   0xff");
	if (g_all_open_led == 1)
		system("himm 0x201c03FC   0xff");
	else
		system("himm 0x201c03FC   0x8f");
	
#else
	gpioSetMode(13,3,GPIO_OUTPUT,1);	 
	gpioSetMode(13,4,GPIO_OUTPUT,1);
	
	
#ifdef VERSION_20170802
	if (g_all_open_led == 1)
	{
		gpioSet(13,3);
		gpioSet(13,4);
	}
	else
	{
		gpioSet(13,3);
		gpioClr(13,4);
	}
#endif
#ifdef VERSION_20171218
	if (g_all_open_led == 1)
	{
		gpioSet(13,3);
		gpioSet(13,4);
	}
	else
	{
		gpioClr(13,3);
		gpioSet(13,4);
	}
#endif	
	
	
	

	
	
#endif

}
void runFlagFun()
{
#ifndef _MSC_VER
	if (1 == test_over)  //测试结束
	{
		test_over = 0;
		closeThread_flag = 1;//关闭键盘检测线程
		running = 0;
		//system(" fbi -T 2 -d /dev/fb0 -noverbose -a  /app/test.png");
		testOverFun();
	}

#endif
}


void openCameraInit()
{
	set_picture_a_gain(g_no_target_again);
	set_picture_isp_gain(g_no_target_ispgain);
	set_picture_expo_time_mamu(g_no_target_shutter);

}

void adjustBr()
{
#ifdef _PRINTF
	dpf( "br=%d, shutter=%d, gray=%d\n", g_pSysInfo->viewcolor.brightness, g_pSysInfo->exposure_time_manu, gray);
#endif	

	char buffer[11] = { 0x61, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 };
	struct NETDATA *nd = (struct NETDATA*)buffer;
	COLOR_CONFG *cc = (COLOR_CONFG *)nd->data; 
	cc->brightness = g_pSysInfo->viewcolor.brightness;
	cc->contrast = g_pSysInfo->viewcolor.contrast;
	cc->saturation = g_pSysInfo->viewcolor.saturation;
	cc->hue = g_pSysInfo->viewcolor.hue;
	
	ConfVideoColor_save((void*)cc);
}

void adjustShutter()
{
#ifdef _PRINTF
	dpf( "br=%d, shutter=%d, gray=%d\n", g_pSysInfo->viewcolor.brightness, g_pSysInfo->exposure_time_manu, gray);
#endif
	set_picture_expo_time_mamu(g_pSysInfo->exposure_time_manu);
}

void setCVBSOutput(int x, int y)
{	
#ifdef _VO_
	if(0 == g_show_full_screen)
	{
		static int pre_x = 1, pre_y = 1;
		if(x == 0 || y == 0)
		{
			
			x = pre_x;
			y = pre_y;
		}
		else
		{
			pre_x = x;
			pre_y = y;
		}
	}
	
	int x_cvbs = x;
	if(g_pSysInfo->is_mirror == 1)
	{
		//左右镜像
		x_cvbs = 2592 - x_cvbs;
	}
	
	set_vpss_chncrop(g_VpssGrp,VPSS_CHN_VO, x_cvbs, y);
#endif
}

void sendInfoToPcByNet(unsigned char gray)
{
#ifdef _NETTHREAD	
	if(g_sock)
	{
		char buffer[6] = { 0xc4, 0x00, 0x00, 0x00, 0x06, 0x00 };
		struct NETDATA * nd = (struct NETDATA *)buffer;
		nd->operate = gray;
		int sent = send( *g_sock, nd, nd->len, 0 );
	}
#endif	
}

void updateStateByDetectResult(int x, int y)
{
	static int s_x = -1, s_y = -1;
	if (s_x == x && s_y == y)
		return
			s_x = x;
	s_y = y;
	static bool frist_detect_flag = 1;
	if (x == 0 && y == 0)  //没有目标时
	{
		if (g_all_open_led == 0)
		{
			if (0 == frist_detect_flag)
			{
				frist_detect_flag = 1;
				set_picture_a_gain(g_no_target_again);
				set_picture_isp_gain(g_no_target_ispgain);
#ifdef YG_BOARD
				if (g_all_open_led == 1)
					system("himm 0x201c03FC   0xff");
				else
					system("himm 0x201c03FC   0x8f");
#else	
#ifdef VERSION_20170802
				if (g_all_open_led == 1)
				{
					gpioSet(13,3);
					gpioSet(13,4);
				}
				else
				{
					gpioSet(13,3);
					gpioClr(13,4);
				}
#endif
#ifdef VERSION_20171218
				if (g_all_open_led == 1)
				{
					gpioSet(13,3);
					gpioSet(13,4);
				}
				else
				{
					gpioClr(13,3);
					gpioSet(13,4);
				}
#endif					
#endif
			}
		}
	}
	else
	{
		if (g_all_open_led == 0)
		{
			if (1 == frist_detect_flag)
			{
				//第一次检测到目标的时候
				frist_detect_flag = 0;
				set_picture_a_gain(g_pSysInfo->a_gain);
				set_picture_isp_gain(g_pSysInfo->isp_gain);
#ifdef YG_BOARD
				system("himm 0x201c03FC   0xff");
#else
				gpioSet(13,3);	
				gpioSet(13,4);
#endif
			}
		}
	}
	
	
}


targetCoordinate detectAlgorithm(char* Image_data_ptr)
{
	targetCoordinate cor;
	cor.x=0;cor.y=0;
	if(NULL == Image_data_ptr)
		return cor;
	cor = pkDetectCvFun(Image_data_ptr);
	
	return cor;
	
}

void detectFun(void)
{
	
	int frist_shutter = g_pSysInfo->exposure_time_manu;
	HI_S32 s32Ret = 0;
	HI_S32 s32GetFrameMilliSec = 40;  //steven 2017-01-13  2000;
#ifdef SINGLE_OUTPUT
	VPSS_CHN VpssChn = VPSS_CHN_MAIN;
	RaspiCamDisInit();
#else
    VPSS_CHN VpssChn = VPSS_EXT_CHN2;//VPSS_CHN_MAIN;//VPSS_EXT_CHN2;//VPSS_CHN_SUB;	//sub stream, vga
#endif
	VI_CHN		ViChn = 0;
	VIDEO_FRAME_INFO_S	stFrameInfo;
	int u32Size = 0;

	s_bStopSignal = false;

	int u32Depth = 1; //steven 2017-01-13  3;
	//s32Ret = HI_MPI_VI_SetFrameDepth(ViChn, u32Depth);
	s32Ret = HI_MPI_VPSS_SetDepth(g_VpssGrp,VpssChn,u32Depth);
	if (HI_SUCCESS != s32Ret)
	{
		sstw_error("HI_MPI_VPSS_SetDepth failed, err = 0x%x!", s32Ret);
	}


	openCameraInit();




	unsigned char g_pre_br=g_pSysInfo->viewcolor.brightness;
	int g_pre_shutter = g_pSysInfo->exposure_time_manu;
	while (false == s_bStopSignal)
    {    
		if (running)
		{		
			s32Ret = HI_MPI_VPSS_GetChnFrame( g_VpssGrp, VpssChn, &stFrameInfo, s32GetFrameMilliSec);       
			//s32Ret = HI_MPI_VI_GetFrame(ViChn, &stFrameInfo, s32GetFrameMilliSec);
			if (HI_SUCCESS != s32Ret)
			{
//#ifdef _PRINTF
				sstw_error("HI_MPI_VPSS_GetChnFrame fail,Error(%#x)",s32Ret);
//#endif
				usleep(s32GetFrameMilliSec);
				continue;
			}
			if (u32Size == 0)
				u32Size = (stFrameInfo.stVFrame.u32Stride[0]) * (stFrameInfo.stVFrame.u32Height) * 3 / 2;

			
			
			static int updateCnt = 0;
			
			if (updateCnt > 0)
			{
				updateCnt--;
				HI_MPI_VPSS_ReleaseChnFrame( g_VpssGrp, VpssChn, &stFrameInfo);
				usleep(s32GetFrameMilliSec);
				continue;
			}


			
			/*
			 
			   HI_MPI_VPSS_ReleaseChnFrame( g_VpssGrp, VpssChn, &stFrameInfo);
			   usleep(s32GetFrameMilliSec);
			 
			   continue;
			 
			*/
			char* data_ptr = stFrameInfo.stVFrame.u32PhyAddr[0];
			char* Image_data_ptr = (HI_CHAR*) HI_MPI_SYS_Mmap(data_ptr, u32Size);
			if (HI_NULL == Image_data_ptr)
			{
				sstw_error("HI_MPI_SYS_Mmap failed");
				return;
			}


			targetCoordinate cor = detectAlgorithm(Image_data_ptr);
			unsigned char pre_br = g_pSysInfo->viewcolor.brightness;
			int pre_shutter = g_pSysInfo->exposure_time_manu;	
			autoAdjustCammerParam(&g_pSysInfo->viewcolor.brightness, &g_pSysInfo->exposure_time_manu, NULL);
			if (pre_br != g_pSysInfo->viewcolor.brightness){
				updateCnt = 3;
				adjustBr();
			}

			if (pre_shutter != g_pSysInfo->exposure_time_manu){
				updateCnt += 2;
				adjustShutter();
			}
			
			unsigned char gray = 0;
			pkReturnAvrGray(&gray, NULL, NULL);
			sendInfoToPcByNet(gray);
			//printf("gray-br-shutter: %d, %d(%d), %d(%d)\n", gray, pre_br, g_pSysInfo->viewcolor.brightness, pre_shutter, g_pSysInfo->exposure_time_manu);
			/*
			   if (g_pre_br != g_pSysInfo->viewcolor.brightness || g_pre_shutter != g_pSysInfo->exposure_time_manu)
			   {
				g_pre_shutter = g_pSysInfo->exposure_time_manu;
				g_pre_br = g_pSysInfo->viewcolor.brightness;
				updateCnt = 0;
			   }
			 
			*/
			
#ifdef SINGLE_OUTPUT
			char *pixelp1 = pkDetectCvFunReturnRoi(Image_data_ptr, 0);
			RaspiCamCvShowImage(pixelp1, m_CVBSW, m_CVBSH);
#else
			
#endif
			
			int x = cor.x;
			int y = cor.y;
			updateStateByDetectResult(x, y);
			
			setCVBSOutput(x, y);
			

			



			HI_MPI_SYS_Munmap(Image_data_ptr, u32Size);
			
			
			s32Ret = HI_MPI_VPSS_ReleaseChnFrame( g_VpssGrp, VpssChn, &stFrameInfo);
			if(s32Ret!=HI_SUCCESS)
			{
				sstw_error("release frame failed: %d",s32Ret);
				return s32Ret;
			}
			
			runFlagFun();
		}
		else
		{
			//usleep(10000);
		}
		usleep(10000);
	}
	return NULL;
}



