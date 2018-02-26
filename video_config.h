
#ifndef _video_config_h
#define _video_config_h

#include "stdafx.h" 

#define GV_FAILURE	0
#define GV_SUCCESS	1

#define GV_REBOOT	0X00001000

#define SIO0_WORK_MODE   AIO_MODE_I2S_MASTER
#define AUDIO_POINT_NUM 320             /* point num of one frame 80,160,320,480,1024,2048*/

#define TLV320_FILE2    "/dev/misc/tlv320aic31"
#define AUDIO_PAYLOAD_TYPE PT_ADPCMA// PT_AAC //PT_G726//   /* encoder type, PT_ADPCMA,PT_G711A,PT_AAC...*/

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_IMA /* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/


#ifdef hi3518e
#define MAX_FRAME_RATE 18
#else
#define MAX_FRAME_RATE 30
#endif


#ifndef GM812X
typedef struct md_cfg
{
	int alarm_th;
	int sad_offset;
};

#endif

typedef enum _sensor_mode
{
	SM_NORMAL = 0,
	SM_NORMAL2,
	SM_FRAME_SLOW,
	SM_FRAME_5FPS,
	SM_FRAME_10FPS,
	SM_FRAME_15FPS,
	SM_FRAME_20FPS,
	SM_FRAME_25FPS,
	SM_FRAME_SLOW_EXTREME,
}sensor_mode_t;

int init_av_config();
void UnInitSysInfo();
void InitStreamAttr();
HI_S32 set_stream_res(VENC_GRP VeG, VENC_CHN VeC, resolution_t *res);
HI_BOOL get_stream_res( VENC_CHN VeC, v_u32 *width, v_u32 *height );
int get_stream_res_sel( VENC_CHN VeC );
HI_S32 ConfVideoColor_save(void *cc);
HI_S32 get_picture_color(COLOR_CONFG* cc);
HI_S32 ConfVideoColor_set_default();
HI_S32 set_stream_attr( VENC_GRP VeG, VENC_CHN VeC, void/*struct CONFGAV*/* configValue );

HI_S32 open_video_decode();
HI_VOID close_video_decode();
void init_video_decode();

HI_S32 set_color2grey( int b_color );
HI_S32 set_noise_level( int noise, int b_color );
HI_S32 set_picture_exposure_compensation( int level );
int set_picture_mirror( int b_mirror );
int set_picture_flip( int b_flip );
int get_picture_flip();
int get_picture_mirror();
HI_S32 set_3d_denoise();
VIDEO_NORM_E get_video_norm();
int get_stream_size(int channel);
void set_picture_sharpen( int sharpen );
void cal_color_value(COLOR_CONFG* cc);
void set_exposure_time(v_u32 time);
void set_backlight_compensation(v_u32 level);
void set_vi_csc_luma(int offset);
HI_S32 set_vo_picture_rotate( ROTATE_E rotate);
void set_picture_exposure_type( int type );
void set_picture_expo_time_mamu( v_u32 level );
void set_picture_a_gain( v_u32 level );
void set_picture_d_gain( v_u32 level );
void set_picture_isp_gain( v_u32 level );

//steven 2017-01-19
void set_picture_ae_auto_speed(v_u32 value);
void set_picture_ae_auto_BlackSpeedBias(v_u32 value);
void set_picture_ae_auto_Tolerance(v_u32 value);
void set_picture_ae_auto_EVBias(v_u32 value);

#endif

