/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */
	
#include "stdafx.h"
#include "video_config.h"
#include "ive_main.h"
#include "util.h"

extern int ad_type;
extern v_u8 MotionBlock[160][160];
extern struct md_cfg md_param;

VPSS_GRP g_VpssGrp = 0;
extern SysInfo * g_pSysInfo;

int set_vpss_chncrop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, v_u32 x, v_u32 y)
{
	v_u32 w,h;
	VPSS_CHN_MODE_S stVpssChnMode;
	int vo_w = g_pSysInfo->size_vo.uWidth;
	int vo_h = g_pSysInfo->size_vo.uHeight;
		
//steven 2017-01-13	sstw_info("vo_w = %d, vo_h = %d", vo_w, vo_h);
// dpf("vo_w = %d, vo_h = %d\n", vo_w, vo_h);//hui 2017-4-9
#ifdef _PRINTF
	dpf("detect : x = %d, y = %d\n", x, y);  //steven 2017-01-12	
#endif


	get_stream_res( STREAM_MAIN, &w, &h );

	// dpf("STREAM_MAIN: w = %d, h = %d\n", w, h);
	if (x == 0 || y == 0)	//ȫ��
	{		
		vo_w = stVpssChnMode.u32Width       = VO_WIDTH;//stSize.u32Width;		//main
		vo_h = stVpssChnMode.u32Height      = VO_HEIGHT;//stSize.u32Height;	//main	
	}
	else
	{		
		stVpssChnMode.u32Width       = w;	//main
		stVpssChnMode.u32Height      = h;	//main
	}

	stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
	stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	stVpssChnMode.enCompressMode  = COMPRESS_MODE_NONE;
          
	HI_S32 s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn, &stVpssChnMode);
	if (s32Ret != HI_SUCCESS)
	{
            sstw_error("HI_MPI_VPSS_SetChnMode  failed!(0x%x)", s32Ret);
	    return HI_FAILURE;
	}     


	int start_x = 0;
	int start_y = 0;

	int tmp_x = x - vo_w/2;
	int tmp_y = y - vo_h/2;


	start_x = tmp_x > 0 ? tmp_x : 0;
	start_y = tmp_y > 0 ? tmp_y : 0;
	
	if(start_x + vo_w > w)
	{
	     start_x = w - vo_w;
	}
	if(start_y + vo_h > h)
	{
		 start_y = h -vo_h;
	}
	
	start_x &= 0xfffffffe;
	start_y &= 0xfffffffe;
	
	// dpf("crop : start_x = %d, start_y = %d\n", start_x, start_y);

	VPSS_CROP_INFO_S stCropInfo;
	stCropInfo.bEnable = true;
	stCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
	stCropInfo.stCropRect.s32X = start_x;
	stCropInfo.stCropRect.s32Y = start_y;
	stCropInfo.stCropRect.u32Width = vo_w;
	stCropInfo.stCropRect.u32Height = vo_h;
	
	s32Ret = HI_MPI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stCropInfo);
	if (s32Ret != HI_SUCCESS)
	{
		sstw_error("HI_MPI_VPSS_SetChnCrop (%d):failed with %#x! rect[%d,%d,%d,%d]",	__LINE__,  s32Ret, 
			start_x, start_y, vo_w, vo_h);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

void start_vo( VPSS_GRP VpssGrp,VPSS_CHN VpssChn )
{ 			
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;;
    VO_CHN VoChn = 0;    
    VO_PUB_ATTR_S stVoPubAttr;
    //SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;
	VO_LAYER VoLayer = 0;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	HI_S32		s32Ret = 0;
#ifdef _PRINTF
	sstw_info("open vo channel begain");	
#endif	
	SAMPLE_COMM_VO_StopLayer(VoLayer);
	SAMPLE_COMM_VO_StopDev(VoDev);

	/******************************************
    step 7: start VO SD0 (bind * vi )
    ******************************************/
    stVoPubAttr.enIntfType = VO_INTF_CVBS;
    stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;    
    stVoPubAttr.u32BgColor = 0x000000ff;

    /* In HD, this item should be set to HI_FALSE */
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if (HI_SUCCESS != s32Ret)
    {
       sstw_error("SAMPLE_COMM_VO_StartDev failed!");
	   return;
    }

    stLayerAttr.bClusterMode = HI_FALSE;
	stLayerAttr.bDoubleFrame = HI_FALSE;
	stLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

	stLayerAttr.stDispRect.s32X = 0;
	stLayerAttr.stDispRect.s32Y = 0;

    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, 
                            &stLayerAttr.stDispRect.u32Width, &stLayerAttr.stDispRect.u32Height,
                            &stLayerAttr.u32DispFrmRt);
    if (HI_SUCCESS != s32Ret)
    {
       sstw_error("SAMPLE_COMM_VO_GetWH failed!");
	   return;
    }     

	
    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;//2592;//stLayerAttr.stDispRect.u32Width;		
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;//1520;//stLayerAttr.stDispRect.u32Height;
#ifdef _PRINTF
	printf("startvo vo: x = %d, y = %d\n", stLayerAttr.stImageSize.u32Width,stLayerAttr.stImageSize.u32Height);
#endif
	

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr, HI_TRUE);
    if (HI_SUCCESS != s32Ret)
    {
       sstw_error("SAMPLE_COMM_VO_StartChn failed!");
	   return;
    }
 
	SAMPLE_COMM_VO_StopChn(VoLayer, VO_MODE_1MUX);

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, VO_MODE_1MUX);
    if (HI_SUCCESS != s32Ret)
    {
       sstw_error("SAMPLE_COMM_VO_StartChn failed!");
	   return;
    }


    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev, VoChn, VpssGrp, VpssChn);
    //s32Ret = SAMPLE_COMM_VO_BindVi(VoDev, VoChn, ViChn);
    if (HI_SUCCESS != s32Ret)
    {
       sstw_error("SAMPLE_COMM_VO_BindVpss(vo:%d)-(VpssChn:%d) failed with %#x!", VoDev, VoChn, s32Ret);
	   return;
    }
	    			
	set_vpss_chncrop(g_VpssGrp,VPSS_CHN_VO, 0, 0);
