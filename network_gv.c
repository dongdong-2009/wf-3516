// http.c : Defines the entry point for the console application.

#include "stdafx.h"
#include <netdb.h>
#include "network_gv.h"
#include "util.h"
#include "video_config.h"
#include "pkDetectCV.h"

#ifdef GV_DEBUG
#undef  GV_DEBUG
#endif

#define GV_DEBUG

#ifdef GV_DEBUG
#define dpf(fmt, args...)	printf("[network_gv.c]--%d: " fmt, __LINE__, ##args)
#else
#define dpf(x...)	
#endif

CtlNetEnv ctlEnv;
extern SysInfo *g_pSysInfo;  // share memory with cgi

#define sizeof_arr(_x)  (sizeof(_x)/sizeof(_x[0]) )

#define SK_CTL_NUM	10

static char *nicname = "eth0";

static int g_client_pool[SK_CTL_NUM];

int get_sock_ctl( int sock )
{
	int index = -1;
	int i;
	static int oldest = 0;

	for( i = 0; i< SK_CTL_NUM; i++ ){
		if( 0 == g_client_pool[i] ){
			index = i;
			g_client_pool[i] = sock;
			break;
		}
	}	

	if( index < 0 ){
		sstw_error("[sys]: close the oldest socket: %d", g_client_pool[oldest] );
		
		close(g_client_pool[oldest]);	
		index = oldest;	
		g_client_pool[index] = sock;
		oldest++;
		if( oldest >= SK_CTL_NUM )
			oldest = 0;

	}
	
	return index;
}

int close_sock_ctl( int sock )
{
	int index = -1;
	int i;

	for( i = 0; i< SK_CTL_NUM; i++ ){
		if( sock == g_client_pool[i] ){
			close( sock );
			g_client_pool[i] = 0;
			index = i;
			break;
		}
	}

	return index;

}



//netdata->data:   user:pw
int login( char* buf, AUTHORITY* au )
{
	printf("verif password\n");
	int i;	
	struct NETDATA *nd = (struct NETDATA*)buf;
	//SYS_MSG *nd = (SYS_MSG*)arg;

	printf("nd->type: %d\n", nd->type);
	printf("nd->data: %s\n", nd->data);

	char user[USER_LEN]; 
	char pw[PASSWORD_LEN];

	memset( user, 0, sizeof(user) );
	memset( pw, 0, sizeof(pw) );

	//	sscanf( nd->data, "%[^:]%s", user, pw );
	//	memcpy( pw, pw+1, sizeof(pw)-1 );

	memcpy( user, nd->data, 12 );
	memcpy( pw, nd->data+12, 12 );

	printf( "user: %s pw: %s\n", user, pw );

	//SysInfo* g_pSysInfo = GetSysInfo();
	if(g_pSysInfo == NULL)
		return -1;

	bool bget = 0;

	for(i = 0; i< ACOUNT_NUM;i++){
		printf("[appro_api.c]-->name: %s, pw:%s\n", g_pSysInfo->acounts[i].user, 
				g_pSysInfo->acounts[i].password );
		if ( strcmp( user, g_pSysInfo->acounts[i].user ) == 0 &&
				strcmp( pw, g_pSysInfo->acounts[i].password ) == 0){
			*au = g_pSysInfo->acounts[i].authority;		
			bget = 1;
			break;
		}
	}	

	if( bget == 1 )
	{
		nd->data[0] = 1;
		nd->data[1] = 1; //accord with dvr
		nd->data[2] = 1;	//normal

		char d_info[1024] = {0};
		int d_len = device_info( d_info );

		sprintf( &(nd->data[3]), "%s:%s,%s", g_pSysInfo->lan_config.title, DEVICE_ID, d_info );
		printf( "******len: %d, *dev info: %s \n", d_len, d_info );

		nd->len = sizeof(struct NETDATA) + 3 + strlen(&(nd->data[3]));

		printf( "******len: %d, *dev info: %s \n", nd->len, &(nd->data[3]) );
	}
	else
	{
		nd->data[0] = nd->data[1] = nd->data[2] = 0;
		nd->len = sizeof(struct NETDATA) + 3;
	}
	return 5;	
}


int get_device_type()
{
	return 122;
		
}

#define FORMAT_VAR "v00.00.00.00"

int get_firmware( char *sVer, int len )
{	
	FILE *fp = NULL;

	if(NULL != (fp = fopen(VER_FILE, "r")))
	{
		fgets(sVer, len, fp);
		if( sVer[0] == 0 )
		{
			sprintf( sVer, FORMAT_VAR );
		}
		fclose(fp);
		return 0;
	}
	else
	{
		sprintf( sVer, FORMAT_VAR );
		return -1;
	}
}
int get_firmware_version( void *arg )
{
	//dpf( "get_firmware_version\n" );

	SYS_MSG*nd = (SYS_MSG*)arg;

	char sVer[128] = {0};

	get_firmware(sVer, sizeof(sVer) );
	//dpf( "ver: %s\n", sVer );

	char* pver = strchr( sVer, 'v' );
	if( pver != NULL )
	{
		//pver[strlen(FORMAT_VAR)] = 0;
		memcpy( nd->data, pver, strlen(pver) );
		nd->len = strlen(pver)+sizeof(struct NETDATA);
	}
	else
	{
		nd->len = sizeof(struct NETDATA);
	}
	return 5;	

}

