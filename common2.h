#ifndef _COMMON_H
#define _COMMON_H

#define TRUE		1
#define FALSE		0

#define HTTP_OPTION_CHANGED		0x0001
  
#define OPTION_OK		"OK "
#define OPTION_NG		"NG "
#define OPTION_NS		"NS "
#define OPTION_UW		"UW "
#define OPTION_UA		"UA "

#define MIN_HTTP_PORT		1
#define MAX_HTTP_PORT		65535

#define IPC_CFG_FILE       "/mnt/mtd/etc/ipc.cfg"

typedef enum { false=0, true=!false } bool;

#define MAX_DOMAIN_NAME		40 ///< Maximum length of domain name. Ex: www.xxx.com
#define MAX_LANCAM_TITLE_LEN	16 ///< Maximum length of LANCAM title.
#define MAX_FQDN_LENGTH	256 ///< Maximum length of FQDN.
#define MAX_STRING_LENGTH	256 ///< Maximum length of normal string.
#define MAC_LENGTH	6 ///< Length of MAC address.
#define ACOUNT_NUM	10 ///< How many acounts which are stored in system.
#define SCHDULE_NUM	10 ///< How many schedules will be stored in system.

#define USER_LEN			32 ///< Maximum of acount username length.
#define PASSWORD_LEN		32 ///< Maximum of acount password length.

#define MOTION_BLK_LEN	(10) ///< Motion block size
#define FLG_UI_EXT (1 << 0)
#define FLG_UI_MOTION (1 << 1)
#define FLG_UI_RECORD (1 << 2)
#define FLG_UI_AVI (1 << 3)
#define FLG_UI_JPG (1 << 4)

#define BITRATE_DEF	1025

#define E_NO_SIGNAL		-8

//typedef unsigned char bool;
typedef unsigned char v_u8;
typedef char		  v_s8;
typedef unsigned short v_u16;
typedef unsigned int v_u32;
//typedef unsigned long long v_u64;

#define VIDEO_NORM_NTSC 	1
#define VIDEO_NORM_PAL	  	0
#define VIDEO_NORM_NUKNOW	3

#define MSG_KEY			0x12345
#define SYS_MSG_KEY		0x36f9 ///< Message key used by system server.
#define SEND_MSG_KEY		0x36c8 ///< Message key for send configuration to CGI; added by collins
#define SEND_SHM_KEY		0x36d6 ///< Share memory key for send configuration CGI; added by collins
#define SEM_KEY			0x36b4 ///< Sem key for synchronize
#define FILE_MSG_KEY	0xc54be5 ///< File message key.
#define ALARM_KEY		0x542c7ae6 ///< Alarm message key.
#define AVCONFG_KEY      0x46cc		/// Video message key

//typedef enum { false=0, true=!false } bool;

//spring_codec
typedef enum {
	GV_CODEC_ID_MPEG = 0,
	GV_CODEC_ID_H264,
	GV_CODEC_ID_HI264,
	GV_CODEC_ID_HI265,
	GV_CODEC_ID_HIMJPEG

}gv_codec_id;

//spring_codec
typedef enum {
	GV_RC_VBR = 0,
	GV_RC_CBR,
	GV_RC_FIXQP,
	GV_RC_END
}gv_codec_rc_mode;

//spring_codec
typedef enum {
	GV_AUDIO_CODEC_G711A = 0,
	GV_AUDIO_CODEC_HIG711A,	
}gv_audio_codec_id;


#define PORTKEY     "Port"

#define KILLBOA	"killall -9 boa"
#define RUNBOA	"boa"

#define COMP_GV

typedef enum {
	AUTHORITY_ADMIN = 0,
	AUTHORITY_OPERATOR,
	AUTHORITY_VIEWER,
	AUTHORITY_NONE = 9
} AUTHORITY;



typedef struct gpio_set
{	
	int group_number;
	int bit_number;
	int set_value;
}gpioinfo;

typedef struct 
{	
	int group_number;
	int bit_number;
}gpio_info_t;

