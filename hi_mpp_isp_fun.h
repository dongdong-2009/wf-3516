
#ifndef HI_MPP_3518_H
#define	HI_MPP_3518_H

#include "stdafx.h"

int  get_sensor_iso();
void set_sensor_mode();
HI_S32 set_3d_denoise();

void dynamic_defect_pixel( int enable );
HI_S32 set_max_dgain();
HI_S32 SAMPLE_IQ_SetGammaTable(HI_U8 u8GammaTabNum);

#endif