#ifdef _PRINTF
	sstw_info("open vo channel success");	
#endif
	
}


/******************************************************************************
* function :  H.264@720p@30fps+H.264@VGA@30fps+H.264@QVGA@30fps
******************************************************************************/
HI_S32 SAMPLE_VENC_720P_CLASSIC(HI_VOID)
{
	PAYLOAD_TYPE_E enPayLoad[3]= {PT_H264, PT_H264, PT_H264};//PT_H264};
    //PIC_XGA,     /* 1024 * 768 */   
    PIC_SIZE_E enSize[3] = {PIC_HD720, PIC_VGA, PIC_QVGA};

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig;
    
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
	SAMPLE_RC_E enRcMode[3] = {SAMPLE_RC_VBR,SAMPLE_RC_VBR,SAMPLE_RC_VBR};

	int u32Profile = 0;
    HI_S32  s32ChnNum = 0;   //4:  for snap
	int i = 0;
	ROTATE_E enRotate = g_pSysInfo->m_vo_rotate;//ROTATE_NONE;//ROTATE_270;
	
    HI_S32 s32Ret = HI_SUCCESS;

	s32Ret = initFontDatabase();	
	if (s32Ret != HI_SUCCESS)
	{
		sstw_error("initfontdatabase failure");
	}

	VIDEO_NORM_E enNorm = get_video_norm();
    
    HI_U32 u32BlkSize;
    SIZE_S stSize;

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
	
	enSize[0] = get_stream_size(STREAM_MAIN);
	enSize[1] = get_stream_size(STREAM_SUB);
	
    stVbConf.u32MaxPoolCnt = 128;
    /*video buffer*/   
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(enNorm,\
                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;

	stVbConf.astCommPool[0].u32BlkCnt = 6;

	//if( APTINA_MT9P006_DC_1080P_30FPS == SENSOR_TYPE )	//for 1G ddr on 200M ipc
	//	stVbConf.astCommPool[0].u32BlkCnt = 8;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(enNorm,\
                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 5;
		

	// strream 3
#ifdef _VO_
	u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(enNorm,\
		enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;

	stVbConf.astCommPool[2].u32BlkCnt = 6;

#endif
		  
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("SAMPLE_COMM_SYS_GetPicSize failed!");
//         goto END_VENC_720P_CLASSIC_1;
		return s32Ret;
    }

	if (ROTATE_90 == enRotate || ROTATE_270 == enRotate)
        {
            u32BlkSize = (CEILING_2_POWER(stSize.u32Width, SAMPLE_SYS_ALIGN_WIDTH) * \
                          CEILING_2_POWER(stSize.u32Height, SAMPLE_SYS_ALIGN_WIDTH) * \
                          ((PIXEL_FORMAT_YUV_SEMIPLANAR_422 == SAMPLE_PIXEL_FORMAT) ? 2 : 1.5));
            stVbConf.astCommPool[3].u32BlkSize = u32BlkSize;
            stVbConf.astCommPool[3].u32BlkCnt = 20;
        }


    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("system init failed with %d!", s32Ret);
        //goto END_VENC_720P_CLASSIC_0;
		return s32Ret;
    }
	
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	
//steven 2016-9-12 �����Աȶȵ�ʱ����Ȳ��仯�� ������ȵ��ڷ�Χ�ı�
#if 1
    VI_MOD_PARAM_S stModParam;
    s32Ret = HI_MPI_VI_GetModParam( &stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_GetModParam failed\n");
        return HI_FAILURE;
    }
#ifdef _PRINTF
	printf("*******stModParam.bLumaExtendEn=%d\n", stModParam.bLumaExtendEn);
	printf("******stModParam.bContrastModeEn=%d\n", stModParam.bContrastModeEn);
#endif
    
    stModParam.bLumaExtendEn = 1;
    stModParam.bContrastModeEn = 1;
    s32Ret = HI_MPI_VI_SetModParam(&stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetModParam failed\n");
        return HI_FAILURE;
    }    
#endif
//steven 2016-9-12
	
	stViConfig.enViMode   = SENSOR_TYPE;	
#ifdef _PRINTF
	printf("vimode = %d\n", stViConfig.enViMode);
#endif
	
    stViConfig.enRotate   = g_pSysInfo->m_vi_rotate;//ROTATE_90;//ROTATE_NONE;
	//stViConfig.
    stViConfig.enNorm     = enNorm;  
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;

    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("SAMPLE_COMM_VI_StartVi failed!");
		return s32Ret;
    }
		
	v_u32 w = 640, h = 480;


    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
