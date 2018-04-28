//
// Created by noxue on 4/27/18.
//

#include "utils.h"


int app_perror( const char *sender, const char *title,
                pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));

    PJ_LOG(3,(sender, "%s: %s [code=%d]", title, errmsg, status));
    return 1;
}




#define SIGN_BIT    (0x80)
#define QUANT_MASK  (0xf)
#define NSEGS       (8)
#define SEG_SHIFT   (4)
#define SEG_MASK    (0x70)

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
                           0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};


unsigned char _u2a[128] = {
        1,  1,  2,  2,  3,  3,  4,  4,
        5,  5,  6,  6,  7,  7,  8,  8,
        9,  10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 27, 29, 31, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 43, 44,
        46, 48, 49, 50, 51, 52, 53, 54,
        55, 56, 57, 58, 59, 60, 61, 62,
        64, 65, 66, 67, 68, 69, 70, 71,
        72, 73, 74, 75, 76, 77, 78, 79,
        81, 82, 83, 84, 85, 86, 87, 88,
        89, 90, 91, 92, 93, 94, 95, 96,
        97, 98, 99, 100,101,102,103,104,
        105,106,107,108,109,110,111,112,
        113,114,115,116,117,118,119,120,
        121,122,123,124,125,126,127,128
};

unsigned char _a2u[128] = {
        1,  3,  5,  7,  9,  11, 13, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 32, 33, 33, 34, 34, 35, 35,
        36, 37, 38, 39, 40, 41, 42, 43,
        44, 45, 46, 47, 48, 48, 49, 49,
        50, 51, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61, 62, 63, 64, 64,
        65, 66, 67, 68, 69, 70, 71, 72,
        73, 74, 75, 76, 77, 78, 79, 79,
        80, 81, 82, 83, 84, 85, 86, 87,
        88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100,101,102,103,
        104,105,106,107,108,109,110,111,
        112,113,114,115,116,117,118,119,
        120,121,122,123,124,125,126,127
};

static int search(int val,short *table,int size)
{
    int     i;
    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}


unsigned char linear2alaw(int pcm_val)
{
    int             mask;
    int             seg;
    unsigned char   aval;

    if (pcm_val >= 0) {
        mask = 0xD5;
    } else {
        mask = 0x55;
        pcm_val = -pcm_val - 8;
    }


    seg = search(pcm_val, seg_end, 8);



    if (seg >= 8)
        return (0x7F ^ mask);
    else {
        aval = seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 4) & QUANT_MASK;
        else
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
        return (aval ^ mask);
    }
}


int alaw2linear(unsigned char a_val)
{
    int     t;
    int     seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
        case 0:
            t += 8;
            break;
        case 1:
            t += 0x108;
            break;
        default:
            t += 0x108;
            t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}

#define BIAS        (0x84)


unsigned char linear2ulaw(int pcm_val)
{
    int     mask;
    int     seg;
    unsigned char   uval;


    if (pcm_val < 0) {
        pcm_val = BIAS - pcm_val;
        mask = 0x7F;
    } else {
        pcm_val += BIAS;
        mask = 0xFF;
    }


    seg = search(pcm_val, seg_end, 8);


    if (seg >= 8)
        return (0x7F ^ mask);
    else {
        uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
        return (uval ^ mask);
    }

}


int ulaw2linear( unsigned char  u_val)
{
    int     t;


    u_val = ~u_val;

    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


unsigned char alaw2ulaw(unsigned char   aval)
{
    aval &= 0xff;
    return ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
            (0x7F ^ _a2u[aval ^ 0x55]));
}


unsigned char ulaw2alaw(unsigned char   uval)
{
    uval &= 0xff;
    return ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)):
            (0x55 ^ (_u2a[0x7F ^ uval] - 1)));
}