//
// Created by noxue on 4/27/18.
//



#ifndef PHONE_UTILS_H
#define PHONE_UTILS_H

#include <pjsip.h>
#include <pjmedia.h>
#include <pjsip_ua.h>
#include <pjlib-util.h>
#include <pjlib.h>


int app_perror( const char *sender, const char *title,
                       pj_status_t status);



/**
 * G711 alaw ,ulaw 和 linear （声卡可以直接播放的波形采样) 的转换代码：

转换测试代码

void testAlaw()
{
 int st = 1209;
 unsigned char p = linear2alaw(st);
 st = alaw2linear(p);   //alaw转换为线性波形采样

 p = linear2alaw(-1209);
 st = alaw2linear(p);
}
 */

static int search(int val,short *table,int size);

unsigned char linear2alaw(int pcm_val);

int alaw2linear(unsigned char a_val);


unsigned char linear2ulaw(int pcm_val);


int ulaw2linear( unsigned char  u_val);


unsigned char alaw2ulaw(unsigned char   aval);

unsigned char ulaw2alaw(unsigned char   uval);



#endif //PHONE_UTILS_H
