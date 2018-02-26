#include "util.h"
#include "net_config.h"

SysInfo *g_pSysInfo = NULL;  // share memory with cgi

static SysInfo SysInfoDefault =
{
	{
		TITLE_DEFAULT,		
		{
			{IP_DEFAULT},
			{NETMASK_DEFAULT},
			{GATEWAY_DEAFULT},
			{DNS_DEFAULT},
			HTTP_PORT_DEFAULT,
			DHCP_ENABLE_DEFAULT,
			DEFAULTTCP,			
			MAC_DEFAULT,			
      	},
	},

	DEVICE_TYPE_DEFAULT,

	DEVICENAME_DEFAULT,

	ACOUNT_DEFAULT,

// 	CUR_LOG_DEFAULT,

	AVCONFGDEF,

	RECORDDEF,

	DEFAULTDDNS,

	DEFAULTVIEWCOLOR,

	DEFAULTMA,

	DEFAULT_IODOOR,
	DEFAULT_NOISE_LEVEL,
	DEFAULT_COLOR_SWITCH,
	DEFAULT_EXP_LEVEL,
	.osd_info = {{0,1,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}}
};

int start_task(pthread_t *thd, thread_fun thd_fun, void* param)
{
	pthread_attr_t    	attr;	
		
	/* Initialize the thread attributes */
	if (pthread_attr_init(&attr)) {
		dpf("Failed to initialize thread attrs\n");
	 }
	
	//set detach, 
	if ( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ){
	    dpf( "Failed to set detached\n" );
	}
	
	if (pthread_create(thd, &attr, thd_fun, param)) {
		dpf("Failed to create network control thread\n");
		exit(0);
	}	
		
	pthread_attr_destroy(&attr);

	return 0;

}

int initthread_attr( pthread_attr_t * attr )
{   
	/* Initialize the thread attributes */
	if (pthread_attr_init(attr)) {
	    dpf("Failed to initialize thread attrs\n");
	    return 0;
	}

	/* Force the thread to use custom scheduling attributes */
	if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED)) {
	    dpf("Failed to set schedule inheritance attribute\n");
        return 0;
	}

	/* Set the thread to be fifo real time scheduled */
	if (pthread_attr_setschedpolicy(attr, SCHED_FIFO)) {
	    dpf("Failed to set FIFO scheduling policy\n");
	    return 0;
	}
    return 1;
}

int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return(fcntl(fd, F_SETLK, &fl));
}

