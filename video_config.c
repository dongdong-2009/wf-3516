
#include"video_config.h"
#include "hi_mpp_isp_fun.h"
#include "util.h"

HI_S32 VideoStandard = VIDEO_NORM_PAL;

extern VENC_CHN g_VeChn;
extern VENC_CHN g_VeSubChn;

extern int ad_type;
extern int enc_fd[2];
extern int dvr_fd;

extern int STREAM_GOP;

extern struct md_cfg md_param;
extern VPSS_GRP g_VpssGrp ;

extern int VI_FPS;
//int g_max_vi_frame_rate = 25;

HI_S32 g_VDec = -1;
RECORD *pRecord;

struct CONFGAV *avConfg;

extern SysInfo * g_pSysInfo;
int g_is_color = true;

v_u8 MotionBlock[160][160];
v_u32 REC_FPS = 8;

//extern eliu_param_t env_eliu;

int init_av_config()
{	
	int i;
			
	//video encode
	for (i = 0; i < sizeof( g_pSysInfo->avConfg)/sizeof( g_pSysInfo->avConfg[0]); i++)
	{
		if (0 == g_pSysInfo->avConfg[i].codec_id)
		{
			g_pSysInfo->avConfg[i].codec_id = GV_CODEC_ID_HI264;
		}
	}

	//it is time to change the default value;
	if( BITRATE_DEF == g_pSysInfo->avConfg[0].bitrate )
	{
		g_pSysInfo->avConfg[0].bitrate = 3*1024;
		
		//if 200 Mega cam
		if( GV_IPCAM_HI3516_AR0330 == ad_type || GV_IPCAM_HI3516_SONY122 == ad_type
			|| GV_IPCAM_HI3518EV200_IMX323 == ad_type || GV_IPCAM_HI3518EV200_SC2035 == ad_type)
		{
			g_pSysInfo->avConfg[0].bitrate = 3*1024;
		}
		else if (GV_IPCAM_HI3516D_OV4689 == ad_type || GV_IPCAM_HI3516A_OV4689 == ad_type)   //4M
		{
			if (g_pSysInfo->avConfg[0].codec_id == GV_CODEC_ID_HI264)
			{
				g_pSysInfo->avConfg[0].bitrate = 7*1024;
			}
			else
			{
				g_pSysInfo->avConfg[0].bitrate = 3*1024;
			}
		}
		else
		{
			g_pSysInfo->avConfg[0].bitrate = 2*1024;
		}
		
		//if (GV_IPCAM_HI3518E_AR0130 == ad_type || GV_IPCAM_HI3518E_OV9712 == ad_type)
		{
#ifdef _PRINTF
			printf( "default value...512\n" );
#endif
			g_pSysInfo->avConfg[1].bitrate = 512;
		}
	}
#ifdef _PRINTF	
	sstw_info( "sensor type: %d, bitrate = %d", ad_type, g_pSysInfo->avConfg[0].bitrate );
#endif
	avConfg = g_pSysInfo->avConfg;
		
	//default value ------------------------------------------
	v_u32 main_stream_res = GV_PIC_HD720;
	if( GV_IPCAM_HI3518E_AR0130 == ad_type || GV_IPCAM_HI3518E_MIS1011 == ad_type )
		main_stream_res = GV_PIC_HD720;
	
	else if( GV_IPCAM_HI3516A_BT1120_1080P == ad_type || GV_IPCAM_HI3516_AR0330 == ad_type 
		|| GV_IPCAM_HI3516_SONY122 == ad_type || GV_IPCAM_HI3518EV200_IMX323 == ad_type 
		|| GV_IPCAM_HI3518EV200_SC2035 == ad_type)
		main_stream_res = GV_PIC_HD1080;
	
	else if( GV_IPCAM_HI3518E_IMX225 == ad_type  )
		;//main_stream_res = GV_PIC_HD960;   //960

	else if(GV_IPCAM_HI3516D_OV4689 == ad_type || GV_IPCAM_HI3516A_OV4689 == ad_type)
		main_stream_res = GV_PIC_2592x1520;   //4M
	
	//else if (GV_IPCAM_HI3518E_OV3660 == ad_type)
	//	main_stream_res = GV_PIC_XGA;

	else if( GV_IPCAM_TYPE == ad_type || GV_IPCAM_TYPE == ad_type )
		main_stream_res = GV_PIC_D1;
	//--------------------------------------------------default value

	//if setup to another resolution, get it!
	if( avConfg[0].res_sel == GV_PIC_HD720 )			//avConfg[0].res_sel  default = GV_PIC_HD720
		main_stream_res = GV_PIC_HD720;
	else if( avConfg[0].res_sel == GV_PIC_HD960 )
		main_stream_res = GV_PIC_HD960;		
	else if( avConfg[0].res_sel == GV_PIC_UXGA )
		main_stream_res = GV_PIC_UXGA;

	if( GV_IPCAM_HI3516A_BT1120_1080P == ad_type )
	{
		main_stream_res = avConfg[0].res_sel;
#ifdef _PRINTF
		dpf("main stream= %d\n", main_stream_res);
#endif
	
	}

	//----sub stream-------------
	v_u32 sub_stream_res = GV_PIC_VGA;
		
#ifdef hi3518e
	sub_stream_res = GV_PIC_QVGA;
#endif
	
	if( avConfg[1].res_sel == GV_PIC_QVGA )
		sub_stream_res = GV_PIC_QVGA;	
	else if( avConfg[1].res_sel == GV_PIC_HD720 )
		sub_stream_res = GV_PIC_HD720;

	if (ad_type == GV_IPCAM_HI3516A_BT1120_1080P)
	{
		sub_stream_res = main_stream_res;
	}
	
	//----sub2 ---------------------
	v_u32 sub2_stream_res = GV_PIC_QVGA;	
	
	//-----------------------------------------
	avConfg[0].res_sel = main_stream_res;
	avConfg[1].res_sel = sub_stream_res;
	avConfg[2].res_sel = sub2_stream_res;
	
	SSTW_SIZE_T pic_size;
	if( sstw_get_pic_size( main_stream_res, &pic_size ) == 0 )
	{
		sstw_error( "get pic size wrong. [%d] ", pic_size );
		return false;
	}
	
	avConfg[0].width  = pic_size.uWidth;
	avConfg[0].height = pic_size.uHeight;
			
	sstw_get_pic_size( sub_stream_res, &pic_size );

	avConfg[1].width  = pic_size.uWidth;
	avConfg[1].height = pic_size.uHeight;

	sstw_get_pic_size( sub2_stream_res, &pic_size );

	avConfg[2].width  = pic_size.uWidth;
	avConfg[2].height = pic_size.uHeight;

	pRecord = &g_pSysInfo->record_config;
#ifdef _PRINTF
	dpf( "res: %d, width: %d, height: %d width: %d, height: %d, fps0: %d, fps1: %d \n", main_stream_res, avConfg[0].width, avConfg[0].height, avConfg[1].width, avConfg[1].height, avConfg[0].fps, avConfg[1].fps );
	#endif
	
	return 0;
}

