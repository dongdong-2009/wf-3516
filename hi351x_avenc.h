
#ifndef _HI351X_AVENC_H
#define _HI351X_AVENC_H

#include "stdafx.h"


#define VPSS_CHN_MAIN	0
//#define VPSS_CHN_SUB	1
#define VPSS_CHN_VO		1
#define VPSS_VO_EXT_CHN	4
#define VPSS_EXT_CHN2	5

typedef struct tagSAMPLE_ADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
    FILE *pfd;
    pthread_t stAdPid;
} SAMPLE_ADEC_S;

typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;

typedef struct hiVDA_MD_PARAM_S
{
    HI_BOOL bThreadStart;
    VDA_CHN VdaChn;
} VDA_MD_PARAM_S;

typedef struct tagSAMPLE_VENC_PARA_S
{
     HI_BOOL bThreadStart;
	 pthread_t tid_venc;
     HI_S32  s32Cnt;
} SAMPLE_VENC_PARA_S;


#ifdef _VO_
int set_vpss_chncrop(VPSS_GRP VpssGrp,VPSS_CHN VpssChn, v_u32 x, v_u32 y);
#endif

#endif