#define ZSGPIO_READ  0x04
#define ZSGPIO_WRITE  0x03

typedef struct __HTTP_OPTION {
	char			*name;
	int			(*handler)(void* );
	AUTHORITY	authority;
	int			now;
	int			visiable;
	struct __HTTP_OPTION *next;
} HTTP_OPTION;

#define MAX_CMD_HASH_SIZE		64
typedef struct __CMD_HASH_TABLE {
	HTTP_OPTION *entry;
} CMD_HASH_TABLE;


typedef struct{
	char id[USER_LEN];
	char password[PASSWORD_LEN];
}login_data_t;
/**
* @brief User authority
*/
typedef struct{
	char user_id[USER_LEN];
	int	authority;
}get_user_authority_t;

typedef struct
{
	struct in_addr	ip; ///< IP address in static IP mode
	struct in_addr	netmask; ///< netmask in static IP mode
	struct in_addr	gateway; ///< gateway in static IP mode
	struct in_addr	dns; ///< DNS IP in static IP mode

	__u16		http_port; ///< HTTP port in web site.
	int			dhcp_enable; ///< current DHCP status.
	int			system_port; //default 5505
}NETWORK_LAN;

/**
* @brief Infomation of network status and user settings.
*/
typedef struct
{
	struct in_addr	ip; ///< IP address in static IP mode
	struct in_addr	netmask; ///< netmask in static IP mode
	struct in_addr	gateway; ///< gateway in static IP mode
	struct in_addr	dns; ///< DNS IP in static IP mode
	__u16		http_port; ///< HTTP port in web site.
	int			dhcp_enable; ///< current DHCP status.
	int			system_port; //default 5505
	__u8		MAC[MAC_LENGTH]; ///< hardware MAC address

} Network_Config_Data;

/**
* @brief Infomation about ftp configuration.
*/
typedef struct
{
	char		servier_ip[MAX_DOMAIN_NAME+1]; ///< FTP server address
	char		username[USER_LEN]; ///< FTP login username
	char		password[PASSWORD_LEN]; ///< FTP login password
	char		foldername[MAX_STRING_LENGTH]; ///< FTP upload folder
	int			image_acount; ///< Image count
	int			pid; ///< PID
	__u16		port; ///< FTP port
 	__u8		rftpenable; ///< RFTP enable
	__u8        ftpfileformat; ///< file format
} Ftp_Config_Data;

/**
* @brief Infomation about SMTP configuration.
*/
typedef struct
{
	char		servier_ip[MAX_DOMAIN_NAME+1]; ///< SMTP server address
	char		username[USER_LEN]; ///< SMTP username
	char		password[PASSWORD_LEN]; ///< SMTP password
	__u8		authentication; ///< SMTP authentication
	char		sender_email[MAX_STRING_LENGTH]; ///< sender E-mail address
	char		receiver_email[MAX_STRING_LENGTH]; ///< receiver E-mail address
	char		CC[MAX_STRING_LENGTH]; ///< CC E-mail address
	char		subject[MAX_STRING_LENGTH]; ///< mail subject
	char		text[MAX_STRING_LENGTH]; ///< mail text
	__u8		attachments; ///< mail attachment
	__u8		view; ///< smtp view
	__u8		asmtpattach; ///< attatched file numbers
	__u8        attfileformat; ///< attachment file format
} Smtp_Config_Data;

/**
* @brief custom data structure for time.
*/
typedef struct{
	__u8	nHour;	///< Hour from 0 to 23.
	__u8	nMin;	///< Minute from 0 to 59.
	__u8	nSec;	///< Second from 0 to 59.
} Time_t;

/**
* @brief custom data structure for schedule entry.
*/
typedef struct{
	__u8	bStatus;	///< schedule status ( 0:disable 1:enable }
	__u8	nDay;		///< schedule day of week (1:Mon 2:Tue 3:Wed 4:Thr 5:Fri 6:Sat 7:Sun 8:Everyday)
	Time_t	tStart;		///< schedule start time
	Time_t	tDuration;	///< schedule duration
} Schedule_t;

