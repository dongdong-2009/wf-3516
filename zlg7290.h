#ifndef ZLG7290_H
#define ZLG7290_H
#ifdef __cplusplus
extern "C" {
#endif
  int zlg7290Init(void);
  unsigned char  zlg7290Read(void);
  unsigned char zlg7290check(void);
#ifdef __cplusplus
}
#endif
#endif
