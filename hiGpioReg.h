#ifndef __HIGPIOREG_H__
#define __HIGPIOREG_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define GPIO_INPUT     0
#define GPIO_OUTPUT    1

extern int gpioClr(unsigned char gpioBank, unsigned char gpioBit);
extern int gpioGet(unsigned char gpioBank, unsigned char gpioBit);
extern int gpioSet(unsigned char gpioBank, unsigned char gpioBit);
extern int gpioSetMode(unsigned char gpioBank, unsigned char gpioBit
             , unsigned char gpioDir, unsigned char gpioValue);
extern int reg_read(unsigned int arg, unsigned int *regvalue);
extern int reg_write(unsigned int arg, unsigned int regvalue);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HIGPIOREG_H__ */