#ifdef _PRINTF
	dpf( "main steam: pic w=%d, h=%d\n", stSize.u32Width, stSize.u32Height );
#endif
	
		
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(g_VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("SAMPLE_COMM_VPSS_StartGroup failed!");
		return s32Ret;
    }
	
    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
		return s32Ret;
    }

	
	VpssChn = VPSS_CHN_MAIN;
	stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble        = HI_FALSE;
    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width       = stSize.u32Width;
    stVpssChnMode.u32Height      = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;

	//s32Ret = HI_MPI_VPSS_SetRotate(g_VpssGrp, VpssChn, ROTATE_90);

    s32Ret = SAMPLE_COMM_VPSS_EnableChn(g_VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("Enable vpss chn failed!");
		return s32Ret;
    }


#ifdef _VO_	
	    
	VpssChn = VPSS_CHN_VO;
	stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
	stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width       = stSize.u32Width;//main
    stVpssChnMode.u32Height      = stSize.u32Height;//main
	stVpssChnMode.enCompressMode  = COMPRESS_MODE_NONE;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
		
	s32Ret = HI_MPI_VPSS_SetRotate(g_VpssGrp, VpssChn, enRotate);

    s32Ret = SAMPLE_COMM_VPSS_EnableChn(g_VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("Enable vpss chn2 failed!");
		return s32Ret;
    }
	
