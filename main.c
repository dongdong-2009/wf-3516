
#include "stdafx.h"
#include "video_config.h"
#include "util.h"
#include "network_gv.h"
#include "ive_main.h"


#include "serial.h"
#include "lkt.h"
#include "DES.h"


#define  MUXCTRL_UART2_RXD  0x200F0088
#define  MUXCTRL_UART2_TXD  0x200F008C
#define  MUXCTRL_GPIO3_0    0x200F012C




time_t g_tm;
int VI_FPS = 30;
int ad_type = GV_IPCAM_HI3516A_OV4689;//GV_IPCAM_HI3518E_MIS1011;// GV_IPCAM_HI3518_AR0130;
extern SysInfo * g_pSysInfo;
extern int g_no_target_ispgain;
extern int g_no_target_again;
extern int g_no_target_shutter;

extern int g_testTimerValue;

extern bool g_show_full_screen;
extern bool g_all_open_led;
//extern int g_randData;
int keyVal_detectCammer = 0; //通过同时按下两个按键--检测摄像头是否需要更换
bool flag_detectCammer = 0; //1-表示更换摄像头
int lkt4200_fd; //打开/dev/ttyUSB0
bool restartcammer_flag = 0;
int fbfd; //打开/dev/fb0
bool flagBr = 0;//0-读run.sh中的值; 1-读文本中的设定值
unsigned char *cur_images_data = NULL;


unsigned char g_target_br = 100;



int findParam(char* param, int argc, const char **argv) {
	int idx = -1;
	int i =0;
	for ( i = 0; i < argc && idx == -1; i++)
		if (strcmp(argv[i], param) == 0) idx = i;
	return idx;

}
//parse command line
//returns the value of a command line param. If not found, defvalue is returned
float getParamVal(char* param, int argc, const char **argv, float defvalue) {
	int idx = -1;
	int i =0;
	for ( i = 0; i < argc && idx == -1; i++)
		if (strcmp(argv[i], param) == 0) idx = i;
	if (idx == -1) return defvalue;
	else return atof(argv[idx + 1]);
}


