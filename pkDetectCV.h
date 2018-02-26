#ifndef __pkDetectCV__
#define __pkDetectCV__

#ifdef __cplusplus
extern "C" {
#endif


	
	/// coordinate position of the card
	typedef struct _targetCoordinate{
		int x;
		int y;
	}targetCoordinate;



	void pkDetectCvInit(/*DETECT_PARAMETERS *param*/);
	void pkDetectCvUnInit();
	targetCoordinate pkDetectCvFun(unsigned char *Image_data_ptr);
	unsigned char* pkDetectCvFunReturnRoi(unsigned char *Image_data_ptr, bool detect_flag);
	unsigned char* keyShowNumberByCVBS(char *text_number);
	unsigned char* activateFailureNum(char *text_number);
	unsigned char* activateSuccess(char *text_number);
	unsigned char* welcomeTestUserProduct(char *text_number);
	unsigned char* welcomeUserProduct(char *text_number);


	unsigned char pkReturnAvrGrayWhite();
	unsigned char pkReturnAvrGrayBlack();
	void pkReturnAvrGray(unsigned char *avr_gray_value, unsigned char *white_black_ratio_value, unsigned char *Image_data_ptr);

	unsigned char randNum();
	void generateActivateCode(char* serial_code, int* activate_code);

	void setParamSaveImage(bool save_image_flag);
	void setParamCameraWH(int width, int height, bool imageFlip);
	void setParamBackColorAbs(int backColorAbs);
	void setParamCurrentBulkThreshold(int currentBulkThreshold );
	void setParamThresbulknum(int thresbulknum);
	void setParamRandNum(int randData);
	void setParamImageFlip(bool ImageFlip);
	void setParamScale(float scale);
	void setParamRoiRectWH(int roiRectW, int roiRectH);
	void setParamCvbsWH(int cvbsW, int cvbsH);

	//void autoAdjustCammerParam(int *cur_brightness);
	int autoAdjustCammerParam(unsigned char *cur_brightness, int *cur_shutter, unsigned char *Image_data_ptr);
	void setParamAutoAdjustCameraTargetShutterThread(int no_target_shutter, int shutter_value_upper, int shutter_value_lower);
	void setParamAutoAdjustCameraTargetBrThread(int br_value_upper, int br_value_lower);
	void setParamAutoAdjustCameraTargetBrLMH(unsigned char targetBr, unsigned char noTargetBrH, unsigned char noTargetBrL);



#ifdef __cplusplus
}
#endif

#endif