typedef struct _SSTW_SIZE_T
{
    unsigned int uWidth;
    unsigned int uHeight;
}SSTW_SIZE_T;


typedef enum _GV_PIC_SIZE{
	GV_PIC_QCIF = 0,
	GV_PIC_QCIF_N,
	GV_PIC_CIF,
	GV_PIC_CIF_N,
	GV_PIC_D1,
	GV_PIC_D1_N,
	GV_PIC_960H,
	GV_PIC_960H_N,
	GV_PIC_2CIF,
	GV_PIC_2CIF_N,
	GV_PIC_QVGA,		

	GV_PIC_VGA,				//11
	GV_PIC_XGA,
	GV_PIC_SXGA,
	GV_PIC_UXGA,
	GV_PIC_QXGA,
	GV_PIC_WXGA,
	GV_PIC_WVGA,
	GV_PIC_WSXGA,
	GV_PIC_WUXGA,
	GV_PIC_WQXGA,

	GV_PIC_HD720,		//21
	GV_PIC_HD1080,
	GV_PIC_HD960,	//1280*960

	GV_PIC_2304x1296, /* 3M:2304 * 1296 */
    GV_PIC_2592x1520, /* 4M:2592 * 1520 */
    GV_PIC_5M,      /* 2592 * 1944 */
	GV_PIC_UHD4K,   /* 3840 * 2160 */

	GV_PIC_800x600,			
	GV_PIC_1600x900,
	GV_PIC_1440x900,
	GV_PIC_1366x768,
	GV_PIC_1360x768,
	GV_PIC_1280x1024,
	GV_PIC_1280x800,
	GV_PIC_1280x768,
}GV_PIC_SIZE;

/**
* @brief  schedule data.
*/
typedef enum {
	 RS_CONTINUE = 0,
	 RS_MOTION,
	 RS_NO,
}RECORD_STATE;

typedef enum{
	STREAM_NO = -1,
	STREAM_MAIN,
	STREAM_SUB,
	STREAM_SUB2,
	STREAM_REC,
	STREAM_AUD,	//audio
}STREAM_TYPE;


typedef enum _DVR_TYPE_ID_
{
	ST_DVR_CAMERA_TYPE = 0,
	ST_IP_CAMERA_TYPE,          // ip camera
	ST_DVR_SERVER_TYPE,         // dvr server
	ST_IP_CAMERA_GROUP_TYPE,    // ip camera group type
	ST_DVR_REMOTE_TYPE,
	ST_DM_SERVER_TYPE,	
	ST_RT_SERVER_TYPE,         // rt server
	NVR_SERVER_TYPE,        // NVR server

	GV_804,
	GV_DVR_CARD_END = 15,
	GV_DVR_CARD_HYBRID = 16,		
	GV_DVR_CARD_HYBRID_V1,
	GV_DVR_CARD_HYBRID_END = 30,		
	GV_DVR = 31,
	GV_D804,
	GV_DVR_END = 60,
	GV_IPC = 61,
	GV_IPC_GM1,
	GV_IPC_HH,
	DVR_CAIJI_KA = 68,

	//ipcamera........
	GV_IPCAM_TYPE = 80,
	GV_IPCAM_OV9712,		// 5 series
	GV_IPCAM_AR0130,		// 6 series
	GV_IPCAM_AR0331,		// 200M camera	
	GV_IPCAM_HI3518_OV9712,	//HI3518
	GV_IPCAM_HI3518_AR0130,	//HI3518
	GV_IPCAM_HI3516_AR0330,	//HI3518
	GV_IPCAM_HI3518_OV9712P,	//
	GV_IPCAM_HI3516_SONY122,	//
	GV_IPCAM_HI3518E_OV9712,	//

	GV_IPCAM_HI3518E_AR0130,	//	90
	GV_IPCAM_HI3518E_MIS1011,	//
	GV_IPCAM_HI3518E_H22,	//
	GV_IPCAM_HI3518E_OV3660,	//
	GV_IPCAM_HI3518E_IMX225,

	IPCAM_DM368_DOME = 100,	//dm368 dome
	
	GV_IPCAM_HI3518EV200_IMX323, 
	GV_IPCAM_HI3518EV200_AR0130, 
	GV_IPCAM_HI3518EV200_SC2035,

	//16A.....
	GV_IPCAM_HI3516D_OV4689 = 120,
	GV_IPCAM_HI3516A_BT1120_1080P,	
	GV_IPCAM_HI3516A_OV4689,

	//GV_IPCAM_HI3516A_BT1120_720P,

	GV_IPC_JU_LONG = 140,	//Ju Long  module

	GV_IPC_END = 160,		
		
	GV_RTS = 161,
	GV_RTS_END = 170,
		
	GV_VIDEO_DEC = 171,
	GV_VIDEO_DEC_END = 179,
		
	GV_NVR = 180,
	GV_NVR_XX,
	GV_NVRMODEL_4,
	GV_NVRMODEL_9,
	GV_NVRMODEL_16,
	GV_NVRMODEL_20,
	GV_NVRMODEL_25,
	GV_NVRMODEL_36,
	GV_NVR_END = 279,
		
}DVR_TYPE_ID;