void UnInitSysInfo()
{
	//shmdt( g_pSysInfo );
	shmctl(SYS_MSG_KEY, IPC_RMID, 0);
	g_pSysInfo = NULL;
	avConfg = NULL;
	pRecord = NULL;
	
}

HI_S32 open_video_decode()
{
		
	return HI_SUCCESS;
}

HI_VOID close_video_decode()
{
	if (g_VDec > 0)
	{
		close(g_VDec);
	}
}


HI_S32 set_picture_color(COLOR_CONFG* cc)
{
    COLOR_CONFG color_c;
    
    memcpy( &color_c, cc, sizeof(color_c) );
#ifdef _PRINTF
	dpf( "b: %d, c: %d, s: %d, h: %d, sa:%d\n", color_c.brightness, color_c.contrast, color_c.sharpness, color_c.hue, color_c.saturation);
#endif	
	
	HI_S32 s32Ret;

	VI_CSC_ATTR_S stCscAttr;	
	
	memset( &stCscAttr, 0, sizeof(stCscAttr) );
	s32Ret = HI_MPI_VI_GetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("Get vi CSC attr err:0x%x\n", s32Ret);
		return s32Ret;
	}
		
	stCscAttr.u32ContrVal	= color_c.contrast;
	stCscAttr.u32HueVal		= color_c.hue;
	if (color_c.brightness > 10)
		stCscAttr.u32LumaVal = color_c.brightness - 10;
	else
		stCscAttr.u32LumaVal = 1;

	stCscAttr.u32SatuVal = color_c.saturation;			
	stCscAttr.enViCscType = VI_CSC_TYPE_709;
	/* Set attribute for vi CSC attr */
	s32Ret = HI_MPI_VI_SetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		sstw_error("Set vi CSC attr err:0x%x", s32Ret);
		return s32Ret;
	}
	
	/* Get attribute for vi CSC attr */
	memset( &stCscAttr, 0, sizeof(stCscAttr) );
	s32Ret = HI_MPI_VI_GetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("Get vi CSC attr err:0x%x\n", s32Ret);
		return s32Ret;
	}
	
	//printf( "brightness: %d, hue: %d, contrast: %d \n", stCscAttr.u32LumaVal, stCscAttr.u32HueVal, stCscAttr.u32ContrVal );
		
    return GV_SUCCESS;
}

void set_vi_csc_luma(int offset)
{	
	VI_CSC_ATTR_S stCscAttr;	
	

	memset( &stCscAttr, 0, sizeof(stCscAttr) );
	HI_S32 s32Ret = HI_MPI_VI_GetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("Get vi CSC attr err:0x%x\n", s32Ret);
		return;
	}

	int tmp = stCscAttr.u32LumaVal+offset;
	if (tmp < 0 )
		tmp = 0;
	else if (tmp >= 100)
		tmp = 99;
	
	// dpf("set csc luma = %d(offset=%d)\n", tmp, offset); //hui 2017-04-09
	
	stCscAttr.u32LumaVal = tmp;
		
	stCscAttr.enViCscType = VI_CSC_TYPE_709;
	/* Set attribute for vi CSC attr */
	s32Ret = HI_MPI_VI_SetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		sstw_error("Set vi CSC attr err:0x%x", s32Ret);
		return;
	}	
}