int already_running(void)
{
    int     fd;
    char    buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        dpf( "can't open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
			
            close(fd);
            return(1);
        }
        dpf( "can't lock %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return(0);
}

#define LOG_FILE "/tmp/run_log"


#define GV_CFG_LINE_LEN	64

/*
	@ return: ret < 0, open filed
			 @ > 0, open success;
 */
FILE* zs_open_cfg( char* cfg_file )
{
	FILE* fin=fopen(cfg_file,"r");  

	if( fin == NULL )
	{
		fin = fopen( cfg_file, "w" );
		if( fin == NULL )
		{
			sstw_error( "open cfg file( %s ) failed", cfg_file );
			return (FILE*)-1;
		}else
		{
			return fin;
		}
	}
	return fin;
}

int zs_get_cfg_int(const char *section, const char *keyname, int def_value, char* cfg_file)
{
	FILE   *fin;  
	char   temp[GV_CFG_LINE_LEN],*ptr,*key_value;  
	char	*ptr_e;
	int		value = 0;
	int		is_getvalue = 0;
	int		ret = def_value;

	fin = zs_open_cfg( cfg_file );
	if( fin <= 0 )
		return ret;
		
	while(1)  
	{  
		memset( temp, 0, sizeof(temp) );
		if( fgets(temp,GV_CFG_LINE_LEN-1,fin) == 0 )
			break;
		
		ptr=strstr(temp,keyname);
		if(  ptr != NULL )
		{  
			ptr_e = strstr( ptr, "=" );
			key_value = ptr_e+1;
			value = atoi( key_value );
			is_getvalue = 1;
		}    
	}  

	fclose( fin );

	if( is_getvalue )
		return(value);
	else 
		return ret;
}

int zs_set_cfg_int( const char*section, const char* keyname, int value, char* cfg_file )
{	

	FILE   *fin,*fout;  
	char   temp[GV_CFG_LINE_LEN],*ptr, *ptr_e;
	char    **key_value = NULL;  
	int		j = 0;  
	int		line_total = 0;
	int		is_find = 0;
	
	fin = zs_open_cfg( cfg_file );
	if( fin < 0 )
		return -1;
	
	while(fgets(temp,GV_CFG_LINE_LEN-1,fin)) line_total++; 
	
	//dpf( "totoal line: %d\n", line_total );

	rewind(fin); 

	if( 0 == line_total )
	{
		fclose( fin );
		goto write_over;
	}

	if(!(key_value=(char**)malloc(line_total*sizeof(char*))) )
		return -1;
	
	for(j=0;j<line_total;j++)  
	{
		if(!(key_value[j]=(char*)malloc(GV_CFG_LINE_LEN)))
		{
			return -1;
		}
	}

	j = 0;
	while(j<line_total)  
	{  		
		memset( temp, 0, sizeof(temp) );
   		fgets(temp,GV_CFG_LINE_LEN-1,fin); 
		
		if((ptr=strstr(temp,keyname)) )
		{  
			ptr_e = strstr( ptr, "=" );
			*(ptr_e+1) = 0;
			sprintf( temp+strlen(temp), "%d", value );
			strcat(temp,"\n");  
			is_find = 1;
		}    

		strcpy( key_value[j], temp );

		j++;
	}  

	fclose(fin); 
	
write_over:

	fout=fopen(cfg_file,"w");  

	j=0;  	
	while(j<line_total) fputs(key_value[j++],fout);

	if( !is_find )
	{
		memset( temp, 0, sizeof(temp) );
		sprintf( temp, "%s=%d", keyname, value );
		fputs( temp, fout );
	}

	fclose(fout);
	
	j=0;  

	while(j<line_total) free(key_value[j++]);  

	if( key_value != NULL )
		free( key_value );

	return 0;
}

//extern eliu_param_t env_eliu;

char channel_name[128] = {0};

#include "sample_comm.h"

/******************************************************************************
* function : get picture size(w*h), according Norm and enPicSize
******************************************************************************/
int sstw_get_pic_size(GV_PIC_SIZE enPicSize, SSTW_SIZE_T *pstSize)
{
    switch (enPicSize)
    {
        case GV_PIC_QCIF:
            pstSize->uWidth  = 176;
            pstSize->uHeight = 144;
            break;
        case GV_PIC_QCIF_N:
            pstSize->uWidth  = 176;
            pstSize->uHeight = 120;
            break;
        case GV_PIC_CIF:
            pstSize->uWidth  = 352;
            pstSize->uHeight = 288;
            break;
        case GV_PIC_CIF_N:
            pstSize->uWidth  = 352;
            pstSize->uHeight = 240;
            break;
        case GV_PIC_D1:
            pstSize->uWidth  = 720;
            pstSize->uHeight = 576;
            break;        
        case GV_PIC_D1_N:
            pstSize->uWidth  = 720;
            pstSize->uHeight = 480;
            break;
        case GV_PIC_960H:
            pstSize->uWidth  = 960;
            pstSize->uHeight = 576;
            break;		
        case GV_PIC_960H_N:
            pstSize->uWidth  = 960;
            pstSize->uHeight = 480;
            break;		
        case GV_PIC_2CIF:
            pstSize->uWidth  = 360;
            pstSize->uHeight = 576;
            break;	
        case GV_PIC_2CIF_N:
            pstSize->uWidth  = 360;
            pstSize->uHeight = 480;
            break;
        case GV_PIC_QVGA:    /* 320 * 240 */
            pstSize->uWidth = 320;
            pstSize->uHeight = 240;
            break;
        case GV_PIC_VGA:     /* 640 * 480 */
            pstSize->uWidth = 640;
            pstSize->uHeight = 480;
            break;
        case GV_PIC_XGA:     /* 1024 * 768 */
            pstSize->uWidth = 1024;
            pstSize->uHeight = 768;
            break;


		case GV_PIC_SXGA:    /* 1400 * 1050 */
            pstSize->uWidth = 1400;//1408;//1400;
            pstSize->uHeight = 1050;// 960;//1050;
            break;

        case GV_PIC_UXGA:    /* 1600 * 1200 */
            pstSize->uWidth = 1600;
            pstSize->uHeight = 1200;
            break;
        case GV_PIC_QXGA:    /* 2048 * 1536 */
            pstSize->uWidth = 2048;
            pstSize->uHeight = 1536;
            break;
        case GV_PIC_WVGA:    /* 854 * 480 */
            pstSize->uWidth = 854;
            pstSize->uHeight = 480;
            break;
        case GV_PIC_WSXGA:   /* 1680 * 1050 */
            pstSize->uWidth = 1680;
            pstSize->uHeight = 1050;
            break;
        case GV_PIC_WUXGA:   /* 1920 * 1200 */
            pstSize->uWidth = 1920;
            pstSize->uHeight = 1200;
            break;
        case GV_PIC_WQXGA:   /* 2560 * 1600 */
            pstSize->uWidth = 2560;
            pstSize->uHeight = 1600;
            break;
        case GV_PIC_HD720:   /* 1280 * 720 */
            pstSize->uWidth = 1280;
            pstSize->uHeight = 720;
            break;
        case GV_PIC_HD1080:  /* 1920 * 1080 */
            pstSize->uWidth = 1920;
            pstSize->uHeight = 1080;
            break;
		case GV_PIC_HD960:   /* 1280 * 720 */
            pstSize->uWidth = 1280;
            pstSize->uHeight = 960;
            break;

		case GV_PIC_2304x1296:    /* 3M:2304 * 1296 */
			pstSize->uWidth = 2304;
			pstSize->uHeight = 1296;
			break;

		case  GV_PIC_2592x1520: /* 4M:2592 * 1520 */
			pstSize->uWidth = 2592;
			pstSize->uHeight = 1520;
			break;
		case GV_PIC_5M:     /* 2592 * 1944 */
			pstSize->uWidth = 2592;
			pstSize->uHeight = 1944;
			break;
							
		case GV_PIC_800x600:   
            pstSize->uWidth = 800;
            pstSize->uHeight = 600;
            break;
								
		case GV_PIC_1600x900:  		
			pstSize->uWidth = 1600;
			pstSize->uHeight = 900;
			break;

		case GV_PIC_1440x900:  		
			pstSize->uWidth = 1440;
			pstSize->uHeight = 900;
			break;
		case GV_PIC_1366x768:  		
			pstSize->uWidth = 1366;
			pstSize->uHeight = 768;
			break;
		case GV_PIC_1360x768:  		
			pstSize->uWidth = 1360;
			pstSize->uHeight = 768;
			break;
		case GV_PIC_1280x1024:  		
			pstSize->uWidth = 1280;
			pstSize->uHeight = 1024;
			break;		
		case GV_PIC_1280x800:  		
			pstSize->uWidth = 1280;
			pstSize->uHeight = 800;
			break;	
		case GV_PIC_1280x768:  		
			pstSize->uWidth = 1280;
			pstSize->uHeight = 768;
			break;

        default:
            return false;
    }
    return true;
}


int create_sys_file(char *name, void *Global){
	FILE *fp;
	int ret;
	unsigned long MagicNum = MAGIC_NUM;
	if((fp = fopen(name, "wb")) == NULL){
		sstw_error("Can't create system file:  %s", name);
		ret = FAIL;
	} else {
		if(fwrite(&MagicNum, 1, sizeof(MagicNum), fp) != sizeof(MagicNum)){
			sstw_error("Writing Magic Number fain");
			ret = FAIL;
		} else {
			if(fwrite(Global, 1, SYS_ENV_SIZE, fp) != SYS_ENV_SIZE){
				sstw_error("Writing global fail");
				ret = FAIL;
			} else {
				ret = SUCCESS;
			}
		}
		fclose(fp);
	}
	return ret;
}

int check_magic_num(FILE *fp)
{
	int ret;
	unsigned long MagicNum;
	if(fread(&MagicNum, 1, sizeof(MagicNum), fp) != sizeof(MagicNum)){
		ret = FAIL;
	} else {
		if(MagicNum == MAGIC_NUM){
			ret = SUCCESS;
		} else {
			ret = FAIL;
		}
	}
	return ret;
}
/**
 * @brief	read SysInfo from system file
 * @param	"void *Buffer" : [OUT]buffer to store SysInfo
 * @return	error code : SUCCESS(0) or FAIL(-1)
 */
int ReadGlobal(void *Buffer)
{
	FILE *fp;
	int ret;
	int read = 0;
	int i = 0;

	if((fp = fopen(SYS_FILE, "rb")) == NULL){
		/* System file not exist */
		sstw_error( "%s not exist.. open failed..", SYS_FILE );
		ret = FAIL;
	} else {
		if(check_magic_num(fp) == SUCCESS)
		{
			int count = 0;
			for( i = 0; i < 10; i++ )
			{
				read = fread(Buffer+count, 1, SYS_ENV_SIZE - count,fp); 
				count += read;
				dpf( "read count :%d\n", count );
				if( count >= SYS_ENV_SIZE )	break;
			}
			if( count != SYS_ENV_SIZE )
			{
				sstw_error( "read sysenv.cfg failed, read bytes: %d != %d",read, SYS_ENV_SIZE );
				sstw_error("err_sysenv_read" );
				ret = FAIL;
			} else {
				ret = SUCCESS;
			}
		} else {
			sstw_error( "check_magic_unm failed" );
			sstw_error("err_magic_num" );
			ret = FAIL;
		}
		fclose(fp);
	}
	return ret;
}

/**
 * @brief	write SysInfo to system file
 * @param	"void *Buffer" : [IN]buffer of SysInfo
 * @return	error code : SUCCESS(0) or FAIL(-1)
 */
int WriteGlobal(void *Buffer)
{
// 	int ret;
//	ret =create_log_file(LOG_FILE, gLogHead);
	return create_sys_file(SYS_FILE, Buffer);
}

int get_network( Network_Config_Data* net )
{
	if( NULL == net )
		return 0;

	Network_Config_Data network ;
	
	FILE * pf = fopen( NETWORK_FILE, "r" );
	if( NULL != pf )
	{
		int ret = fread( &network, 1, sizeof(network), pf );
		if (ret != sizeof(network))
		{
			sstw_error("err_network_file", ERR_READ_CFG );
			return -1;
		}
		memcpy( net, &network, sizeof(network) );
		return 0;
	}
	else
	{	
		dpf( "open %s failed\n", NETWORK_FILE );
				
		save_network( &(SysInfoDefault.lan_config.net) );
		memcpy( net, &(SysInfoDefault.lan_config.net), sizeof(network) );

		return 0;
	}

	return 0;
}

int save_network( Network_Config_Data* net )
{
	if( NULL == net )
		return -1;
		
	FILE* pf = fopen( NETWORK_FILE, "w" );
	if( pf == NULL )
		return -1;

	fwrite( net, 1, sizeof(Network_Config_Data), pf );
	fflush( pf );
	fclose( pf );

	printf( "save network cfg success\n" );
	
	return 0;
}

void SystemInfoInit(SysInfo* sys)
{
	(sys)->sharpen = 20;  //10-30; default: 20
//	sys->exposure_time = 20;	//exposure 
	
	(sys)->size_extch.uWidth = 1280;
	(sys)->size_extch.uHeight = 720;

	(sys)->size_vo.uWidth = 720;
	(sys)->size_vo.uHeight = 576;

	(sys)->backlight_com = 0x30;
	(sys)->exposure_compensation = 0x38;
		
	(sys)->exposure_type = OP_TYPE_AUTO;
	(sys)->exposure_time_manu = 0x4000;
	(sys)->a_gain = 0;
	(sys)->d_gain = 0;
	(sys)->isp_gain = 0;
	(sys)->nf_3d_level = 700;    //steven 2017-01-16 3D NF Level
	
	(sys)->ae_auto_u8Speed = 0x40;  //steven 2017-01-19
	(sys)->u16BlackSpeedBias = 0x90;
	(sys)->u8Tolerance = 0x2;
	(sys)->u16EVBias = 0x400;



}

int FileMngInit(SysInfo *ShareMem)
{
	int ret;
	dpf("Global value size:%d\n", SYS_ENV_SIZE);
	ret = ReadGlobal(ShareMem);
	if(ret == FAIL){
		sstw_error( "filemnginit, read sysenv failed, set default..." );
				
		//SysInfoDefault.is_color_switch = 
		ret = create_sys_file(SYS_FILE, &SysInfoDefault);
		if(ret == SUCCESS)	
		{
			memcpy(ShareMem, &SysInfoDefault, SYS_ENV_SIZE);
			SystemInfoInit(ShareMem);
			dpf("wwww=%d, hhh = %d\n",ShareMem->size_extch.uWidth, ShareMem->size_extch.uHeight);
		}
	}
	else
	{
		dpf( "read gloable data successed\n" );
	}
		
	//read network cfg
	ret = get_network(&(ShareMem->lan_config.net));
	get_mac(&(ShareMem->lan_config.net));
	return 0;
}

int init_system_info()
{
	g_pSysInfo = (SysInfo *)malloc(sizeof(SysInfo));
	if( NULL == g_pSysInfo )
	{
		dpf( "malloc system_info failed\n" );
		return -1;
	}
#ifdef _NETTHREAD	
	SysInfo sys_info;

	if(FileMngInit(&sys_info) != 0)
	{
		return -1;
	}
	memcpy( g_pSysInfo, &sys_info, sizeof(SysInfo) );
#endif
	g_pSysInfo->sensor_type = 122;
#ifdef _PRINTF
	dpf( "tcpport: %d\n", g_pSysInfo->lan_config.net.system_port  );	
	dpf("wwww=%d, hhh = %d\n",g_pSysInfo->size_extch.uWidth, g_pSysInfo->size_extch.uHeight);
#endif


	return 0;
}