/*
typedef enum
{
	tvp5150,
	ip2986,
	ov9712,
	mt9m034,
	tw99x0,
	ar0331,
	chassis_camera,
	imx122,
}AD_TYPE;
*/

typedef struct {
	//RECORD_STATE   mode[7][24];
	v_u8 mode[7][24];
	//v_u8    state;
	int	stream_id;
	v_u32  motion_length;
}RECORD;

#define BUFFERSIZE 256
#define LARGE_BUFFER_SIZE	4096

typedef enum {
	CAMERAINFO = 0,
	CAMERAOPER,
	FINDFILE,
	PLAYFILE,
	ISACTIVE,
	FINDFILE_8_1,
	COERCE_DISCONNECT
}CAMERA;

typedef enum {
	LOGIN = 0,
		
	//CAMERA_CTL_BEGIN = 40,
	COLORPREVIEW,		
	COLORSAVE,
	COLORRESTORE,
	VIDEO1CONFIG,
	VIDEO2CONFIG,		
	RECORDCONFIG,
	SET_COLOR_BW,
	AV_NOISE_LEVEL,
	AV_SET_HDR,
	AV_SET_COMPENSATION,		//10
	AV_SET_MIRROR,
	AV_SET_FLIP,
	SETTCP,
	VIDEO3CONFIG,
	AV_SET_WHITE_LIGHT,
	SET_OSD_INFO,
	AV_HAVE_SIGNAL,
	AV_SET_OSD2,				//osd print
	AV_SET_SHARPEN,
	AV_SET_EXPOSURE_TIME,

	CAMERA_CTL_BEGIN = 60,
	ADDUSER = 61,		// 61	
	DELUSER,				
	EVENTSTART,				
	DHCP,	
	INTERNETIP,		// 65
	SUBNETMASK,			
	DNSIP,					
	GATEWAY,			
	HTTPPORT,

	PPPOE,		// 70	
	GET_DEVICE_INFO,		
	NEWDATE,				
	NEWTIME,			
	SETDAYLIGHT,
	TIMEZONE,	
	GETSYSTEMINFO,	// 75+1
	TIMEFORMAT,
	COLORKILLER,
	AWB,

	ENABLE_WRITE_CFG,			//80
	GETTITLE,				
	CAMERATITLE,	
	WRITELOG,
	CLEANLOG,
	RESETDEFAULT,	
	SETSCHEDULE,	
	UPDATEFW,		
	SETDDNS,
	DELDDNS,

	NEWPWD,				//90
	TCPPORT,
        MD_SET,          
	MD_GET,
        SET_IP2986,
        GET_IP2986,
	RESET_ADMIN,
	SET_COLOR,
	GET_COLOR,
	GET_NETWORK_LAN,   

	SET_INDOOR_OUTDOOR,		//100
        GET_INDOOR_OUTDOOR,
	UPGRADE,
	GET_FIRMWARE_VERSION,
	SET_NOISE_LEVEL,
	GET_NOISE_LEVEL,
	SET_NETWORK_LAN,
	SET_VIDEO_PARAM,
	GET_VIDEO_PARAM,
	SET_COLOR_SWITCH,

	GET_COLOR_SWITCH,		//110
	SET_PTZ_OPERA,
	GET_PTZ_PRESETS,
	SET_BRIGHTNESS,
	GET_BRIGHTNESS,
	SET_IDR,
	SET_MAC,
	SET_HDR,
	GET_HDR,
	SET_MIRROR,

	GET_MIRROR,			//120
	SET_FLIP,
	GET_FLIP,
	SET_ELIU_SERVER,
	GET_ELIU_SERVER,
	SET_ONVIF,
	GET_ONVIF,
	GET_SDCARD_INFO,
	SET_AUDIO_ST,	
	GET_AUDIO_ST,
	SET_REBOOT,		//130
				
	SET_WHITE_LIGHT, //liuyang for set_white_light
	GET_WHITE_LIGHT, //liuyang for get_white_light
	SET_MAIN_STREAM_RES, //liuyang for set_main_stream_res
	GET_MAIN_STREAM_RES, //liuyang for get_main_stream_re
	SET_MANUFACTURE,
	SET_PURE_SYSTEM,
	SET_ENABLE_OSD,
	GET_ENABLE_OSD,
	SET_VIDEO_NORM,
	GET_VIDEO_NORM,		//140

	SET_DATA_TIME_OFFSET,
	SET_VIDEO_IN_MODE,
	GET_VIDEO_IN_MODE,
	SET_IRCUT_STATUS,
	GET_IRCUT_STATUS,
	SET_IRCUT_DIRECTION,
	GET_IRCUT_DIRECTION,
	SET_MD_STATUS,
	SET_ALARM_MODE,
	GET_ALARM_MODE,			//150

	UPGRADE2,
	SET_WHITE_LIGHT_SCHEDULE,
	GET_WHITE_LIGHT_SCHEDULE,
	SET_DANALE_CONFIG,
	SET_SUB_STREAM_RES,
	SET_AUTO_REBOOT,
	SET_STREAM_CODEC,
	GET_STREAM_CODEC,
	SET_ENCODE_RC_MODE,
	GET_ENCODE_RC_MODE,		//160

	SET_OSD_PARAM,
	GET_OSD_PARAM,
	SET_BT1120_INPUT,
	GET_BT1120_INPUT,
	SET_OSD2,	
	GET_SHARPEN,
	SET_SHARPEN,
	GET_EXPOSURE_TIME,
	SET_EXPOSURE_TIME,
	GET_EXTCH_RES,			//170

	SET_EXTCH_RES,
	GET_VO_RES,
	SET_VO_RES,
	GET_BACKLIGHT_COM,
	SET_BACKLIGHT_COM,

	SET_PIC_EXPO_TIME_MENU,    //176
	GET_PIC_EXPO_TIME_MENU,
	SET_PIC_A_GAIN,
	GET_PIC_A_GAIN,
	SET_PIC_D_GAIN,
	GET_PIC_D_GAIN,
	SET_PIC_ISP_GAIN,
	GET_PIC_ISP_GAIN,
	SET_PIC_EXPO_TYPE,
	GET_PIC_EXPO_TYPE,    //185
	
	SET_PIC_3D_NF_LEVEL,    //steven 2017-01-16
	GET_PIC_3D_NF_LEVEL,
	
	SET_PIC_AE_AUTO_SPEED, //steven 2017-01-19   188
	GET_PIC_AE_AUTO_SPEED,  //189
	SET_PIC_AE_AUTO_BLACKSPEEDBIAS, //190
	GET_PIC_AE_AUTO_BLACKSPEEDBIAS, //191
	SET_PIC_AE_AUTO_TOLERANCE, //192
	GET_PIC_AE_AUTO_TOLERANCE, //193
	SET_PIC_AE_AUTO_EVBIAS, //194
	GET_PIC_AE_AUTO_EVBIAS, //195
	GET_PIC_GRAY,	
	
	
	
        //please add to here.......,

	//ACK
	GET_NETWORK_LAN_ACK = 180,
	LOGIN_ACK,
	GET_ELIU_SERVER_ACK,
	GET_AUDIO_ST_ACK,

	HEART_BEAT = 250,
	CAMERA_CTL_TOTAL
}CAMERA_CTL;