#if 0    //steven 2017-3-1
void init_video_decode()
{			
	set_noise_level( g_pSysInfo->noise_level, 1 );
	set_picture_exposure_compensation( g_pSysInfo->exposure_compensation );
	set_picture_mirror( g_pSysInfo->is_mirror );
	set_picture_flip( g_pSysInfo->is_flip );
	
	get_sensor_sharpen();
	set_picture_sharpen(g_pSysInfo->sharpen);
	set_exposure_time(g_pSysInfo->exposure_time);
	set_backlight_compensation(g_pSysInfo->backlight_com);
	//steven 2017-01-19
	set_picture_ae_auto_speed(g_pSysInfo->ae_auto_u8Speed);
	set_picture_ae_auto_BlackSpeedBias(g_pSysInfo->u16BlackSpeedBias);
	set_picture_ae_auto_Tolerance(g_pSysInfo->u8Tolerance);
	set_picture_ae_auto_EVBias(g_pSysInfo->u16EVBias);

	set_picture_a_gain(g_pSysInfo->a_gain);
	set_picture_d_gain(g_pSysInfo->d_gain);
	set_picture_isp_gain(g_pSysInfo->isp_gain);
	set_picture_3d_nf_level(g_pSysInfo->nf_3d_level);    //steven 2017-01-16 3D NF Level
	set_picture_expo_time_mamu(g_pSysInfo->exposure_time_manu);
	set_picture_exposure_type(g_pSysInfo->exposure_type);

	if (GV_IPCAM_HI3516A_OV4689 == ad_type)
		set_color2grey(0);

	set_picture_color( &g_pSysInfo->viewcolor );

}

#else
void init_video_decode()
{			
	set_color2grey(false);
	set_picture_color( &g_pSysInfo->viewcolor );

	set_noise_level( g_pSysInfo->noise_level, 1 );
	set_picture_exposure_compensation( g_pSysInfo->exposure_compensation );
	set_picture_mirror( g_pSysInfo->is_mirror );
	set_picture_flip( g_pSysInfo->is_flip );
	
	get_sensor_defog();
	set_defog(g_pSysInfo->defog_flag);
	get_sensor_dis();
	set_dis(g_pSysInfo->dis_flag);
	get_sensor_dci();
	set_dci(g_pSysInfo->dci_flag);
	
	
	get_sensor_sharpen();
	set_picture_sharpen(g_pSysInfo->sharpen); 
	set_exposure_time(g_pSysInfo->exposure_time);
	set_backlight_compensation(g_pSysInfo->backlight_com);

	if (OP_TYPE_MANUAL == g_pSysInfo->exposure_type)
	{
		set_picture_exposure_type(g_pSysInfo->exposure_type);
		set_picture_a_gain(g_pSysInfo->a_gain);
		set_picture_d_gain(g_pSysInfo->d_gain);
		set_picture_isp_gain(g_pSysInfo->isp_gain);
		set_picture_expo_time_mamu(g_pSysInfo->exposure_time_manu);
	}

	set_picture_3d_nf_level(g_pSysInfo->nf_3d_level);    //steven 2017-01-16 3D NF Level
}
#endif


HI_S32 set_stream_attr( VENC_GRP VeG, VENC_CHN VeC, void* configValue )
{
    if( VeC > 2 )
        return GV_FAILURE;
	
    HI_S32 s32Ret = 0;
	
    struct CONFGAV config;
    memcpy( &config, configValue, sizeof(config) );
#ifdef _PRINTF
	dpf( "sizeof CONFGAV: %d, fps: %d, bitrate: %d, gop=%f, is cbr: %d\n",
		sizeof(config),  config.fps, config.bitrate, config.Iframe_sec, config.encode_rc_mode);
#endif
	
	if (config.Iframe_sec == 0)		config.Iframe_sec = 30;
		
   		
    return s32Ret;
}

HI_S32 set_stream_res(VENC_GRP VeG, VENC_CHN VeC, resolution_t *res)
{
   	HI_S32 s32Ret = GV_SUCCESS;

    return s32Ret;

}

HI_BOOL get_stream_res( VENC_CHN VeC, v_u32 *width, v_u32 *height )
{
	if( VeC == STREAM_MAIN )
	{
		int res = get_stream_res_sel(VeC);

		SSTW_SIZE_T pic_size;
		if( sstw_get_pic_size( res, &pic_size ) == 0 )
		{
			sstw_error( "get pic size wrong. [%d] ", pic_size );
			return false;
		}

		*width  = pic_size.uWidth;
		*height = pic_size.uHeight;
		
	}
	else if (VeC == STREAM_SUB)
	{
		*width  = g_pSysInfo->size_extch.uWidth;
		*height = g_pSysInfo->size_extch.uHeight;
		if (*width == 0 || *height== 0)
		{
			*width = 1280;
			*height = 720;
		}
	}
	return true;
}

int get_stream_res_sel( VENC_CHN VeC )
{
	if( VeC > 2 )
	    return -1;
		
	int resolutiong = avConfg[VeC].res_sel;
	
	return resolutiong;
}



HI_S32 set_stream_pic_level(VENC_GRP VeG, VENC_CHN VeC, v_u32 pic_level)
{
	
    avConfg[VeC].pic_level = pic_level;
    
    return GV_SUCCESS;
    
}

