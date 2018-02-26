
#ifndef _IVE_MAIN_H
#define _IVE_MAIN_H

#include "stdafx.h"

#include "hi_comm_ive.h"
#include "mpi_ive.h"

#ifdef hi3516a
void detectFun(void);
HI_S32 ive_frame_init(IVE_SRC_IMAGE_S * stFrameInfo, HI_U16 u16Width, HI_U16 u16Height);
void* thread_key_control();

void init4200();
void initIO();
#endif

#endif
