#include <sam.h>
#include <string.h>

#include "sam_ba_cdc.h"
#include "utils.h"
#include "log.h"

void logInit() {
#if DEBUG
  P_USB_CDC pCdc = usb_init();

  // TODO: Add a timeout
  while( pCdc->IsConfigured(pCdc) == 0 || pCdc->currentConnection == 0) {}

  // Wait for USB stuff to settle before continuing
  delay(250);
#endif
}

static char hex_nibble(uint8_t d) {
  return d > 9 ? 'A' + d - 10 : '0' + d;
}

void logStr(const char* str) {
  cdc_write_buf(str, strlen(str));
}

void logHex8(uint8_t n)
{
  char buff[2];
  buff[0] = hex_nibble(n >> 4);
  buff[1] = hex_nibble(n & 0x0F);
  cdc_write_buf(buff, sizeof(buff));
}

void logHex32(uint32_t n)
{
  char buff[8];
  int i;
  for (i=0; i<8; i++)
  {
    int d = n & 0XF;
    n = (n >> 4);

    buff[7-i] = hex_nibble(d);
  }
  cdc_write_buf(buff, sizeof(buff));
}