HI_S32 get_picture_color(COLOR_CONFG* cc)
{   
    COLOR_CONFG color_c;

	memcpy( &color_c, &(g_pSysInfo->viewcolor), sizeof(COLOR_CONFG) );

	if( color_c.contrast > 10  )
	{		
		cc->contrast = color_c.contrast;
		cc->hue = color_c.hue;
		cc->brightness = color_c.brightness;
		cc->saturation = color_c.saturation;
	}
	else	// <= 10;  old version
	{
		cc->contrast = 50+(color_c.contrast-5)*5;
		cc->hue = 50+(color_c.hue-5)*5;
		cc->brightness = 50+(color_c.brightness-5)*5;
		cc->saturation = 50+(color_c.saturation-5)*5;		
	}

// 	memcpy( cc, &(g_pSysInfo->viewcolor), sizeof(COLOR_CONFG) );

	return 1;
		
	// 		dpf( "b: %d, c: %d, s: %d, h: %d, sa:%d\n", color_c.brightness, color_c.contrast, color_c.sharpness, color_c.hue, color_c.saturation);
	
	HI_S32 s32Ret;

	VI_CSC_ATTR_S stCscAttr;	
	stCscAttr.enViCscType = VI_CSC_TYPE_709;

	if( color_c.contrast > 100 )
	{
		return 0;
	}
	else if( color_c.contrast > 10  )
	{		
		stCscAttr.u32ContrVal = color_c.contrast;
		stCscAttr.u32HueVal = color_c.hue;
		stCscAttr.u32LumaVal = color_c.brightness;
		stCscAttr.u32SatuVal = color_c.saturation;
	}
	else	// <= 10;  old version
	{
		stCscAttr.u32ContrVal = 50+(color_c.contrast-5)*5;
		stCscAttr.u32HueVal = 50+(color_c.hue-5)*5;
		stCscAttr.u32LumaVal = 50+(color_c.brightness-5)*5;
		stCscAttr.u32SatuVal = 50+(color_c.saturation-5)*5;		
	}
	
	/* Set attribute for vi CSC attr */
	s32Ret = HI_MPI_VI_SetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("Set vi CSC attr err:0x%x\n", s32Ret);
		return s32Ret;
	}
	
	/* Get attribute for vi CSC attr */
	memset( &stCscAttr, 0, sizeof(stCscAttr) );
	s32Ret = HI_MPI_VI_GetCSCAttr(0, &stCscAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("Get vi CSC attr err:0x%x\n", s32Ret);
		return s32Ret;
	}
#ifdef _PRINTF
	printf( "brightness: %d, hue: %d, cont: %d \n", stCscAttr.u32LumaVal, stCscAttr.u32HueVal, stCscAttr.u32ContrVal );
#endif

}

HI_S32 set_noise_level( int noise, int b_color )
{
	
	return 1;
}

int get_noise_reduction( int noise_level, int is_color )
{
	if( ad_type == GV_IPCAM_HI3518_OV9712 || ad_type == GV_IPCAM_HI3518_OV9712P || ad_type == GV_IPCAM_HI3518E_OV9712)
	{	
		if( is_color )
			return noise_level * 10 + 5 + 20;
		else
			return noise_level * 10;
	}
	else
	{		
		if( is_color )
		{
			return noise_level * 10 + 3;
		}
		else
		{
			return noise_level * 10 + 5;
		}
	}
}