void processCommandLine(int argc, const char **argv) 
{
	if(NULL == g_pSysInfo)
		return;
	unsigned char m_ImageFlip = getParamVal("-flip", argc, argv, 0); //0

	
	int width = getParamVal("-srcw", argc, argv, 2592);	//摄像头的宽 -srcw 2592
	int height = getParamVal("-srch", argc, argv, 1520);	//摄像头的高 -srch 1520
#ifdef SINGLE_OUTPUT
	width = 2592;
	height = 1520;
#endif
	setParamCameraWH(width, height, m_ImageFlip);
	//printf("width:%d, height:%d, m_ImageFlip:%d\n", width, height, m_ImageFlip);
	
	g_pSysInfo->size_extch.uWidth = width;
	g_pSysInfo->size_extch.uHeight = height;
	g_pSysInfo->is_mirror  = getParamVal("-mirror", argc, argv, 0); //0-左右不镜像   1-左右镜像
	g_pSysInfo->is_flip  = 0;
	g_pSysInfo->backlight_com  = 11;
	g_pSysInfo->exposure_type = OP_TYPE_MANUAL;
	g_pSysInfo->exposure_time_manu = getParamVal("-ss", argc, argv, 20000); //快门
	
	g_pSysInfo->m_vi_rotate = getParamVal("-vi_rotate", argc, argv, ROTATE_NONE); //VI旋转
	g_pSysInfo->m_vo_rotate = getParamVal("-vo_rotate", argc, argv, ROTATE_180); 
	//vi_rotate 0; vo_rotate 2;  flip 1    ---------口袋 
	//vi_rotate 0; vo_rotate 3;  flip 0    ---------手持 
	
	g_all_open_led = getParamVal("-aol", argc, argv, 1); //1：二路红外灯全开  0：只开一路 
	g_show_full_screen = getParamVal("-sfs", argc, argv, 1); //1：显示全局画面  0：不显示全局
	
	g_testTimerValue = getParamVal("-ttv", argc, argv, 2); //1：1分钟 2分钟 ...  默认2分钟 上限五分钟
	if(g_testTimerValue > 5)
		g_testTimerValue = 5;
	
	g_pSysInfo->sharpen = getParamVal("-sh", argc, argv, 21); //锐度
	//printf("sharpen:%d\n", g_pSysInfo->sharpen);
	g_pSysInfo->nf_3d_level = getParamVal("-n3", argc, argv, 300); //3D降噪
	g_pSysInfo->defog_flag = getParamVal("-defog", argc, argv, 1); //去雾
	g_pSysInfo->dis_flag = getParamVal("-dis", argc, argv, 1); //防抖
	g_pSysInfo->dci_flag = getParamVal("-dci", argc, argv, 0); //动态对比度增强
	
	g_pSysInfo->viewcolor.brightness = getParamVal("-br", argc, argv, 35); //亮度
	g_pSysInfo->viewcolor.contrast = getParamVal("-co", argc, argv, 90); //对比度
	g_pSysInfo->viewcolor.saturation = getParamVal("-sa", argc, argv, 100); //饱和度
	g_pSysInfo->viewcolor.hue = getParamVal("-hue", argc, argv, 90); //色度
	
	g_pSysInfo->isp_gain = getParamVal("-ispg", argc, argv, 100); //ISP增益
	g_pSysInfo->a_gain = getParamVal("-ag", argc, argv, 250); //模拟增益
	g_pSysInfo->d_gain = getParamVal("-dg", argc, argv, 0); //数字增益
	
	
	g_target_br = getParamVal("-tbr", argc, argv, 100); //目标亮度值 --自动逼近
	unsigned char no_target_high_br = getParamVal("-nhbr", argc, argv, 70);  //没有目标时，频闪--高亮
	unsigned char no_target_low_br = getParamVal("-nlbr", argc, argv, 20);   //没有目标时，频闪--低亮
	setParamAutoAdjustCameraTargetBrLMH(g_target_br, no_target_high_br, no_target_low_br);
	
	g_no_target_ispgain = getParamVal("-nispg", argc, argv, 250); //没有目标时，频闪--ISP增益
	g_no_target_again = getParamVal("-nag", argc, argv, 400); //没有目标时，频闪--模拟增益
	g_no_target_shutter = getParamVal("-nshutter", argc, argv, 30000);//
	
	///*
	if(g_all_open_led == 1)
	{
		g_no_target_ispgain = g_pSysInfo->isp_gain;
		g_no_target_again = g_pSysInfo->a_gain;
		g_no_target_shutter = g_pSysInfo->exposure_time_manu;
	}
	//*/

	unsigned char target_high_br = getParamVal("-hbr", argc, argv, 80);  //亮度参数上限值
	unsigned char target_low_br = getParamVal("-lbr", argc, argv, 10);   //亮度参数下限值
	setParamAutoAdjustCameraTargetBrThread(target_high_br, target_low_br);
	
	unsigned char save_frist_param_flag = getParamVal("-sfpf", argc, argv, 1);   //是否保存第一下的调光参数
	setParamAutoAdjustCameraTargetSaveParam(save_frist_param_flag);
	
	unsigned char disturbance_rejection_num_thread = getParamVal("-drnt", argc, argv, 3);   ////扰动次数阈值，如果大于该值，则进行重新调光
	setParamAutoAdjustCameraTargetDisturbanceRejection(disturbance_rejection_num_thread);
	
	unsigned char remove_frist_unstable_value_thread = getParamVal("-rfuvt", argc, argv, 0);   ////第一次检测到目标后，需要排除不稳定因子，如果大于该值，则开始调光
	setParamAutoAdjustCameraTargetRemoveFristUnstableValue(remove_frist_unstable_value_thread); 
	
	
	//printf("no_target_high_br:%d, no_target_low_br:%d\n", no_target_high_br, no_target_low_br);
	int target_high_shutter = getParamVal("-hshutter", argc, argv, 28000);//快门参数上限值
	int target_low_shutter = getParamVal("-lshutter", argc, argv, 15000);//快门参数下限值

	setParamAutoAdjustCameraTargetShutterThread(g_no_target_shutter, target_high_shutter, target_low_shutter);
	
	int m_backColorAbs = getParamVal("-abs", argc, argv, 35);
	setParamBackColorAbs(m_backColorAbs);
	//printf("m_backColorAbs:%d\n", m_backColorAbs);
	//unsigned char m_thresholdvalue = getParamVal("-th", argc, argv, 40);
	int m_currentBulkThreshold = getParamVal("-cbt", argc, argv, 80);
	setParamCurrentBulkThreshold(m_currentBulkThreshold);
	//printf("m_currentBulkThreshold:%d\n", m_currentBulkThreshold);
	int m_thresbulknum = getParamVal("-tn", argc, argv, 7);
	setParamThresbulknum(m_thresbulknum);
	//printf("m_thresbulknum:%d\n", m_thresbulknum);
	
	bool save_image_flag = getParamVal("-spic", argc, argv, 0);
	setParamSaveImage(save_image_flag);
	//m_CVBSW = getParamVal("-cvbsW", argc, argv, 576);
	//m_CVBSH = getParamVal("-cvbsH", argc, argv, 768);
	//printf("m_CVBSW:%d, m_CVBSH:%d\n", m_CVBSW, m_CVBSH);
	
	float m_scale = getParamVal("-scale", argc, argv, 1.6);
	int scale = m_scale * 10;
	setParamScale(scale);
	
	restartcammer_flag = getParamVal("-restartcammer", argc, argv, 0);
	flagBr = getParamVal("-rbr", argc, argv, 0);
	//testTimerValue = getParamVal("-ttime", argc, argv, 5);
	//testFlag = getParamVal("-spic", argc, argv, 0);
	
	unsigned char m_whRatio = getParamVal("-whRatio", argc, argv, 1);
	//printf("m_whRatio:%d\n", m_whRatio);
	int m_ROIRectW = getParamVal("-rw", argc, argv, 420);
	int m_ROIRectH = getParamVal("-rh", argc, argv, 420);
	//printf("m_ROIRectW:%d, m_ROIRectH:%d\n", m_ROIRectW, m_ROIRectH);
	float wr = 3.0;
	float hr = 4.0;
	if (m_whRatio)
	{
		float wr = 4.0;
		float hr = 3.0;
	}
	if (m_ROIRectW == 0 && m_ROIRectH == 0)
	{
		m_ROIRectW = 540;
		m_ROIRectH = 720;
	}
	
	if (m_ROIRectW > 0 && m_ROIRectH == 0)
	{
		m_ROIRectH = m_ROIRectW*hr / wr;
	}
	else if (m_ROIRectH > 0 && m_ROIRectW == 0)
	{
		m_ROIRectW = m_ROIRectH*wr / hr;
	}
	g_pSysInfo->size_vo.uWidth = m_ROIRectW;
	g_pSysInfo->size_vo.uHeight = m_ROIRectH;
	
#ifdef SINGLE_OUTPUT
	setParamRoiRectWH(m_ROIRectW, m_ROIRectH);
#endif
	
#ifndef _LKT4200_
	unsigned char temp1 = randNum();
	
	setParamRandNum(temp1 ^ 0xAE);
#endif
	

}



