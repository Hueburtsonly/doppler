#include "usbtg.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ftdi.h>

//FT_HANDLE ftHandle = 0;

struct ftdi_context *ftdi = 0;

bbStatus tgOpen(void) {
  int ret;
  if ((ftdi = ftdi_new()) == 0) {
    fprintf(stderr, "ftdi_new failed\n");
    return bbDeviceConnectionErr;
  }
  if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6001)) < 0) {
    fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    ftdi_free(ftdi);
    return bbDeviceConnectionErr;
  }
  if ((ret = ftdi_set_baudrate(ftdi, 25000)) < 0) {
    fprintf(stderr, "unable to set baud: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    ftdi_free(ftdi);
    return bbDeviceConnectionErr;
  }
  return bbNoError;
}

bbStatus tgSetTg(double center, double level) {
  int ret;
  uint32_t fset = (uint32_t)(center / 10 + 0.5);
  uint8_t aset = 0x32;
  for (int lev = -10; lev >= -30; --lev) {
    if (lev > level + 0.1) continue;
    aset = (-10 - lev) * 2 + 0x0a;
    break;
  }
  unsigned char buf[12];
  buf[0] = 'F';
  buf[1] = fset & 0xff;
  buf[2] = (fset >> 8) & 0xff;
  buf[3] = (fset >> 16) & 0xff;
  buf[4] = (fset >> 24) & 0xff;
  buf[5] = 0xC0;
  buf[6] = 'A';
  buf[7] = aset;
  buf[8] = 0xC0;
  buf[9] = 'M';
  buf[10] = (center >= 4e9) ? 0x02 : 0x01;
  buf[11] = 0xC0;

  fprintf(stderr, "Writing %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \r\n"
    , (unsigned int)(buf[0])
    , (unsigned int)(buf[1])
    , (unsigned int)(buf[2])
    , (unsigned int)(buf[3])
    , (unsigned int)(buf[4])
    , (unsigned int)(buf[5])
    , (unsigned int)(buf[6])
    , (unsigned int)(buf[7])
    , (unsigned int)(buf[8])
    , (unsigned int)(buf[9])
    , (unsigned int)(buf[10])
    , (unsigned int)(buf[11])
    );

  if ((ret = ftdi_write_data(ftdi, buf, 9)) < 0) {
    fprintf(stderr, "unable to write to ftdi device a: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    return bbDeviceConnectionErr;
  }

  return bbNoError;
}

bbStatus tgSetTgReference(uint8_t setting) {
  int ret;
  if (setting >= 3) {
    return bbInvalidParameterErr;
  }
  unsigned char buf[3];
  buf[0] = 'R';
  buf[1] = setting;
  buf[2] = 0xC0;

  fprintf(stderr, "Setting ref...\r\n");

  if ((ret = ftdi_write_data(ftdi, buf, 3)) < 0) {
    fprintf(stderr, "unable to write to ftdi device a: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    return bbDeviceConnectionErr;
  }

  return bbNoError;
}

void tgClose() {
  int ret;
  fprintf(stderr, "Closing FTDI...\r\n");
  if ((ret = ftdi_usb_close(ftdi)) < 0)
  {
      fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
  }
  ftdi_free(ftdi);
}


/*

//usr/local/lib/libbb_api.so: undefined reference to `FT_Write'
//usr/local/lib/libbb_api.so: undefined reference to `FT_GetDeviceInfoDetail'
//usr/local/lib/libbb_api.so: undefined reference to `FT_Open'
//usr/local/lib/libbb_api.so: undefined reference to `FT_Read'
//usr/local/lib/libbb_api.so: undefined reference to `FT_SetLatencyTimer'
//usr/local/lib/libbb_api.so: undefined reference to `FT_CreateDeviceInfoList'
//usr/local/lib/libbb_api.so: undefined reference to `FT_Close'
//usr/local/lib/libbb_api.so: undefined reference to `FT_SetBaudRate'
//usr/local/lib/libbb_api.so: undefined reference to `FT_SetTimeouts'

usb.src == "host"

*/