int set_picture_mirror( int b_mirror )
{
//	dpf( "set_picture_mirror: %d\n", b_mirror );

	if( b_mirror != 0 && b_mirror != 1 )
		return 0;
	
	int i =0;
	VPSS_CHN_ATTR_S stChnAttr;

	for (i = 0; i < 3; i++)
	{
		memset( &stChnAttr, 0, sizeof(stChnAttr) );

		int s32Ret = HI_MPI_VPSS_GetChnAttr(g_VpssGrp, i, &stChnAttr);
		//int s32Ret = HI_MPI_VI_GetChnAttr(ViChn, &stChnAttr);
		if (HI_SUCCESS != s32Ret)
		{
			printf("get vi chn attr err:0x%x\n", s32Ret);
			return s32Ret;
		}

		stChnAttr.bMirror = b_mirror;
		stChnAttr.s32SrcFrameRate = -1;

		stChnAttr.s32DstFrameRate = -1;

		s32Ret = HI_MPI_VPSS_SetChnAttr(g_VpssGrp, i, &stChnAttr);
		//s32Ret = HI_MPI_VI_SetChnAttr(ViChn,&stChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			printf("Set chn attributes failed with error code %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}


	return 1;
}

int get_picture_mirror()
{
	return g_pSysInfo->is_mirror;
}


int set_picture_flip( int b_flip )
{	
#ifdef _PRINTF
	dpf( "set_picture_flip: %d\n", b_flip );
#endif

	if( b_flip != 0 && b_flip != 1 )
		return 0;
		
	VI_CHN ViChn = 0;

	VPSS_CHN_ATTR_S stChnAttr;
	//VI_CHN_ATTR_S stChnAttr;
	
	int i = 0;
	for (i = 0; i < 3; i++)
	{
		memset( &stChnAttr, 0, sizeof(stChnAttr) );

		int s32Ret = HI_MPI_VPSS_GetChnAttr(g_VpssGrp, i, &stChnAttr);
		//int s32Ret = HI_MPI_VI_GetChnAttr(ViChn, &stChnAttr);
		if (HI_SUCCESS != s32Ret)
		{
			printf("get vi chn attr err:0x%x\n", s32Ret);
			return s32Ret;
		}

		stChnAttr.bFlip = b_flip;
		stChnAttr.s32SrcFrameRate = -1;

		stChnAttr.s32DstFrameRate = -1;

		s32Ret = HI_MPI_VPSS_SetChnAttr(g_VpssGrp, i, &stChnAttr);
		//s32Ret = HI_MPI_VI_SetChnAttr(ViChn,&stChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			printf("Set chn attributes failed with error code %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}

	return 1;
}

int get_picture_flip()
{
	return g_pSysInfo->is_flip;
}

HI_S32 set_vo_picture_rotate( ROTATE_E rotate)
{

	HI_S32 s32Ret = HI_MPI_VPSS_SetRotate(g_VpssGrp, VPSS_CHN_MAIN, rotate);
	if (HI_SUCCESS != s32Ret)
	{
		sstw_error("HI_MPI_VPSS_SetRotate err:0x%x", s32Ret);
		return s32Ret;
	}	
	return HI_SUCCESS;
}
//@param brightness:  -2 to 2
//@ new param 10 - 30, default 20;
HI_S32 set_picture_exposure_compensation( int exp_level )
{	
	if( exp_level < 0 || exp_level > 0xff )
		return 0;
	
	//if( exp_level == 0 )	exp_level = 0x38;   // exp_level == 0, maybe the default value of new param.
		
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_AUTO;
	stExpAttr.stAuto.u8Speed = 0x40;
#ifdef _PRINTF
	dpf( "the current expCompensation: %d, new exp = %d\n", stExpAttr.stAuto.u8Compensation, exp );
#endif

	stExpAttr.stAuto.u8Compensation = exp_level;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	return true;

}

HI_S32 ConfVideoColor_save(void* cc)
{
#ifdef _PRINTF
	printf( "save color.......contrast:.%d\n", g_pSysInfo->viewcolor.contrast);
#endif
	
    set_picture_color((COLOR_CONFG*)cc );	
	
    return GV_SUCCESS;
}

HI_S32 ConfVideoColor_set_default()
{
	COLOR_CONFG cc;
	cc.brightness = 5;
	cc.contrast = 5;
	cc.hue = 5;
	cc.saturation = 5;
	cc.sharpness = 5;
	
	ConfVideoColor_save( &cc );
	
	return GV_SUCCESS;
}

HI_BOOL set_video_norm( VENC_CHN VeC, v_u32* std )
{
    if( *std != VIDEO_NORM_PAL && *std != VIDEO_NORM_NTSC )
        return false;

    return true;
}


HI_S32 set_color2grey( int b_color )
{
	//int val, off;

	g_is_color = b_color;
#ifdef _PRINTF
	printf( "set color = %d\n", b_color );	
#endif


	if( 1 )// ad_type == GV_IPCAM_HI3518E_OV9712 || ad_type == GV_IPCAM_HI3518E_AR0130 )
	{
		ISP_SATURATION_ATTR_S pstSatAttr;
		ISP_DEV IspDev = 0;
		HI_S32 s32Ret = HI_MPI_ISP_GetSaturationAttr(IspDev, &pstSatAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetSaturationAttr failed!\n");
			// 		return s32Ret;
		}

		if( b_color )
		{
		
			pstSatAttr.enOpType = OP_TYPE_AUTO;
		}
		else
		{	
			pstSatAttr.enOpType = OP_TYPE_MANUAL;
			pstSatAttr.stManual.u8Saturation = 0;

		}

		s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetSaturationAttr failed!\n");
			// 		return s32Ret;
		}
	}
		
	set_picture_exposure_compensation( g_pSysInfo->exposure_compensation );
	
	return true;
}

//static HI_U8 MD_LEVEL_SEC[10] = { 2, 4, 6,8,10,12,14,16,18,20 };
HI_U8 MD_LEVEL = 4;

VIDEO_NORM_E get_video_norm()
{
	//spring_3660
	if (ad_type == GV_IPCAM_HI3518E_OV3660 || ad_type == GV_IPCAM_HI3518E_H22 || ad_type == GV_IPCAM_HI3518E_IMX225)
		return VIDEO_NORM_NTSC;

	if (NULL == g_pSysInfo )
	{
		return VIDEO_NORM_NUKNOW;
	}

	if( 0 == g_pSysInfo->video_norm )
		return VIDEO_NORM_PAL;
	else
		return VIDEO_NORM_NTSC;
}

ISP_SHARPEN_ATTR_S sensor_sharpen_attr;

void get_sensor_sharpen()
{	
	ISP_DEV IspDev = 0;
	if( HI_MPI_ISP_GetSharpenAttr( IspDev, &sensor_sharpen_attr ) )
	{
		dpf("fail to HI_MPI_ISP_GetSharpenAttr\n");
		return ;
	}
#ifdef _PRINTF
	printf("enable=%d, manual=%d\n", sensor_sharpen_attr.bEnable, sensor_sharpen_attr.enOpType);
#endif

	sensor_sharpen_attr.bEnable = 1;
	sensor_sharpen_attr.enOpType = OP_TYPE_AUTO;
	
}
void set_picture_sharpen( int sharpen )
{
#if 1
	//sharpen: 10 - 30;
	if( sharpen < 0 || sharpen > 90 )
		return;
	
	if( sharpen == 0 )	sharpen = 20;   // exp_level == 0, maybe the default value of new param.
	
	//
	float exp = 1.0;

	if (sharpen >= 20)
	{
		exp = exp + (sharpen-20) * 0.2;
	}
	else
	{
		exp = exp - (20 - sharpen) * 0.08;
	}
		
	ISP_SHARPEN_ATTR_S stSharpenAttr;
	ISP_DEV IspDev = 0;

	memcpy( &stSharpenAttr, &sensor_sharpen_attr, sizeof(sensor_sharpen_attr) );
#ifdef _PRINTF
	printf("enable=%d, manual=%d, exp=%f\n", stSharpenAttr.bEnable, stSharpenAttr.enOpType, exp);
#endif
	int i = 0;
	for ( i = 0; i < sizeof(stSharpenAttr.stAuto.au8SharpenD)/sizeof(stSharpenAttr.stAuto.au8SharpenD[0]); i++)
	{
		int tmp = stSharpenAttr.stAuto.au8SharpenD[i]*exp;
		int tmp2 = stSharpenAttr.stAuto.au8SharpenUd[i]*exp;
		int tmp3 = stSharpenAttr.stAuto.au8SharpenRGB[i]*exp;
		
		stSharpenAttr.stAuto.au8SharpenD[i] = tmp > 254 ? 254 : tmp;
		stSharpenAttr.stAuto.au8SharpenUd[i] = tmp2 > 254 ? 254 : tmp2;
		stSharpenAttr.stAuto.au8SharpenRGB[i] = tmp3 > 254 ? 254 : tmp3;

		//printf("harpend[%d] = %d, sharpenud[%d] = %d\n", i, stSharpenAttr.stAuto.au8SharpenD[i], i, stSharpenAttr.stAuto.au8SharpenUd[i]);
	}
	
	if( HI_MPI_ISP_SetSharpenAttr( IspDev, &stSharpenAttr ) )
	{
		dpf("fail to HI_MPI_ISP_SetSharpenAttr\n");
		return;
	}
#endif
}


ISP_DEFOG_ATTR_S sensor_defog_attr;

void get_sensor_defog()
{	
	ISP_DEV IspDev = 0;
	
	if( HI_MPI_ISP_GetDeFogAttr( IspDev, &sensor_defog_attr ) )
	{
		dpf("fail to HI_MPI_ISP_GetDeFogAttr\n");
		return ;
	}
//#ifdef _PRINTF
	printf("enable=%d, manual=%d, Hblack: %d, Vblack: %d, autoA:%d\n", sensor_defog_attr.bEnable, sensor_defog_attr.enOpType, 
		   sensor_defog_attr.u8HorizontalBlock, sensor_defog_attr.u8VerticalBlock, sensor_defog_attr.stAuto.u8strength);
//#endif

	
	sensor_defog_attr.enOpType = OP_TYPE_AUTO;
	
}
void set_defog(int defog_flag)
{
#if 1

	ISP_DEV IspDev = 0;
	sensor_defog_attr.bEnable = defog_flag;
	if( HI_MPI_ISP_SetDeFogAttr( IspDev, &sensor_defog_attr ) )
	{
		dpf("fail to HI_MPI_ISP_SetDeFogAttr\n");
		return;
	}
#endif
}



ISP_DIS_ATTR_S sensor_dis_attr;
void get_sensor_dis()
{	
	ISP_DEV IspDev = 0;
	
	if( HI_MPI_ISP_GetDISAttr( IspDev, &sensor_dis_attr ) )
	{
		dpf("fail to HI_MPI_ISP_GetDISAttr\n");
		return ;
	}
//#ifdef _PRINTF
	printf("sensor_dis_attr enable=%d\n", sensor_dis_attr.bEnable);
//#endif

	

}
void set_dis(int dis_flag)
{
#if 1

	ISP_DEV IspDev = 0;
	sensor_dis_attr.bEnable = dis_flag;


	if( HI_MPI_ISP_SetDISAttr( IspDev, &sensor_dis_attr ) )
	{
		dpf("fail to HI_MPI_ISP_SetDISAttr\n");
		return;
	}
#endif
}




VI_DCI_PARAM_S stDCIParam;
void get_sensor_dci()
{	
	ISP_DEV IspDev = 0;
	
	if( HI_MPI_VI_GetDCIParam( IspDev, &stDCIParam ) )
	{
		dpf("fail to HI_MPI_VI_GetDCIParam\n");
		return ;
	}
//#ifdef _PRINTF
	printf("sensor_dci_attr enable=%d, bD:%d\n", stDCIParam.bEnable, stDCIParam.u32BlackGain);
//#endif

	


}
void set_dci(int dci_flag)
{
#if 1

	ISP_DEV IspDev = 0;
	stDCIParam.bEnable = dci_flag;

	if( HI_MPI_VI_SetDCIParam( IspDev, &stDCIParam ) )
	{
		dpf("fail to HI_MPI_VI_SetDCIParam\n");
		return;
	}
#endif
}





#define BACKLIGHT_MAXHIST	0x40		//0x20

void set_exposure_time(v_u32 time)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	set_slowframe_rate(30,30);

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
#ifdef _PRINTF
	printf("bByPass=%d, manual=%d, min=%d, max=%d, u16HistRatioSlope=%d, u8MaxHistOffset=%d\n",
			stExpAttr.bByPass, stExpAttr.enOpType, stExpAttr.stAuto.stExpTimeRange.u32Min,stExpAttr.stAuto.stExpTimeRange.u32Max, 
			stExpAttr.stAuto.u16HistRatioSlope, stExpAttr.stAuto.u8MaxHistOffset);
		
#endif
	
	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_AUTO;
	stExpAttr.stAuto.stExpTimeRange.u32Max = time;
	stExpAttr.stAuto.stExpTimeRange.u32Min = 10;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
}