//深圳新版--红外灯IO口
//方向 himm 0x201c0400   0xff
//第1个  GPIOC-7 ---大电阻   himm 0x201c03fc   0x8f
//第2个  GPIOC-6 --小电阻    himm 0x201c03fc   0x4f
//

void zspioInit()
{
	int fd,ret;
	fd = open("/dev/zsgpio", 0);
	if(fd<0)
	{
		sstw_error("Open zsgpio error!");
		//	return -1;
	}
	
	gpioinfo up;
	bzero(&up,sizeof(gpioinfo));	
	up.group_number = 8;
	up.bit_number = 7;
	up.set_value = 1;
	ret = ioctl(fd, ZSGPIO_WRITE, &up);

}

	
bool initPhase()
{
#ifndef YG_BOARD
	initIIC();
#endif
	
#ifdef _HAVECLOCK_
	clockInit();
#endif
#ifdef _LKT4200_
	init4200();
#endif
	
	initIO();
	return 1;
}

int main(int argc, char *argv[])
{
	printf("argc:  %d\n", argc);
	if( already_running() ){
		printf( "app has already runing... exit!\n" );		
		exit(1);
	}
	
	int ret = 0;
	ret = init_system_info();
	if( ret < 0 )
	{
		sstw_error("init system info failed, exit...");
		exit(0);
	}
	
	pkDetectCvInit();//放前面
	processCommandLine(argc, argv);
	initPhase();
		
	ret = init_av_config();
	if( ret )	return 0;
#ifdef YG_BOARD		
	zspioInit(); //深圳板子红外灯IO初始化
#endif	
#ifdef _NETTHREAD	
	ret = init_lan_probe();
	if (ret < 0)
	{
		//exit(0);
		sstw_error(" Fail to init_lan_probe");
	}
	pthread_t network_tid;
	ret = start_task(&network_tid, thread_network_control, (void*)NULL);
#endif	
	




	SAMPLE_VENC_720P_CLASSIC(); 
	
	init_video_decode();
	
	
	detectFun();//开启条码检测
	
	

	
	
	
cleanup:
#ifdef _PRINTF
	dpf( "cleanup\n" );
#endif
	
		
	close_video_decode();

//	CloseAudio();

	cleanup_hi351x();
	
	UnInitSysInfo();
	return HI_SUCCESS;
    
}
	