int set_firmware_version( char *sVer )
{	
	FILE *fp = NULL;

	if(NULL != (fp = fopen(VER_FILE, "w")))
	{
		fputs(sVer, fp);		
		fclose(fp);
		//sstw_error("write %s ", VER_FILE);

		return 0;
	}
	else
	{
		//sprintf( sVer, FORMAT_VAR );
		sstw_error("open %s failed", VER_FILE);
		return -1;
	}
}

void flash_write( char* firmware )
{
	if( firmware == NULL )
		return;

	char cmd[256]= {0};
	sprintf( cmd, "flashcp %s /dev/mtd3 -v", firmware );
	printf( "flash write cmd: %s\n", cmd );
	system( cmd );

}
int g_upgrading = false;

typedef struct _firmware_header{
	char version[32];
	int  hw_type;
	char reserved[28];
}firmware_header;

#define ERR_SPACE			0
#define ERR_SUCCESS			1
#define ERR_NETWORK			2
#define ERR_MISMATCH		3
//--------------------------
int upgrade2( void* arg )
{
	//char *pt = NULL,*ptr = NULL,*ptr_e = NULL;
	char dev_key[12] = {0};
	
	//char temp[LINE_LEN];
	//unsigned char dev = 0;
	int upgrade_status = ERR_SUCCESS;
	char* buf  = NULL;
	
	g_upgrading = true;

	printf("upgrade22........\n" );

	//delete tmp file;	
	system( "/bin/rm /tmp/*.tmp -rf" );

	struct NETDATA* pnd = arg;
	int sock = pnd->id;
	char* file_name = pnd->data;
	int len = pnd->len-sizeof(struct NETDATA);
	file_name[len] = 0;
	struct NETDATA net_buf;

	dpf( "sock: %d, data len: %d, file: %s\n", sock, pnd->len, file_name );
			
	char file[256] = {0};
#if 0
	sprintf( file, "/mnt/mtd/%s.tmp", file_name );	
#else
	sprintf( file, "/tmp/%s.tmp", file_name );	
#endif

	sprintf( dev_key, "%03d", g_pSysInfo->sensor_type );

	printf( "upgrade2 file: %s, dev: %s\n", file, dev_key );
	
	FILE* pf = fopen( file, "w" );
	if( NULL == pf )
	{
		upgrade_status = ERR_SPACE;
		goto end;
	}		
	int ret= recv( sock, (char*)&net_buf, sizeof(struct NETDATA), 0);
	if( ret <= 0 )
	{
		upgrade_status = ERR_NETWORK;
		goto end;
	}

	int file_len = (net_buf.len | (net_buf.id<<16));   //size of update file is net_buf.len + net_buf.id;
	int buf_len = 10*1500;
	buf = malloc( buf_len );

	printf( "upgrade file len = %d\n", file_len );
	
	firmware_header fh;
	memset( &fh, 0, sizeof(fh) );

	ret = recv( sock, &fh, sizeof(fh), 0 ); //header of firmware.
	if( ret <sizeof(fh) )
	{
		system("/bin/rm /tmp/*.tmp");
		perror( "recv error" );
		upgrade_status = ERR_NETWORK;
		goto end;
	}

	if( fh.hw_type != g_pSysInfo->sensor_type )
	{
		printf("Patch file is mismatched!\n");
		//	fclose(fp_match);		
		upgrade_status = ERR_MISMATCH;
		goto end;
	}

	file_len -= sizeof(fh);

	while( file_len > 0 ){ 
		if( file_len < buf_len )
			buf_len = file_len;

		ret = recv( sock, buf, buf_len, 0 ); 
		if( ret <= 0 ){
			//dpf( "send failed\n " );
			system("/bin/rm /tmp/*.tmp");
			perror( "recv error" );
			upgrade_status = ERR_NETWORK;
			goto end;
		}else{
			file_len -= ret; 
			if( fwrite( buf, ret, 1, pf ) != 1 )
			{
				system("/bin/rm /tmp/*.tmp");
				printf( "write to file failed\n");
				upgrade_status = ERR_NETWORK;
				goto end;
			}
		}
	}

	fflush(pf);

end:
	
	//send back;
	memset( &net_buf, 0, sizeof(net_buf) );

	net_buf.operate = upgrade_status;
	
	send( sock, (char*)&net_buf, sizeof(net_buf), 0 );
	
	if( NULL != buf )	free( buf );
	
	if (upgrade_status == ERR_SUCCESS)
	{
		char filename_new[256] = {0};
		memcpy( filename_new, file, strlen(file) );
		filename_new[strlen(filename_new)-4] = 0;
		dpf( "upgrade file: %s\n", filename_new );
		rename( file, filename_new );
		
		set_firmware_version( fh.version );
		flash_write(filename_new);		
		system("/sbin/reboot");
		exit(0);
	}
	else
	{
		g_upgrading = false;
	}
	return 0;
}


int get_title(void *arg)
{
	SYS_MSG *nd = (SYS_MSG*)arg;

	//SysInfo *g_pSysInfo = GetSysInfo();
	if(g_pSysInfo == NULL)
		return -1;

	memcpy( nd->data, g_pSysInfo->lan_config.title, sizeof(g_pSysInfo->lan_config.title) );
	nd->len = sizeof(g_pSysInfo->lan_config.title)+sizeof(struct NETDATA);

	return 5;

}