//steven 2017-01-19
void set_picture_ae_auto_speed(v_u32 value)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
#ifdef _PRINTF
	printf("u8Speed=%x, u16BlackSpeedBias=%x, u8Tolerance=%x, u16EVBias=%x\n", 
                        stExpAttr.stAuto.u8Speed, 
                        stExpAttr.stAuto.u16BlackSpeedBias,
			stExpAttr.stAuto.u8Tolerance,
			stExpAttr.stAuto.u16EVBias );
#endif

	//printf("bByPass=%d, manual=%d, min=%d, max=%d, u16HistRatioSlope=%d, u8MaxHistOffset=%d\n",
	//	stExpAttr.bByPass, stExpAttr.enOpType, stExpAttr.stAuto.stExpTimeRange.u32Min,stExpAttr.stAuto.stExpTimeRange.u32Max, 
	//	stExpAttr.stAuto.u16HistRatioSlope, stExpAttr.stAuto.u8MaxHistOffset);

	//stExpAttr.bByPass = HI_FALSE;
	//stExpAttr.enOpType = OP_TYPE_AUTO;
	if(value<0 || value>0xff)
	    return;
	    
	stExpAttr.stAuto.u8Speed = value;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	
//printf("set_picture_ae_auto_speed ok\n");

}

void set_picture_ae_auto_BlackSpeedBias(v_u32 value)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	//printf("bByPass=%d, manual=%d, min=%d, max=%d, u16HistRatioSlope=%d, u8MaxHistOffset=%d\n",
	//	stExpAttr.bByPass, stExpAttr.enOpType, stExpAttr.stAuto.stExpTimeRange.u32Min,stExpAttr.stAuto.stExpTimeRange.u32Max, 
	//	stExpAttr.stAuto.u16HistRatioSlope, stExpAttr.stAuto.u8MaxHistOffset);

	//stExpAttr.bByPass = HI_FALSE;
	//stExpAttr.enOpType = OP_TYPE_AUTO;
	if(value<0 || value>0xffff)
	    return;
	    
	stExpAttr.stAuto.u16BlackSpeedBias = value;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