#if 1
	VpssChn = VPSS_VO_EXT_CHN;
	VPSS_EXT_CHN_ATTR_S	stVpssExtChn;

	stVpssExtChn.s32BindChn = VPSS_CHN_VO;
    stVpssExtChn.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssExtChn.u32Width       = VO_WIDTH;//stSize.u32Width;		//main
    stVpssExtChn.u32Height      = VO_HEIGHT;//stSize.u32Height;	//main
	stVpssExtChn.enCompressMode  = COMPRESS_MODE_NONE;
    stVpssExtChn.s32SrcFrameRate = -1;
    stVpssExtChn.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(g_VpssGrp, VpssChn, &stVpssChnAttr, NULL, &stVpssExtChn);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("Enable vpss chn2 failed!");
		return s32Ret;
    }

	get_stream_res(STREAM_SUB, &w, &h);
#ifdef _PRINTF
	dpf( "vpss ext2: pic w=%d, h=%d\n", w, h );
#endif
	
	w = w & 0xfffffffe;
	h = h & 0xfffffffe;

	VpssChn = VPSS_EXT_CHN2;

	stVpssExtChn.s32BindChn = VPSS_CHN_MAIN;
    stVpssExtChn.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssExtChn.u32Width       = w;
    stVpssExtChn.u32Height      = h;
	stVpssExtChn.enCompressMode  = COMPRESS_MODE_NONE;
    stVpssExtChn.s32SrcFrameRate = -1;
    stVpssExtChn.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(g_VpssGrp, VpssChn, &stVpssChnAttr, NULL, &stVpssExtChn);
    if (HI_SUCCESS != s32Ret)
    {
        sstw_error("Enable vpss chn2 failed!");
		return s32Ret;
    }
#endif

#endif
	
#ifdef _VO_
	VpssChn = VPSS_VO_EXT_CHN;//VPSS_VO_EXT_CHN;
	start_vo(g_VpssGrp, VpssChn);
#endif
	
	/*int ret = 0;
	pthread_t tid_ive_task;
	ret = start_task(&tid_ive_task, thread_ive, (void*)NULL);
#ifdef _VO_
	VpssChn = VPSS_VO_EXT_CHN;//VPSS_VO_EXT_CHN;
	start_vo(g_VpssGrp, VpssChn);
#endif
	
	sleep(1);
		
	//SAMPLE_IQ_SetGammaTable(2);

	// set the user saved param;
	init_video_decode();
	*/

	return s32Ret;
}

//--------------------------------------------------
void cleanup_hi351x()
{
	VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;   
	VENC_GRP VencGrp;
    VENC_CHN VencChn;  

    SAMPLE_VI_CONFIG_S stViConfig; 
	stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
	
// END_VENC_720P_CLASSIC_5:
    VpssGrp = 0;
    
    VpssChn = 0;
    VencGrp = 0;   
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);

    SAMPLE_COMM_VENC_Stop(VencChn);

    VpssChn = 1;
    VencGrp = 1;   
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);

    SAMPLE_COMM_VENC_Stop(VencChn);
    VpssChn = 3;
    VencGrp = 2;   
    VencChn = 2;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
    SAMPLE_COMM_VI_UnBindVpss(SENSOR_TYPE);
	
// END_VENC_720P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
    VpssChn = 3;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
// END_VENC_720P_CLASSIC_3:    //vpss stop       
    SAMPLE_COMM_VI_UnBindVpss(SENSOR_TYPE);
// END_VENC_720P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
// END_VENC_720P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
// END_VENC_720P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return ;    
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