#pragma pack(1)

#ifndef _WIN32

struct NETDATA{
	v_u8 type;
	v_u8 operate;
	v_u16 id;
	v_u16 len;
	char data[];
} __attribute__((packed));
#else
struct NETDATA{
	v_u8 type;
	v_u8 operate;
	v_u16 id;
	v_u16 len;
	char data[];
};

#endif

#pragma pack() 

typedef enum {
	//
	UP = 0,
    DOWN,
	LEFT,
	RIGHT,	
	FOCUSFAR,
	FOCUSNEAR,
	ZOOMWIDE,
	ZOOMTELE,
	IRISCLOSE,
	IRISOPEN,
	STOP,               //10
	GET,
	SET,
	ZONESTART,
	ZONEEND,
	WIPERON,
	WIPEROFF,
	LIGHTON,
	LIGHTOFF,
   	SETPRESET,
	CLEARPRESET,       //20
	GOPRESET,
	EXSTO,
	PTZ_CRUISE,
	PTZ_CRUISE_STOP,
	PTZ_PAN_SPEED,
	PTZ_TILT_SPEED,
	PTZ_CRUISE_INTVAL,
	PTZ_SPEED_UP,
	PTZ_SPEED_DOWN,
	PTZ_ID,				//30

	PTZ_OPEN_OSD,		//new	
	PTZ_LIGHT_SET,

	PTZ_SET_PRESET_VALUE,
	PTZ_CLEAR_PRESET_VALUE,
	PTZ_GO_PRESET_VALUE,
	
} NETOPERATE;


