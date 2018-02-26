
#ifndef _STDAFX_H_
#define _STDAFX_H_

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/soundcard.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include <poll.h>
#include <linux/watchdog.h>
#include <sys/msg.h>
#include "netdb.h"
#include <sys/vfs.h>
#include <linux/v4l2-dv-timings.h>

#define HI_XXXX


#include "sample_comm.h"
#include "hi_comm_vi.h"
#include "mpi_vi.h"
#include "mpi_region.h"
#include "hi351x_avenc.h"
#include "hi_ae_comm.h"
#include "hi_comm_isp.h"

#include "common2.h"
#include "common.h"

#ifdef SINGLE_OUTPUT
	//#undef _VO_
#else
	#define _VO_
#endif


#define VO_WIDTH	720
#define VO_HEIGHT	576	//576

#ifdef hi3518e
#undef _SNAP_
#undef _VO_
#else
//#define _SNAP_
#endif

#define OV_9712P

#ifdef hi3518e
#define SENSOR_SC2035_DC_1080P_30FPS
#endif

#define FRAME_I    1
#define FRAME_P	  0

#define STREAM_TCP 	1
#define SOCKET_ERROR            (-1)

#define PF_DEBUG

#ifdef PF_DEBUG
#define dpf(fmt, args...)	printf("[%s]--%d: " fmt, __FILE__,__LINE__, ##args)
#else
#define dpf(x...)	
#endif


#if defined(GM812X) || defined(HI_XXXX)	  
extern time_t g_tm;

#define sstw_error(fmt, args...)	{ char stime[64] = {0}; sprintf(stime,"%s",ctime(&g_tm)); stime[strlen(stime)-1] = 0; \
									fprintf(stderr, "\033[41;37m [%s]--%d:" fmt, stime, __LINE__, ##args); \
									fprintf(stderr, "\033[0m \n");	\
									}

#else
#define sstw_error(fmt, args...)	fprintf(stderr, "--%d:" fmt,  __LINE__, ##args)
#endif

typedef struct{
	v_u32	imageWidth[2];
	v_u32	imageHeight[2];	
	pthread_rwlock_t *rwlock;
	pthread_mutex_t	*pMutex;
}VideoEncodeEnv;

typedef enum _MEDIA_TYPE{
	VIDEO = 0,
	AUDIO
}MEDIA_TYPE;

#define sstw_info(fmt, args...)	{ fprintf(stderr, "\033[10;36m " fmt, ##args); \
									fprintf(stderr, "\033[0m \n");	\
									}

#ifdef hi3518e		//hi3518ev200

//#define APTINA_AR0130_DC_720P_30FPS  0
//#define APTINA_9M034_DC_720P_30FPS  1
//#define APTINA_AR0230_HISPI_1080P_30FPS  2
//#define SONY_IMX122_DC_1080P_30FPS 3
//#define SONY_IMX122_DC_720P_30FPS  4
//#define SAMPLE_VI_MODE_1_D1 5
//#define  SAMPLE_VI_MODE_BT1120_720P 6
//#define  SAMPLE_VI_MODE_BT1120_1080P 7
//#define OV_9712_DC_720P_30FPS 8

#endif

/* Thread error codes */
#define THREAD_SUCCESS      (void *) 0
#define THREAD_FAILURE      (void *) -1


/* Cleans up cleanly after a failure */
#define cleanup(x)    status = (void*)(x);   goto cleanup


//#define VIDEO_STD_PAL 

#ifdef VIDEO_STD_PAL
#define V_WIDTH 704
#define V_HEIGHT 576
#define FRAME_RATE 25
#else
#define V_WIDTH 640
#define V_HEIGHT 480
#define FRAME_RATE 30
#endif

/* The levels of initialization */
#define ACCEPTTHREADCREATED   	 0x1
#define CAPTURETHREADCREATED     0x2
#define RWLOCKCREATED            0x4
#define CTLTHREADCREATED         0x8
#define RECORDTHREADCREATED     0xF
#define MDTHREADCREATED     0x10
#define SNAPTHREADCREATED     0x20


typedef struct {
    v_u8 x;
    v_u8 y;
    v_u8 width;
    v_u8 height;
}MBRECT;

//#define PATH_MAX			128

/*----------------------------------------------*
 * The common data type, will be used in the whole project.*
 *----------------------------------------------*/


#ifndef NULL
#define NULL             0L
#endif

#define HI_NULL          0L
#define HI_NULL_PTR      0L

#define HI_SUCCESS          0
#define HI_FAILURE          (-1)

#define MSG_ALARM_NOW	100
#define MSG_ALARM_DONE	101

#define PROJ_ELIU

//#define PROJ_CRTMP

#define closesocket(_x_)	close(_x_)	
#define Sleep(_x_)			usleep(_x_*1000)

// typedef unsigned char           BYTE;
// typedef unsigned short          WORD;
// typedef unsigned int            DWORD;
 typedef unsigned long long      __int64;
 
typedef struct _flv_file_tag
{
	__int64 timestamp_video;	
	__int64 timestamp_audio;
	int last_tag_size;
	int start_h264_2_flv;
	int fps;
	int width;
	int height; 
}flv_file_tag;

#define  TIME_LENGTH	30   //30 mins

#define FLV_CODEC_H264 7 



int AlarmSnap();

int dana_snap();

int dana_snap_done();

HI_S32 SAMPLE_VENC_720P_CLASSIC(HI_VOID);

void cleanup_hi351x();

#endif