int device_info( char* info )
{
	if( info == NULL )
		return 0;

	int w = g_pSysInfo->avConfg[0].width;
	int h = g_pSysInfo->avConfg[0].height;

	int has_sd = 0;

	//http port
	int http_port = g_pSysInfo->lan_config.net.http_port;
	int stream_port = g_pSysInfo->lan_config.net.system_port;

	eliu_server_user_t callee_info;
	memset( &callee_info, 0, sizeof(callee_info) );

	//dev type
	DVR_TYPE_ID dev_type = get_device_type();

	//resolution
	char dev_res[128] = {0};	
	MAIN_STREAM_T ipc_main_res = {-1};

	switch(dev_type)
	{		
		case GV_IPCAM_HI3518_OV9712P:
		case GV_IPCAM_HI3518_OV9712:
		case GV_IPCAM_HI3518E_OV3660:
		case GV_IPCAM_HI3518E_OV9712:
		case GV_IPCAM_HI3518E_IMX225:
		case GV_IPCAM_HI3518E_H22:
			{
				printf("ov9712p\n");
				ipc_main_res.res_c_1 = GV_PIC_HD720;
				ipc_main_res.res_c_2 = -1;
				ipc_main_res.res_c_3 = -1;
				ipc_main_res.res_c_4 = -1;
				ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;
				break;
			}
		case GV_IPCAM_HI3518E_AR0130:
		case GV_IPCAM_HI3518E_MIS1011:
			{
				printf("ar0130+3518e\n");
				//ipc_main_res.res_c_1 = GV_PIC_HD960;
				ipc_main_res.res_c_1 = GV_PIC_HD720;
				ipc_main_res.res_c_2 = GV_PIC_HD960;
				ipc_main_res.res_c_3 = -1;
				ipc_main_res.res_c_4 = -1;
				ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;		//xxxx;//GV_PIC_HD960;//
				break;
			}
		case GV_IPCAM_HI3518_AR0130:
			{
				printf("ar0130\n");
				ipc_main_res.res_c_1 = GV_PIC_HD720;
				ipc_main_res.res_c_2 = GV_PIC_HD960;
				ipc_main_res.res_c_3 = -1;
				ipc_main_res.res_c_4 = -1;
				ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;
				break;
			}
		
		case GV_IPCAM_HI3516_AR0330:
			{
				printf("ar0330\n");
				ipc_main_res.res_c_1 = GV_PIC_HD1080;
				ipc_main_res.res_c_2 = -1;
				ipc_main_res.res_c_3 = -1;
				ipc_main_res.res_c_4 = -1;
				ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;
				break;
			}	
		case GV_IPCAM_HI3516A_BT1120_1080P:
		{
			printf("ar0330\n");
			ipc_main_res.res_c_1 = GV_PIC_HD1080;
			ipc_main_res.res_c_2 = GV_PIC_UXGA;//GV_PIC_800_600;//GV_PIC_HD720;
			ipc_main_res.res_c_3 =	GV_PIC_800x600;//GV_PIC_1360x768;//GV_PIC_UXGA;
			ipc_main_res.res_c_4 = GV_PIC_XGA;
			ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;
			break;
		}	
		case GV_IPCAM_HI3516_SONY122:
		case GV_IPCAM_HI3518EV200_IMX323:
		{
			printf("ar0330\n");
			ipc_main_res.res_c_1 = GV_PIC_HD1080;
			ipc_main_res.res_c_2 = -1;
			ipc_main_res.res_c_3 = -1;
			ipc_main_res.res_c_4 = -1;
			ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;
			break;
		}
		case GV_IPCAM_HI3516A_OV4689:
		{
			printf("3516a_ov4689\n");
			ipc_main_res.res_c_1 = GV_PIC_2592x1520;
			ipc_main_res.res_c_2 = -1;
			ipc_main_res.res_c_3 = -1;
			ipc_main_res.res_c_4 = -1;
			ipc_main_res.res_current = g_pSysInfo->avConfg[0].res_sel;
			break;
		}
			
		default :
				break;
	}
	sprintf( dev_res, "%d-%d-%d-%d-%d", ipc_main_res.res_c_1, ipc_main_res.res_c_2, ipc_main_res.res_c_3, ipc_main_res.res_c_4, ipc_main_res.res_current );
	
	char dev_sub_res[128] = {0};
	MAIN_STREAM_T ipc_sub_res = {-1};
	ipc_sub_res.res_c_1 = GV_PIC_VGA;
	ipc_sub_res.res_c_2 = GV_PIC_QVGA;
	ipc_sub_res.res_c_3 = -1;// GV_PIC_2CIF;
	ipc_sub_res.res_c_4 = -1;
	ipc_sub_res.res_current = g_pSysInfo->avConfg[1].res_sel;

	if (dev_type == GV_IPCAM_HI3516A_BT1120_1080P)
	{
		memcpy( &ipc_sub_res, &ipc_main_res, sizeof(ipc_sub_res) );
		ipc_sub_res.res_current = g_pSysInfo->avConfg[1].res_sel;
	}

	sprintf( dev_sub_res, "%d-%d-%d-%d-%d", ipc_sub_res.res_c_1, ipc_sub_res.res_c_2, ipc_sub_res.res_c_3, ipc_sub_res.res_c_4, ipc_sub_res.res_current );
		
	sprintf( info, "stream:%d,max_width:%d,max_height:%d,codec:hi_h264,ptz:0,store:%d,httpport:%d,ipc_tital:%s,dana_id:%s,dev_type:%d,"
					"dev_res:%s,stream_port:%d,osd:%d,video_norm:%d,onvif_month_offset:%d,onvif_hour_offset:%d,alarm_out_mode:%d,"
					"wl_schedule_begin:%d,wl_schedule_end:%d,dev_sub_res:%s,auto_reboot:%d,",
				   3, w, h, has_sd, http_port, g_pSysInfo->lan_config.title, callee_info.dev_id, dev_type, dev_res,stream_port,g_pSysInfo->dis_osd,
				   g_pSysInfo->video_norm, g_pSysInfo->onvif_month_offset, g_pSysInfo->onvif_hour_offset, g_pSysInfo->alarm_out_mode,
				   g_pSysInfo->wl_schedule.begin, g_pSysInfo->wl_schedule.end, dev_sub_res, !g_pSysInfo->n_auto_reboot);
	
	printf( "---dev info %s\n", info );

	return strlen( info );
}