#define IVS_ALARM_OUT_NOTHING				0
#define	IVS_ALARM_OUT_WHITELIGHT_ALWAYS		4
#define IVS_ALARM_OUT_WHITELIGHT			5
#define IVS_ALARM_OUT_ALARM					6

#define  PRESET_MAX 16
#define  PRESET_NAME_LEN	16
typedef struct _PTZ_CMD
{
	int cmd;
	char param[PRESET_NAME_LEN];
}PTZ_CMD;

/**
* @brief IPCAM configuration data.
*/
typedef struct
{
	char			title[MAX_LANCAM_TITLE_LEN+1];	///< camera title
	Network_Config_Data	net;						///< network status and user settings
} Lancam_Config_Data;

/**
* @brief SD card configuration data.
*/
typedef struct
{
/**  __u8			sdrecordtype;
  __u8			sdcount;
  __u8			sdrate;
  __u8			sdduration;
  __u8			aviprealarm;*/
   //__u8         sdformat;
   __u8			sdfileformat;	///< file format saved into SD card
   __u8			sdrenable;		///< enable SD card recording
   __u8			sdinsert;		///< SD card inserted
}Sdcard_Config_Data;

/**
* @brief IPCAM user account data.
*/
typedef struct
{
	char	user[USER_LEN];			///< username
	char	password[PASSWORD_LEN];	///< password
	__u8	authority;				///< user authority
}Acount_t;

/**
* @brief motion detection configuration data.
*/
typedef struct
{
  __u8      motionenable;				///< motion detection enable
  __u8      motioncenable;				///< customized sensitivity enable
  __u8      motionlevel;				///< predefined sensitivity level
  __u8      motioncvalue;				///< customized sensitivity value
  __u8	 motionblock[MOTION_BLK_LEN];	///< motion detection block data
}Motion_Config_Data;


