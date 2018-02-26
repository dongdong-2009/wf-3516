#include "hi_mpp_isp_fun.h"
#include "video_config.h"

extern int VI_FPS;
extern VPSS_GRP g_VpssGrp ;
extern int ad_type;
#define EXPOSURE_GAIN_SHIFT 10


int get_sensor_iso()
{
	//if (GV_IPCAM_HI3518_OV9712P != ad_type && GV_IPCAM_HI3518_OV9712 != ad_type )	return -1;		//9712p is tested.
	static int iso_pre = 0;
	ISP_DEV dev = 0;
    ISP_EXP_INFO_S isp_info;
	 
	HI_S32 s32Ret = HI_MPI_ISP_QueryExposureInfo(dev, &isp_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_ISP_QueryExposureInfo failed!\n");
        return -1;
    }
	
	HI_U32 u32ISO = ((HI_U64) isp_info.u32AGain * isp_info.u32DGain
             * isp_info.u32ISPDGain * 100) >> (EXPOSURE_GAIN_SHIFT * 3);

#ifdef IV_DEBUG	
	//int debug = zs_get_cfg_int("MAXGAIN","debug", 0, IPC_CFG_FILE );
	//if (debug)
	{
		if (iso_pre != u32ISO)
		{			
			printf( "# analog gain=%d, dig gain=%d, isp dig gain=%d, iso=%d, exptime=%d\n", 
				isp_info.u32AGain, isp_info.u32DGain, isp_info.u32ISPDGain, u32ISO, isp_info.u32ExpTime );
		}
	}
	iso_pre = u32ISO;
#endif

	return u32ISO;
}

HI_S32 set_3d_denoise()
{	
    VPSS_GRP_PARAM_S stVpssParam;

    HI_S32 s32Ret = HI_MPI_VPSS_GetGrpParam(g_VpssGrp, &stVpssParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_GetGrpParam failed!, return 0x%x\n", s32Ret);
        return s32Ret;
    }

	int iso = get_sensor_iso();
	if (iso < 0)
    {
        SAMPLE_PRT("get_sensor_iso failed!\n");
        return s32Ret;
    }

	/*printf("s32IeStrength = %d, s32YSFStrength= %d,s32YTFStrength;=%d,s32CSFStrength= %d,s32CTFStrength=%d,s32MotionLimen =%d  \n",
		stVpssParam.s32IeStrength,stVpssParam.s32YSFStrength, stVpssParam.s32YTFStrength,stVpssParam.s32CSFStrength,
		stVpssParam.s32CTFStrength,stVpssParam.s32MotionLimen);*/

	//if (ad_type == GV_IPCAM_HI3516_SONY122)
	{
		if( iso <= 100 )	
		{		
			//stVpssParam.u32SfStrength =	stVpssParam.u32TfStrength = stVpssParam.u32ChromaRange = 0x5;
			stVpssParam.s32GlobalStrength = 62;
		}
		else if( iso <= 200  )
		{
			//stVpssParam.u32SfStrength =	stVpssParam.u32TfStrength = stVpssParam.u32ChromaRange = 0x5;
			stVpssParam.s32GlobalStrength = 65;
		}
		else if( iso < 400  )
		{/*
			stVpssParam.u32SfStrength =	0x6;
			stVpssParam.u32TfStrength = stVpssParam.u32ChromaRange = 0x6;*/
			stVpssParam.s32GlobalStrength = 86;
		}
		else if( iso < 800 )	
		{/*
			stVpssParam.u32SfStrength =	0xe;
			stVpssParam.u32TfStrength = stVpssParam.u32ChromaRange = 0x7;*/
			stVpssParam.s32GlobalStrength = 155;
		}
		else if( iso < 1600 )	
		{
		/*	stVpssParam.u32SfStrength =	0x28;
			stVpssParam.u32TfStrength = 0xa;
			stVpssParam.u32ChromaRange = 0xa;*/
			stVpssParam.s32GlobalStrength = 202;
		}
		else if( iso < 3200 )	
		{
	/*		stVpssParam.u32SfStrength = 0x40;
			stVpssParam.u32TfStrength = 0x10;
			stVpssParam.u32ChromaRange = 0x20;*/
			stVpssParam.s32GlobalStrength = 339;
		}
		else if( iso < 6400 )	
		{
			/*stVpssParam.u32SfStrength = stVpssParam.u32TfStrength * 4;
			stVpssParam.u32TfStrength = 0x14;
			stVpssParam.u32ChromaRange = stVpssParam.u32TfStrength*2;*/
			stVpssParam.s32GlobalStrength = 665;
		}	
		else					
		{
			/*stVpssParam.u32SfStrength = stVpssParam.u32TfStrength * 4;
			stVpssParam.u32TfStrength = 0x1c;
			stVpssParam.u32ChromaRange = stVpssParam.u32TfStrength*2;*/
			stVpssParam.s32GlobalStrength = 830;
		}
	}
		
	/*int tf = zs_get_cfg_int("MAXGAIN","tf", 0, IPC_CFG_FILE );
	if( tf != 0 )
	{
		stVpssParam.u32TfStrength = tf;
		stVpssParam.u32ChromaRange = tf*2;
		stVpssParam.u32SfStrength = tf*4;
	}*/
	//
	
	//val += 10;
	s32Ret = HI_MPI_VPSS_SetGrpParam(g_VpssGrp, &stVpssParam);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VPSS_SetGrpParam failed!\n");
	}
}

HI_S32 SAMPLE_IQ_SetGammaTable(HI_U8 u8GammaTabNum)
{
	return 0;
}

