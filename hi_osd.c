
#include "stdafx.h"
#include "video_config.h"
#include "loadbmp.h"

/******************************************************************************
* funciton : OSD
******************************************************************************/
typedef struct
{
	char *littleHZK;
	char *bigHZK;
	int locations[32];
	int num;
} hzkDatabase;

#define START_POINT_X_OFFSET 0
#define START_POINT_Y_OFFSET 0

#define NO_SIGNAL_W		460
#define NO_SIGNAL_H		100
#define NO_SIGNAL_BMP	"./no_signal.bmp"

RGN_HANDLE RgnHandle;
#define OSD_LEN 128
char osd_str[OSD_LEN];
BITMAP_S stBitmap;
int pstBitmap_pData_len[2];
extern SysInfo * g_pSysInfo;

int have_signal_input = 1;

void is_have_signal(char* data)
{
	sstw_error("get msg: %s", data);

	if (0 == strncmp(data, "yes", 3))
		have_signal_input = 1;
	else
		have_signal_input = 0;
}

/******************************************************************************
* funciton : load bmp from file
******************************************************************************/
HI_S32 SAMPLE_RGN_LoadBmp(const char *filename, BITMAP_S *pstBitmap, HI_BOOL bFil, HI_U32 u16FilColor)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;
    HI_U32 u32BytePerPix = 0;
    
    if(GetBmpInfo(filename, &bmpFileHeader, &bmpInfo) < 0)
    {
		printf("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
    u32BytePerPix      = 2;
    
    pstBitmap->pData = malloc(u32BytePerPix * (bmpInfo.bmiHeader.biWidth) * (bmpInfo.bmiHeader.biHeight));
	
    if(NULL == pstBitmap->pData)
    {
        printf("malloc osd memroy err!\n");        
        return HI_FAILURE;
    }
    
    CreateSurfaceByBitMap(filename, &Surface, (HI_U8*)(pstBitmap->pData));
	
    pstBitmap->u32Width      = Surface.u16Width;
    pstBitmap->u32Height     = Surface.u16Height;
    pstBitmap->enPixelFormat = PIXEL_FORMAT_RGB_1555;
    
    
    int i,j;
    HI_U16 *pu16Temp;
    pu16Temp = (HI_U16*)pstBitmap->pData;
    
    if (bFil)
    {
        for (i=0; i<pstBitmap->u32Height; i++)
        {
            for (j=0; j<pstBitmap->u32Width; j++)
            {
                if (u16FilColor == *pu16Temp)
                {
                    *pu16Temp &= 0x7FFF;
                }

                pu16Temp++;
            }
        }

    }
        
    return HI_SUCCESS;
}

/**
* 初始化字库littleFontDatabase，bigFontDatabase，hzkFontDatabase
* 其中littleFontDatabase，bigFontDatabase分别是asc的小字体和大字体，存储的字模为' '到'~'
*/
HI_S32 initFontDatabase()
{	
	//load bmp	
	if( HI_SUCCESS != SAMPLE_RGN_LoadBmp(NO_SIGNAL_BMP, &stBitmap, HI_FALSE, 0))
	{
		sstw_error("load no signal bmp failed\n");
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

HI_S32 create_Overlay_Regions( int rgn_len, int rng_height )
{
	HI_S32 s32Ret = HI_FAILURE;
	RGN_ATTR_S stRgnAttr;
	/****************************************
	step 1: create overlay regions
	****************************************/
	//no_signal
	stRgnAttr.enType = OVERLAYEX_RGN;

	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stSize.u32Width = NO_SIGNAL_W;//1024;//1024;//640;//16;//640;/*域宽 */		
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = NO_SIGNAL_H;//640;//32;/*域高 */	//spring_osd   2行
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x1f;
	RgnHandle = 0;///*域操作句柄*/

	s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	
	return HI_SUCCESS;
}

HI_S32 regions_AttachToChn()
{
	HI_S32 s32Ret = HI_FAILURE;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	
	v_u32 w = 0 ,h = 0;
	get_stream_res(STREAM_MAIN, &w, &h);
	int x = (VO_WIDTH - NO_SIGNAL_W)/2;
	int y = (VO_HEIGHT - NO_SIGNAL_H)/2;

	//no_signal in main stream
	stChn.enModId = HI_ID_VPSS;//HI_ID_VENC;//HI_ID_GROUP;
	stChn.s32DevId = 0;
	stChn.s32ChnId = VPSS_CHN_VO;	//0;  //VPSS_CHN_VO;  spring

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow = HI_TRUE;
	stChnAttr.enType = OVERLAYEX_RGN;
	stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = x;	// START_POINT_X_OFFSET;//通道画面中显示的位置（x）
	stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = y;	//START_POINT_Y_OFFSET;//通道画面中显示的位置（Y）
	stChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = 255;
	stChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 255;
	stChnAttr.unChnAttr.stOverlayExChn.u32Layer = 0;
	
	s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret) {
		sstw_error("HI_MPI_RGN_AttachToChn (RgnHandle:%d, chnid:%d) failed with %#x!",RgnHandle,stChn.s32ChnId , s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
	if (s32Ret != HI_SUCCESS) {
		sstw_error("HI_MPI_RGN_SetBitMap (RgnHandle:%d) failed with %#x!",RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 regions_UpdateBitmap()
{

	//if (cc > 10)
	{		
	}

	return HI_SUCCESS;
}

HI_S32 regions_DetachToChn()
{
	MPP_CHN_S stChn;
	HI_S32 s32Ret = HI_FAILURE;
	stChn.enModId = HI_ID_GROUP;
	stChn.s32DevId = 0;
	stChn.s32ChnId = VPSS_CHN_VO;//VPSS_CHN_VO;

	stChn.enModId = HI_ID_VPSS;//HI_ID_VENC;//HI_ID_GROUP;

	s32Ret = HI_MPI_RGN_DetachFromChn(RgnHandle, &stChn);

	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_DetachFrmChn (RgnHandle:%d, chnid:0) failed with %#x!\n",RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	
	//printf("Detach regions from channels success\n");
	return HI_SUCCESS;
}

HI_S32 regions_Distroy()
{
	HI_S32 s32Ret = HI_FAILURE;

	s32Ret = HI_MPI_RGN_Destroy(RgnHandle);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_Destroy [%d] failed with %#x\n", RgnHandle,
			s32Ret);
	}

	return HI_SUCCESS;
}

HI_S32 show_regions()
{
	HI_S32 s32Ret = HI_FAILURE;
	int attachFlag = 0;

	s32Ret = create_Overlay_Regions(1024, 32*2);   //2 lines
	if(HI_SUCCESS != s32Ret)
	{
		printf("[regions_Process]:create_Overlay_Regions failed !\n");
		return HI_FAILURE;
	}

	if(attachFlag == 0)
	{

		if(regions_AttachToChn()!= HI_SUCCESS)
		{
			printf("[regions_Process]:regions_AttachToChn failed !\n");
			return;
		}

		attachFlag = 1;
	}

	show_no_signal(!have_signal_input);

}


void distory_regions()
{
	HI_S32 s32Ret = HI_FAILURE;

	s32Ret = regions_DetachToChn();
	regions_Distroy();
	free(stBitmap.pData);
}


void show_no_signal(bool show)
{		
	RGN_CHN_ATTR_S stChnAttr;
	MPP_CHN_S stChn;
	HI_S32 s32Ret = HI_SUCCESS;

	//for (i = 0; i < 2; i++)
	{
		stChn.enModId = HI_ID_VPSS;//HI_ID_VENC;//HI_ID_GROUP;
		stChn.s32DevId = 0;
		stChn.s32ChnId = VPSS_CHN_VO;//VPSS_CHN_VO;
		
		s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
		if(s32Ret != HI_SUCCESS)
		{
			sstw_error("HI_MPI_RGN_GetDisplayAttr failed(, handle=%d) err=0x%x", RgnHandle, s32Ret);
			return;
		}
			
		//sstw_error("************ HI_MPI_RGN_GetDisplayAttr ");

		stChnAttr.bShow = show;

		s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
		if(s32Ret != HI_SUCCESS)
		{		
			sstw_error("HI_MPI_RGN_SetDisplayAttr failed(cahnn, handle=%d) err=0x%x", RgnHandle, s32Ret);
			return;
		}
	}
}