//printf("set_picture_ae_auto_BlackSpeedBias ok\n");
}

void set_picture_ae_auto_Tolerance(v_u32 value)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	//printf("bByPass=%d, manual=%d, min=%d, max=%d, u16HistRatioSlope=%d, u8MaxHistOffset=%d\n",
	//	stExpAttr.bByPass, stExpAttr.enOpType, stExpAttr.stAuto.stExpTimeRange.u32Min,stExpAttr.stAuto.stExpTimeRange.u32Max, 
	//	stExpAttr.stAuto.u16HistRatioSlope, stExpAttr.stAuto.u8MaxHistOffset);

	//stExpAttr.bByPass = HI_FALSE;
	//stExpAttr.enOpType = OP_TYPE_AUTO;
	if(value<0 || value>0xff)
	    return;
	    
	stExpAttr.stAuto.u8Tolerance = value;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

//printf("set_picture_ae_auto_Tolerance ok\n");
}

void set_picture_ae_auto_EVBias(v_u32 value)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	//printf("bByPass=%d, manual=%d, min=%d, max=%d, u16HistRatioSlope=%d, u8MaxHistOffset=%d\n",
	//	stExpAttr.bByPass, stExpAttr.enOpType, stExpAttr.stAuto.stExpTimeRange.u32Min,stExpAttr.stAuto.stExpTimeRange.u32Max, 
	//	stExpAttr.stAuto.u16HistRatioSlope, stExpAttr.stAuto.u8MaxHistOffset);

	//stExpAttr.bByPass = HI_FALSE;
	//stExpAttr.enOpType = OP_TYPE_AUTO;
	if(value<0 || value>0xffff)
	    return;
	    
	stExpAttr.stAuto.u16EVBias = value;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

//printf("set_picture_ae_auto_EVBias ok\n");
}

void set_backlight_compensation(v_u32 level)
{
#ifdef _PRINTF
	dpf("set_backlight_compensation = %d\n",level);
	
#endif
	
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_AUTO;

	if (level == 0 )
	{
		stExpAttr.stAuto.enAEStrategyMode = AE_EXP_HIGHLIGHT_PRIOR;

		stExpAttr.stAuto.u16HistRatioSlope = 0x80;
		stExpAttr.stAuto.u8MaxHistOffset = 0x10;
	}
	else
	{
		stExpAttr.stAuto.enAEStrategyMode = AE_EXP_LOWLIGHT_PRIOR;

		stExpAttr.stAuto.u16HistRatioSlope = level;
		stExpAttr.stAuto.u8MaxHistOffset = BACKLIGHT_MAXHIST;
	}

	stExpAttr.stAuto.u8Speed = 0x40;

	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
}

void set_slowframe_rate( int fps_vi, int fps )
{
	
#ifdef _PRINTF
	sstw_info( "set slowframe rate = %d", fps );

#endif
				
	if (fps_vi == 0)
	{
		fps_vi = VI_FPS;

		//if( get_video_norm() == VIDEO_NORM_NTSC )		fps_vi = 30;
		//
		//if (GV_IPCAM_HI3516_SONY122 == ad_type )		fps_vi = 30;

	}
	//fps = fps_vi/(fps_rate>>4);  --> fps = (vi_fps << 4) / fps_rate;
	int fps_rate = (fps_vi << 4) / fps;	//0x14: set vi to 20 fps
	
	ISP_PUB_ATTR_S stPubAttr;
	ISP_DEV IspDev = 0;

	HI_MPI_ISP_GetPubAttr(IspDev, &stPubAttr);
	stPubAttr.f32FrameRate = fps;

	HI_MPI_ISP_SetPubAttr(IspDev, &stPubAttr);	
}