/**
* @brief structure for PTZ control data.
*/
typedef struct
{
  char      ptzdata[300];
}Ptz_Data;


/**
* @brief PTZ configuration data.
*/
typedef struct
{
  char      ptzzoomin[6];	///< zoom-in
  char      ptzzoomout[7];	///< zoom-out
  char      ptzpanup[2];	///< pan-up
  char      ptzpandown[4];	///< pan-down
  char	ptzpanleft[4];		///< pan-left
  char	ptzpanright[5];		///< pan-right
}Ptz_Config_Data;

/**
* @brief event log data structure.
*/
//typedef struct LogEntry_t{
typedef struct {
#ifdef _WIN32
	char event[60];		///< event description
#else
	char event[50];		///< event description
#endif
	struct tm time;		///< event log time
}LogEntry_t;

typedef struct LogData_t{
	LogEntry_t tLogData;
	struct LogData_t* pNext;
}LogData_t;


/*typedef struct LogData_t{
	LogEntry_t tLogData;
	struct LogData_t* pNext;
}LogData_t;

static LogData_t* gLogHead = NULL;
*/
//video     


typedef struct {
    v_u8     brightness;
    v_u8     contrast;
    v_u8     saturation;
    v_u8     hue;
    v_u8     sharpness;
}COLOR_CONFG;

typedef struct _RESOLUTION
{
	v_u32	width;
	v_u32	height;
}resolution_t;

static const int brightness_level[5][2] = {{0x40,0x40},{0x55,0x4d},{0x65, 0x5d}, {0x75, 0x6d}, {0x8a,0x7d}};

// 								

struct CONFGAV{    
    v_u32     width;
    v_u32     height;
	v_u16	  codec_id;	
	v_u16	  reserved;	
	v_u32	  reserved2;
    v_u32     fps; 
    float     Iframe_sec;		//dummy
    v_u32     encode_rc_mode;
    v_u32     bitrate;        //k bit/s
    v_u32     pic_level;
    v_u32      bMainStream;
    COLOR_CONFG dummy;
    v_u32    res_sel;
};


typedef struct _DYNAMICIP{
	unsigned long	operate;
	unsigned long	len;
	char			name[12];//use 8 char
	unsigned long	ip;
//	char			password[12];
}DYNAMICIP, *PDYNAMICIP;

typedef enum {
	PUTIP = 1111,
		GETIP,
		GETTENIP,
		UNGETTENIP,
		APPLYID,
		IDVALID,
		IDUNVALID,
		REGISTERNAME,
		DELETENAME,
		REGISTERNAME_2,
		ID_EXPIRE,
}DYIP;

typedef struct _MOTION_PARAM
{
	float left;
	float top;
	float right;
	float bottom;
}MOTION_PARAM;

typedef struct
{
	v_u8 bAlarm_in;
	v_u8 bRecord;
	v_u8 bAlarm_out;
	v_u32 alarm_out_len;
	
}Alarm_t;

typedef struct 
{
	v_u8	mode_auto;
	v_u8	b_indoor;
}INDOOR_OUTDOOR;

typedef struct _sd_card_info
{
	int total_space;
	int free_space;
}sd_card_info_t;

#define ELIU_CFG "/mnt/mtd/etc/eliu.cfg"	

typedef struct _eliu_server_user
{
	char server[64];		//address of server
	char dev_id[64];			//dev id
}eliu_server_user_t;

typedef struct _eliu_ipc_info
{
	eliu_server_user_t eliu_server;
	unsigned int xx;//dev_id;

}eliu_ipc_info_t;


#ifdef COMP_GV
#define SERVER_DDNS "59.124.124.118"
#else
#define SERVER_DDNS  "121.37.59.252"
#endif

#define PORT_DDNS	10011