#define ONVIF_VER "v00.00.02.20"

int get_device_info( void * arg )
{
	dpf( "get_device_info\n" );

	SYS_MSG*nd = (SYS_MSG*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	char dev_info[1024] = {0};

	int len = device_info( dev_info );

	printf( "devinfo len: %d\n", len );

	memcpy( nd->data, dev_info, strlen(dev_info) );
	nd->len = strlen(dev_info)+sizeof(struct NETDATA);

	return 5;	
}

/* open a listening socket */
int socket_open_listen( int port )
{
    int server_fd, tmp;
	struct sockaddr_in my_addr;

    server_fd = socket(AF_INET,SOCK_STREAM,0);
    if (server_fd < 0) {
        perror ("socket");
        return -1;
    }

    tmp = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
	
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);

    if (bind (server_fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) < 0) {
        char bindmsg[32];
        snprintf(bindmsg, sizeof(bindmsg), "bind(port %d)", ntohs(my_addr.sin_port));
        perror (bindmsg);
        close( server_fd );
        return -1;
    }

    if (listen (server_fd, 5) < 0) {
        perror ("listen");
        close(server_fd);
        return -1;
    }
    //ff_socket_nonblock(server_fd, 1);

    return server_fd;
}

int set_video_color( void *arg )
{
	static int qid = -1;

	dpf( "set_video_color\n" );

	struct NETDATA *nd = (struct NETDATA*)arg;

	COLOR_CONFG *cc = (COLOR_CONFG *)nd->data; 
	dpf( "%d, %d, %d, %d, %d\n", cc->brightness, cc->contrast, cc->hue, cc->saturation, cc->sharpness  );

	ConfVideoColor_save((void*)cc);

	memcpy( &(g_pSysInfo->viewcolor), nd->data, sizeof(g_pSysInfo->viewcolor) );

	WriteGlobal(g_pSysInfo);

	return 1;
}

int get_video_color( void *arg )
{
	dpf( "get_video_color\n" );
		
	struct NETDATA *nd = (struct NETDATA*)arg;

	//SysInfo *g_pSysInfo = GetSysInfo();
	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->viewcolor), sizeof(g_pSysInfo->viewcolor) );
	nd->len = sizeof(g_pSysInfo->viewcolor)+sizeof(struct NETDATA);

	COLOR_CONFG *cc = (COLOR_CONFG *)nd->data;
	dpf( "%d, %d, %d, %d, %d\n", cc->brightness, cc->contrast, cc->hue, cc->saturation, cc->sharpness  );

	return 5;	
}

