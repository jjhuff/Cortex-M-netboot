#ifndef __LOG_H__
#define __LOG_H__

void logInit();
void logStr(const char* str);
void logHex8(uint8_t v);
void logHex32(uint32_t v);

#if DEBUG
# define LOG(m) logStr(m "\r\n");
# define LOG_STR(m) logStr(m);
# define LOG_HEX_BYTE(v) logHex8((uint8_t)v);
# define LOG_HEX(v) logHex32((uint32_t)v);
#else
# define LOG(m) ;
# define LOG_STR(m) ;
# define LOG_HEX_BYTE(v) ;
# define LOG_HEX(v) ;
#endif

#endif