typedef struct _IRCUTINFO_
{
	v_u8	b_mode_manual;	
	v_u8	b_write_black;
}IRCUTINFO;

typedef struct _white_light_schedule{
	unsigned short	begin;	// 24 hour
	unsigned short	end;	// 24 hour
}white_light_schedule;

typedef struct _osd_param{
	v_u8  disable;
	v_u8  size;				//font size
	v_u8  vertical;		
	v_u8  color_inverse;
	v_u16 color;		//PIXEL_FORMAT_RGB_1555
	v_u16 lines :6;
	v_u16 reserved	:10;
	int start_x;
	int start_y;
}osd_param;

/**
* @brief system info main data structure.
*/
typedef struct SysInfo{
	Lancam_Config_Data lan_config;		///< IPCAM configuration data
	unsigned short DeviceType;			///< IPCAM device type
	char devicename[MAX_STRING_LENGTH];	///< device name
	Acount_t	acounts[ACOUNT_NUM];	///< user account data
// 	LogEntry_t	tCurLog;				///< event log
	struct CONFGAV     avConfg[4];
	RECORD record_config;			// schedule data
	char ddns[12];					// ddns 
	COLOR_CONFG viewcolor;
	MOTION_PARAM  ma[4];
	INDOOR_OUTDOOR iodoor;
	int noise_level;
	int is_color_switch;
	int exposure_compensation;     //  -2 -> 2
	int hdr_level;
	int is_mirror;
	int is_flip;
	int is_onvif;
	sd_card_info_t sd_card;
	int en_audio;
	int dis_osd;
	int is_white_light;
	int danale_cloud_alarm;
	DVR_TYPE_ID sensor_type;
	int video_norm;			//pal or ntsc , power_freq 50hz or 60hz
	int onvif_month_offset;	//8:8:8:8, mon_off, day_off, hour_off, no use
	int onvif_hour_offset;
	int video_in_mode;
	IRCUTINFO	ir_cut;
	int ircut_dir_opposite;
	int alarm_out_mode;
	white_light_schedule	wl_schedule;
	int n_auto_reboot;		//do not auto reboot;
	osd_param osd_info[3];
	int bt1120_input_mode;
	int sharpen;
	int exposure_time;		//us
	SSTW_SIZE_T size_extch;
	SSTW_SIZE_T size_vo;
	int backlight_com;
	int exposure_type;
	unsigned int exposure_time_manu;		//us
	unsigned int a_gain;
	unsigned int d_gain;
	unsigned int isp_gain;
	int nf_3d_level;  //steven 2017-01-16 3D NF Level
	
	int defog_flag;
	int dis_flag;
	int dci_flag;
	
	int m_vi_rotate;
	int m_vo_rotate;
	
	unsigned char  ae_auto_u8Speed;    //steven 2017-01-19
	unsigned short u16BlackSpeedBias;
	unsigned char  u8Tolerance;
	unsigned short  u16EVBias;
	
	char dummy[136];
}SysInfo;

typedef enum{
	VI_MODE_20FPS,
	VI_MODE_NORMAL,	
	VI_MODE_15FPS,
	VI_MODE_10FPS,
	VI_MODE_5FPS
};


enum _input_mode
{
	INPUT_DVI,
	INPUT_VGA
};

typedef struct  _CTLENV{	
	int sock;
#ifndef _WIN32
	pthread_mutex_t   *prime;
#else
	int   *prime;
#endif
	
}CTLENV, *LCTLENV;

typedef struct {
	long msg_to;
	v_u8 type;
	v_u8 operate;
	v_u16 id;
	v_u16 len;
	char data[BUFFERSIZE];
}SYS_MSG;

typedef struct {
	long msg_to;
	v_u8 type;
	v_u8 operate;
	v_u16 id;
	v_u16 len;
	char data[LARGE_BUFFER_SIZE];
}SYS_MSG_LARGE;

#define Debug_Information_Flag  0  //by tianfb

#define CMD_RESET_ADMIN_PWD "000000000000"
#endif


