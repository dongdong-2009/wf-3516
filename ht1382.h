#ifndef ZLG7290_H
#define ZLG7290_H
#ifdef __cplusplus
extern "C" {
#endif
  typedef struct {
	unsigned char year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
  }Timer;
  
  int ht1382Init(void);
  void setTime(Timer *t);
  void getTime(char* timer);
  void setId(char * id);
  void getId(char * id);
#ifdef __cplusplus
}
#endif
#endif
