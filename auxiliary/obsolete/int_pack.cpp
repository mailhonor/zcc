/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-09
 * ================================
 */

inline unsigned int int_unpack(const char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned n = p[0];
    n <<= 8; n |= p[1];
    n <<= 8; n |= p[2];
    n <<= 8; n |= p[3];
    return n;
}

inline void int_pack(int num, char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[3] = num & 255;
    num >>= 8; p[2] = num & 255;
    num >>= 8; p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}

inline unsigned int int_unpack3(const char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned n = p[0];
    n <<= 8; n |= p[1];
    n <<= 8; n |= p[2];
    return n;
}

inline void int_pack3(int num, char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[2] = num & 255;
    num >>= 8; p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}