void set_picture_expo_time_mamu( v_u32 level )
{	
	int fps = 1000*1000 / level;
	if (fps > 30)
		fps = 30;
	
	set_slowframe_rate(30, fps);
#ifdef _PRINTF
	dpf("set_picture_expo_time_mamu = %d\n",level);

#endif
			
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_MANUAL;

	stExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
	stExpAttr.stManual.u32ExpTime = level;
	
	stExpAttr.stAuto.stExpTimeRange.u32Max = level;
	stExpAttr.stAuto.stExpTimeRange.u32Min = 10;


	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
}

void set_picture_a_gain( v_u32 level )
{	
#ifdef _PRINTF
	dpf("set_picture_a_gain = %d\n",level);
#endif

	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_MANUAL;

	stExpAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
	stExpAttr.stManual.u32AGain = 0x400+level;
	
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
}

void set_picture_d_gain( v_u32 level )
{
#ifdef _PRINTF
	dpf("set_picture_d_gain = %d\n",level);
	
#endif
			
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_MANUAL;

	stExpAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
	stExpAttr.stManual.u32DGain = 0x400+level;
	
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
}

void set_picture_isp_gain( v_u32 level )
{
#ifdef _PRINTF
	dpf("set_picture_isp_gain = %d\n",level);

#endif
			
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_MANUAL;

	stExpAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
	stExpAttr.stManual.u32ISPDGain = 0x400+level;
	
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
}
void set_picture_3d_nf_level( v_u32 level )
{	
#ifdef _PRINTF
	dpf("set_picture_3d_nf_level = %d\n",level);

#endif
		
	VPSS_GRP_PARAM_S stVpssParam;
	HI_S32 s32Ret = HI_MPI_VPSS_GetGrpParam(g_VpssGrp, &stVpssParam);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VPSS_GetGrpParam failed!, return 0x%x\n", s32Ret);
		return s32Ret;
	}
	stVpssParam.s32GlobalStrength = level;
	if (stVpssParam.s32GlobalStrength < 0 || stVpssParam.s32GlobalStrength > 1408) {
		printf("the 3DNR strength is out of the range\n");
		return -1;
	}
	
	s32Ret = HI_MPI_VPSS_SetGrpParam(g_VpssGrp, &stVpssParam);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VPSS_SetGrpParam failed!\n");
		return s32Ret;
	}
}

void set_picture_exposure_type( int type )
{
#ifdef _PRINTF
	dpf("set_picture_exposure_type = %d\n",type);
		
#endif
	
	if (type != 0 && type != 1)
		return ;
	
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);

	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = type;

	/*if (type == OP_TYPE_MANUAL)
	{
		stExpAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enISPAGainOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enISPISPDGainOpType = OP_TYPE_MANUAL;	
		stExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;

		stExpAttr.stManual.u32ExpTime = g_pSysInfo->exposure_time_manu;
		stExpAttr.stManual.u32ISPDGain = 0x400+g_pSysInfo->isp_gain;
		stExpAttr.stManual.u32AGain = 0x400+g_pSysInfo->a_gain;
		stExpAttr.stManual.u32DGain = 0x400+g_pSysInfo->d_gain;
	}
	else
	{
	}*/
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	
	if (type == OP_TYPE_AUTO)
		set_exposure_time(g_pSysInfo->exposure_time);
	else
		set_picture_expo_time_mamu(g_pSysInfo->exposure_time_manu);

	//if (type == OP_TYPE_MANUAL)
	//{
	//	set_picture_a_gain(g_pSysInfo->a_gain);
	//}
	//else
	//{
	//	set_picture_exposure_compensation( g_pSysInfo->exposure_compensation );
	//}
}

int get_stream_size(int channel)
{
	if (channel != STREAM_MAIN && channel != STREAM_SUB && channel != STREAM_SUB2)
		return -1;
	
	int pic_size;
	int en_size = PIC_HD720;

	//must be true
	pic_size = get_stream_res_sel(channel);

	switch (pic_size)
	{
	case GV_PIC_XGA:
		en_size = PIC_XGA;
		break;
	case GV_PIC_HD1080:
		en_size = PIC_HD1080;
		break;
	case GV_PIC_SXGA:
		en_size = PIC_SXGA;
		break;
	case GV_PIC_VGA:
		en_size = PIC_VGA;
		break;
	case GV_PIC_QVGA:
		en_size = PIC_QVGA;
		break;
		
	case GV_PIC_UXGA:
		en_size = PIC_UXGA;
		break;
		
	case GV_PIC_WUXGA:   //spring_bt1120
		en_size = PIC_WUXGA;
		break;
	case GV_PIC_WSXGA:   //spring_bt1120
		en_size = PIC_WSXGA;
		break;

#ifdef hi3516a
	case GV_PIC_2592x1520:
		en_size = PIC_2592x1520;
		break;
#endif
	default:
		en_size = PIC_HD720;
		break;
	}

	return en_size;
}