int set_exposure_compensation( void *arg )
{	
	dpf("set_exposure_compensation \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	int level= 0;
	memcpy( &level, nd->data, sizeof(level) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->exposure_compensation)
	{
		set_picture_exposure_compensation( level );

		g_pSysInfo->exposure_compensation = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_exposure_compensation( void *arg )
{
	dpf( "get_exposure_compensation\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	int level = g_pSysInfo->exposure_compensation;

	memcpy( nd->data, &level, sizeof(level) );
	nd->len = sizeof(level)+sizeof(struct NETDATA);

	return 5;	
}

int get_sharpen( void *arg )
{
	dpf( "get_sharpen\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	nd->operate = (v_u8)g_pSysInfo->sharpen;

	nd->len = sizeof(struct NETDATA);

	return 5;	
}

int set_sharpen( void *arg )
{	
	dpf("set_sharpen \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	int level= nd->operate;

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->sharpen)
	{
		set_picture_sharpen(level);
		g_pSysInfo->sharpen = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_exposure_time(void *arg)
{
	dpf( "get_exposure_time\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->exposure_time), sizeof(g_pSysInfo->exposure_time) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->exposure_time);
	
	return 5;	
}

int set_exposure_time_net(void *arg)
{
	dpf("set_exposure_time \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->exposure_time)
	{
		set_exposure_time(level);

		g_pSysInfo->exposure_time = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_extch_res(void *arg)
{
	dpf( "get_extch_res, wid = %d, hei = %d\n", g_pSysInfo->size_extch.uWidth, g_pSysInfo->size_extch.uHeight );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->size_extch), sizeof(g_pSysInfo->size_extch) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->size_extch);
	
	return 5;	
}

int set_extch_res(void *arg)
{
	dpf("set_extch_res \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	SSTW_SIZE_T size_extch;
	
	memcpy( &size_extch, nd->data, sizeof(size_extch) );

	dpf( "extch_wid: %d, extch_hei = %d\n", size_extch.uWidth, size_extch.uHeight );
	
	memcpy( &(g_pSysInfo->size_extch), nd->data, sizeof(g_pSysInfo->size_extch) );

	WriteGlobal(g_pSysInfo) ;	

	system("/sbin/reboot");

	return 1;
}

int get_vo_res(void *arg)
{
	dpf( "get_vo_res, wid = %d, hei = %d\n", g_pSysInfo->size_vo.uWidth, g_pSysInfo->size_vo.uHeight );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->size_vo), sizeof(g_pSysInfo->size_vo) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->size_vo);
	
	return 5;	
}

int set_vo_res(void *arg)
{
	dpf("set_vo_res \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	SSTW_SIZE_T size_extch;
	
	memcpy( &size_extch, nd->data, sizeof(size_extch) );

	dpf( "extch_wid: %d, extch_hei = %d\n", size_extch.uWidth, size_extch.uHeight );
	if (size_extch.uWidth > 720 || size_extch.uHeight > 576)
		return 1;
	
	memcpy( &(g_pSysInfo->size_vo), nd->data, sizeof(g_pSysInfo->size_vo) );

	WriteGlobal(g_pSysInfo) ;	

	return 1;
}


int get_backlight_com(void *arg)
{
	dpf( "get_backlight_com\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->backlight_com), sizeof(g_pSysInfo->backlight_com) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->backlight_com);
	
	return 5;	
}

int set_backlight_com(void *arg)
{
	dpf("set_backlight_com \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->backlight_com)
	{
		//set_exposure_time(level);
		set_backlight_compensation(level);

		g_pSysInfo->backlight_com = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}
int set_reboot( void *arg )
{	
	dpf("set_reboot \n");

	struct NETDATA *nd = ( struct NETDATA*)arg;

	int level= 0;
	memcpy( &level, nd->data, sizeof(level) );

	dpf( "enable reboot ?: %d \n", level );

	system("/sbin/reboot");

	return 1;
}

int set_cameratitle(void *arg)
{
	printf("set_cameratitle\n");
	//NETDATA *nd = (NETDATA*)arg;
	struct NETDATA *nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return -1;

	if(sizeof(g_pSysInfo->lan_config.title) < strlen(nd->data)+1)
		return -1;

	memcpy(g_pSysInfo->lan_config.title, nd->data, strlen(nd->data));
	g_pSysInfo->lan_config.title[strlen(nd->data)] = '\0';

	return WriteGlobal(g_pSysInfo);

}

int get_network_lan( void* arg )
{
	dpf( "get_network\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->lan_config.net), sizeof(NETWORK_LAN) );
	nd->len = sizeof(NETWORK_LAN)+sizeof(struct NETDATA);
	nd->type = GET_NETWORK_LAN_ACK;
	NETWORK_LAN *cc = (NETWORK_LAN *)nd->data;
	dpf( "http port: %d\n", cc->http_port);

	return 5;	
}

int set_network_lan( void* arg )
{
	printf("set_network_lan\n");
	struct NETDATA *nd = (struct NETDATA*)arg;
	
	int b_reboot = 0;

	NETWORK_LAN lan;

	memcpy( &lan, nd->data, sizeof(NETWORK_LAN) );
		
	if( (lan.ip.s_addr & lan.netmask.s_addr) != (lan.gateway.s_addr & lan.netmask.s_addr) )
	{
		sstw_error("ip and gatway is not in the same network");
		return false;
	}

	//char *strIP;
	struct in_addr sys_ip;
		
	if (net_set_ifaddr(nicname, lan.ip.s_addr) < 0)
	{
		printf("net set ifaddr failed\n");
		return -1;
	}

	//gateway
	sys_ip.s_addr = net_get_gateway();

	printf("sys_ip=%x, new=%x\n", sys_ip.s_addr, lan.gateway.s_addr);
	if (sys_ip.s_addr != lan.gateway.s_addr)
	{
		if (net_set_gateway(lan.gateway.s_addr) < 0)
		{
			printf("net set gateway failed\n");
			// 			return -1;		
		}
		b_reboot = 1;
	}

	
	//dns
	sys_ip.s_addr = net_get_dns();

	if (sys_ip.s_addr != lan.dns.s_addr) 
	{
		if (net_set_dns(inet_ntoa(lan.dns)) < 0)
		{
			printf("net_set_dns failed\n");
			// 			return -1;
		}
	}

	//netmask
	sys_ip.s_addr = net_get_netmask(nicname);

	if ( sys_ip.s_addr != lan.netmask.s_addr)
	{
		if ( net_set_netmask(nicname, lan.netmask.s_addr) < 0 )
		{
			printf("net_set_netmask fail\n");
			return -1;
		}
		b_reboot = 1;
	}

	//http port
	if( lan.http_port == 0 )
	{
		lan.http_port = g_pSysInfo->lan_config.net.http_port;
	}
	else if( lan.http_port != g_pSysInfo->lan_config.net.http_port )
	{
		b_reboot = 1;
	}

	//system port
	if( lan.system_port == 0 )
	{
		lan.system_port = g_pSysInfo->lan_config.net.system_port;
	}
	else if( lan.system_port != g_pSysInfo->lan_config.net.system_port )
	{
		b_reboot = 1;
	}

	//dhcp
	if( lan.dhcp_enable != g_pSysInfo->lan_config.net.dhcp_enable )
	{
		if( lan.dhcp_enable == 1 )
		{
			lan.dhcp_enable = 1;
			b_reboot = 1;
		}
		else
		{
			lan.dhcp_enable = 0;
		}
	}
	
	sstw_error( "2 reboot=%d", b_reboot );

	memcpy(&g_pSysInfo->lan_config.net, &lan, sizeof(NETWORK_LAN));

	WriteGlobal(g_pSysInfo);
	save_network(&(g_pSysInfo->lan_config.net));

	if( b_reboot )
		system("/sbin/reboot");

	return 1;
}


int set_expo_time_manu(void *arg)
{
	dpf( "set_expo_time_manu\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->exposure_time_manu)
	{
		set_picture_expo_time_mamu(level);

		g_pSysInfo->exposure_time_manu = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_expo_time_manu(void *arg)
{
	dpf( "get_expo_time_manu\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->exposure_time_manu), sizeof(g_pSysInfo->exposure_time_manu) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->exposure_time_manu);
	
	return 5;	
}

int set_a_gain(void *arg)
{
	dpf( "set_a_gain\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->a_gain)
	{
		set_picture_a_gain(level);

		g_pSysInfo->a_gain = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}
int get_a_gain(void *arg)
{
	dpf( "get_a_gain\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->a_gain), sizeof(g_pSysInfo->a_gain) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->a_gain);
	
	return 5;	
}

int set_d_gain(void *arg)
{
	dpf( "set_d_gain\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->d_gain)
	{
		set_picture_d_gain(level);

		g_pSysInfo->d_gain = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}
int get_d_gain(void *arg)
{
	dpf( "get_d_gain\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->d_gain), sizeof(g_pSysInfo->d_gain) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->d_gain);
	
	return 5;	
}

int set_isp_gain(void *arg)
{
	dpf( "set_isp_gain\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->isp_gain)
	{
		set_picture_isp_gain(level);

		g_pSysInfo->isp_gain = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_isp_gain(void *arg)
{
	dpf( "get_isp_gain\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->isp_gain), sizeof(g_pSysInfo->isp_gain) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->isp_gain);
	
	return 5;	
}

int set_expo_type(void *arg)
{
	dpf( "set_expo_type\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->exposure_type)
	{
		set_picture_exposure_type(level);

		g_pSysInfo->exposure_type = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_expo_type(void *arg)
{
	dpf( "get_expo_type\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
		return 0;

	memcpy( nd->data, &(g_pSysInfo->exposure_type), sizeof(g_pSysInfo->exposure_type) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->exposure_type);
	
	return 5;	
}

//steven 2017-01-16 3D NF Level
int set_3D_nf_level(void *arg)
{
	dpf( "set_3D_nf_level\n" );


	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 level = 0;
	
	memcpy( &level, nd->data, sizeof(int) );

	dpf( "level: %d\n", level );
	
	if( level != g_pSysInfo->nf_3d_level)
	{
		
                set_picture_3d_nf_level(level);
		g_pSysInfo->nf_3d_level = level;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

//steven 2017-01-19
int set_ae_auto_speed(void *arg)
{
	dpf( "set_ae_auto_speed\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 value = 0;
	
	memcpy( &value, nd->data, sizeof(int) );

	dpf( "value: %d\n", value );
	
	if( value != g_pSysInfo->ae_auto_u8Speed)
	{
		
                set_picture_ae_auto_speed(value);
		g_pSysInfo->ae_auto_u8Speed = value;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int set_ae_auto_BlackSpeedBias(void *arg)
{
	dpf( "set_ae_auto_BlackSpeedBias\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 value = 0;
	
	memcpy( &value, nd->data, sizeof(int) );

	dpf( "value: %d\n", value );
	
	if( value != g_pSysInfo->u16BlackSpeedBias)
	{		
                set_picture_ae_auto_BlackSpeedBias(value);
		g_pSysInfo->u16BlackSpeedBias = value;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int set_ae_auto_Tolerance(void *arg)
{
	dpf( "set_ae_auto_Tolerance\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 value = 0;
	
	memcpy( &value, nd->data, sizeof(int) );

	dpf( "value: %d\n", value );
	
	if( value != g_pSysInfo->u8Tolerance)
	{		
                set_picture_ae_auto_Tolerance(value);
		g_pSysInfo->u8Tolerance = value;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int set_ae_auto_EVBias(void *arg)
{
	dpf( "set_ae_auto_EVBias\n" );

	struct NETDATA *nd = ( struct NETDATA*)arg;

	v_u32 value = 0;
	
	memcpy( &value, nd->data, sizeof(int) );

	dpf( "value: %d\n", value );
	
	if( value != g_pSysInfo->u16EVBias)
	{		
                set_picture_ae_auto_EVBias(value);
		g_pSysInfo->u16EVBias = value;
		WriteGlobal(g_pSysInfo) ;			
	}
	return 1;
}

int get_3D_nf_level(void *arg)
{
	dpf( "get_3D_nf_level\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
	    return 0;

	memcpy( nd->data, &(g_pSysInfo->nf_3d_level), sizeof(g_pSysInfo->nf_3d_level) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->nf_3d_level);
	
	return 5;	
}

//steven 2017-01-19
int get_ae_auto_speed(void *arg)
{
	dpf( "get_ae_auto_speed\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
	    return 0;

	memcpy( nd->data, &(g_pSysInfo->ae_auto_u8Speed), sizeof(g_pSysInfo->ae_auto_u8Speed) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->ae_auto_u8Speed);
	
	return 5;	
}

int get_ae_auto_BlackSpeedBias(void *arg)
{
	dpf( "get_ae_auto_BlackSpeedBias\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
	    return 0;

	memcpy( nd->data, &(g_pSysInfo->u16BlackSpeedBias), sizeof(g_pSysInfo->u16BlackSpeedBias) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->u16BlackSpeedBias);
	
	return 5;	
}

int get_ae_auto_Tolerance(void *arg)
{
	dpf( "get_ae_auto_Tolerance\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
	    return 0;

	memcpy( nd->data, &(g_pSysInfo->u8Tolerance), sizeof(g_pSysInfo->u8Tolerance) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->u8Tolerance);
	
	return 5;	
}

int get_ae_auto_EVBias(void *arg)
{
	dpf( "get_ae_auto_EVBias\n" );

	struct NETDATA*nd = (struct NETDATA*)arg;

	if(g_pSysInfo == NULL)
	    return 0;

	memcpy( nd->data, &(g_pSysInfo->u16EVBias), sizeof(g_pSysInfo->u16EVBias) );
			
	nd->len = sizeof(struct NETDATA)+sizeof(g_pSysInfo->u16EVBias);
	
	return 5;	
}
void * thread_ctrl( void* arg )
{
	int   sock = *(int *) arg;
	AUTHORITY  au = AUTHORITY_ADMIN;
	int ver, ret, sent;
	char buf[4096];
	struct NETDATA * nd = (struct NETDATA *)buf;
	v_u32 get_beat = 1;
	v_u32 init_beat = 0;
	
	struct timeval timeout={30,0};//3

	ret=setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
	g_sock = &sock;	
	while(1){
		memset(buf, 0, sizeof(buf) );
		ret = recv( sock, buf, sizeof(struct NETDATA), 0);
		if( ret == 0 )
		{
			sstw_error(" [sys] recv == 0, sock = %d err = %d", sock, errno );

			goto end;
		}
		if( ret == -1 )
		{
// 			sstw_error(" [sys] recv == -1, sock = %d err = %d", sock, errno );

			if( errno == EAGAIN || errno == ENOENT )  //errno == 11 or 2
			{
// 				dpf( "system server time out, get_beat=%d, init_beat=%d\n", get_beat, init_beat );
				if( get_beat == 0 && init_beat == 1 )
				{
					sstw_error(" [sys] did't get heart beat, close it: %d", sock );
					goto end;
				}
				else
				{
					get_beat = 0;
					sleep(1);
					continue;
				}
			}
			else
			{
				sstw_error(" [sys] socket is closed, sock = %d, errno = %d", sock, errno );
				goto end;
			}			
		}

		dpf("cammand: %d\n", nd->type);

		if( nd->len > sizeof(struct NETDATA) )
		{
			ret = recv( sock, nd->data, nd->len-sizeof(struct NETDATA), 0);
			if( ret <= 0 )
				goto end;
			
// 			dpf( "get other data:%d-- %s\n", nd->len-sizeof(struct NETDATA), nd->data );
		}
		
		//printf("msgbuf received\n");
		if ( nd->type == LOGIN ){		
			dpf("login\n" );
			//char verify = 0;
			ver = login( (char*)nd, &au );
			send( sock, nd, nd->len, 0 );	
		}else if( nd->type == UPGRADE2){
			dpf( "sock :%d\n", sock );
			nd->id = sock;
			if(!upgrade2( (void*)nd ))
				goto end;
		}else if( nd->type == HEART_BEAT){
			get_beat = 1;		
			init_beat = 1;
			dpf("get get_beat....sock = %d\n",sock );
		}
		else if( nd->type == GET_FIRMWARE_VERSION){
			ver = get_firmware_version((void*)nd);
		}else if( nd->type == SET_COLOR){
			ver = set_video_color((void*)nd);
		}else if( nd->type == GET_COLOR){
			ver = get_video_color((void*)nd);
		//}else if( nd->type == SET_SUB_STREAM_RES){

		}else if( nd->type == SET_BRIGHTNESS){
			set_exposure_compensation((void*)nd);
		}else if( nd->type == GET_BRIGHTNESS){
			get_exposure_compensation((void*)nd);
		
		}else if( nd->type == GET_SHARPEN){
			get_sharpen( (void*)nd );
		
		}else if( nd->type == SET_SHARPEN){		
			set_sharpen( (void*)nd );

		}else if( nd->type == GET_EXPOSURE_TIME){
			get_exposure_time((void*)nd);		
		}else if( nd->type == SET_EXPOSURE_TIME){
			set_exposure_time_net((void*)nd);
		}else if( nd->type == CAMERATITLE){
			set_cameratitle((void*)nd);		
		}else if( nd->type == SET_REBOOT){
			set_reboot((void*)nd);
		}else if( nd->type == GET_EXTCH_RES){
			get_extch_res((void*)nd);
		}else if( nd->type == SET_EXTCH_RES){
			set_extch_res((void*)nd);
		}else if( nd->type == GET_VO_RES){
			get_vo_res((void*)nd);
		}else if( nd->type == SET_VO_RES){
			set_vo_res((void*)nd);
		}else if( nd->type == GET_BACKLIGHT_COM){
			get_backlight_com((void*)nd);
		}else if( nd->type == SET_BACKLIGHT_COM){
			set_backlight_com((void*)nd);
		}
		
		else if( nd->type == SET_PIC_EXPO_TIME_MENU){
			set_expo_time_manu((void*)nd);
		}else if( nd->type == GET_PIC_EXPO_TIME_MENU){
			get_expo_time_manu((void*)nd);
		}else if( nd->type == SET_PIC_A_GAIN){
			set_a_gain((void*)nd);
		}else if( nd->type == GET_PIC_A_GAIN){
			get_a_gain((void*)nd);
		}else if( nd->type == SET_PIC_D_GAIN){
			set_d_gain((void*)nd);
		}else if( nd->type == GET_PIC_D_GAIN){
			get_d_gain((void*)nd);
		}else if( nd->type == SET_PIC_ISP_GAIN){
			set_isp_gain((void*)nd);
		}else if( nd->type == GET_PIC_ISP_GAIN){
			get_isp_gain((void*)nd);
		}else if( nd->type == SET_PIC_EXPO_TYPE){
			set_expo_type((void*)nd);
		}else if( nd->type == GET_PIC_EXPO_TYPE){
			get_expo_type((void*)nd);
		}else if( nd->type == SET_PIC_3D_NF_LEVEL){  //steven 2017-01-16 3D NF Level
			set_3D_nf_level((void*)nd);
		}else if( nd->type == GET_PIC_3D_NF_LEVEL){  //steven 2017-01-16 3D NF Level
			get_3D_nf_level((void*)nd);
		}else if( nd->type == SET_PIC_AE_AUTO_SPEED){  ////steven 2017-01-19
			set_ae_auto_speed((void*)nd);
		}else if( nd->type == GET_PIC_AE_AUTO_SPEED){  
			get_ae_auto_speed((void*)nd);
		}else if( nd->type == SET_PIC_AE_AUTO_BLACKSPEEDBIAS){
			set_ae_auto_BlackSpeedBias((void*)nd);
		}else if( nd->type == GET_PIC_AE_AUTO_BLACKSPEEDBIAS){  
			get_ae_auto_BlackSpeedBias((void*)nd);
		}else if( nd->type == SET_PIC_AE_AUTO_TOLERANCE){ 
			set_ae_auto_Tolerance((void*)nd);
		}else if( nd->type == GET_PIC_AE_AUTO_TOLERANCE){  
			get_ae_auto_Tolerance((void*)nd);
		}else if( nd->type == SET_PIC_AE_AUTO_EVBIAS){  
			set_ae_auto_EVBias((void*)nd);
		}else if( nd->type == GET_PIC_AE_AUTO_EVBIAS){  
			get_ae_auto_EVBias((void*)nd);
		}
		
		
		if ( ver == 5 ){
			dpf( "should send back: sock=%d\n", sock );
			sent = send( sock, nd, nd->len, 0 );
			if ( sent != nd->len ){
				dpf( "send failed\n" );
				goto end;
			}
		}
    }

 end:
 	printf( "end of network control----sock: %d\n", sock );

	close_sock_ctl( sock );
	free( arg );
	return 0;
}

void* thread_network_control()
{
	pthread_t          	ctlThread;
	pthread_attr_t    	attr;	
	//int 		        status;
	int                 sock;
	int                 sock_client;
	int                 *pSock;
	//int			i;
	struct sockaddr_in  addr;
	int addr_len = sizeof(struct sockaddr_in);	  
	int port = g_pSysInfo->lan_config.net.system_port;
	
	dpf( "control port: %d\n", port );
	
	sock = socket_open_listen( port );
	if( sock < 0 )
		return 0;
	
	ctlEnv.sock = sock;
		
	/* Initialize the thread attributes */
	if (pthread_attr_init(&attr)) {
		dpf("Failed to initialize thread attrs\n");
	 }

	//set detach, 
	if ( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ){
	    dpf( "Failed to set detached\n" );
	}

	while( 1 ){		
		
		sock_client = accept(sock, &addr, &addr_len);
		if ( sock_client > 0 ){

			if( get_sock_ctl(sock_client) < 0 ){
				close( sock_client );
				continue;
			}			
			
			sstw_error(" [sys] accept from: %s, port = %d: sock = %d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), sock_client );

			pSock = (int *)malloc( sizeof(int) );  //
			*pSock = sock_client;
			//g_sock = pSock;
			dpf( "control accepted....sock: %d \n", sock_client );
			if (pthread_create(&ctlThread, &attr, thread_ctrl, (void*)pSock)) {
				free( pSock );
				dpf("Failed to create control thread\n");
				exit(0);
				//goto cleanup;
			}
		}else{			
			perror( "control accept failed \n" );
		}        
	}

cleanup:
	close(sock);

	pthread_attr_destroy(&attr);

	return 0;
}


int start_network_control(pthread_t ctlThread)
{
	pthread_attr_t    	attr;	
		
	memset( g_client_pool, 0, sizeof(g_client_pool) );
	memset( &ctlEnv, 0, sizeof(ctlEnv) );

	/* Initialize the thread attributes */
	if (pthread_attr_init(&attr)) {
		dpf("Failed to initialize thread attrs\n");
	 }
	
	//set detach, 
	if ( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ){
	    dpf( "Failed to set detached\n" );
	}
	
	if (pthread_create(&ctlThread, &attr, thread_network_control, (void*)NULL)) {
		dpf("Failed to create network control thread\n");
		exit(0);
	}	

	ctlEnv.tid = ctlThread;
	
	pthread_attr_destroy(&attr);

	return 1;

}




