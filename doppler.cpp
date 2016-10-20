#include "linux_bb60_sdk/include/bb_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

volatile int closing = 0;

static inline void drawBar(double size, char* center, char ch) {
  int dir = 1;
  if (size < 0) {
    dir = -1;
    size = -size;
  }
  int len = size * 11;
  for (int i = 1; i <= len && i <= 10; i++) {
    center[dir*i] = ch;
  }
}

int main(int argc, const char** argv) {

  int wantTg = 1;
  int device;
  double center = 2.39e9;
  double outputlevel = 0; //dBm
  double inputref = -0; //dBm
  bbStatus status;

  status = bbOpenDevice(&device);
  if (status != bbNoError) {
    printf("bbOpenDevice = %d\r\n", status);
    return 1;
  }

  if (wantTg) {
    status = bbAttachTg(device);
    if (status != bbNoError) {
      printf("bbAttachTg = %d\r\n", status);
      bbCloseDevice(device);
      return 1;
    }

    if (wantTg >= 2) {
      status = bbNotSupportedErr; //bbSetTgReference(device, TG_REF_EXTERNAL_IN);
      if (status != bbNoError) {
        printf("bbSetTgReference(device, TG_REF_EXTERNAL_IN) = %d\r\n", status);
        bbCloseDevice(device);
        return 1;
      }
    }
  }

  status = bbSetTg(device, center, outputlevel);
  if (status != bbNoError) {
    printf("bbSetTg = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }

  status = bbConfigureCenterSpan(device, center, 200e3);
  if (status != bbNoError) {
    printf("bbConfigureCenterSpan = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }
  status = bbConfigureLevel(device, inputref, -1);
  if (status != bbNoError) {
    printf("bbConfigureLevel = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }
  status = bbConfigureGain(device, BB_AUTO_GAIN);
  if (status != bbNoError) {
    printf("bbConfigureGain = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }
  status = bbConfigureIQ(device, 4096, 7.99e3);
  if (status != bbNoError) {
    printf("bbConfigureIQ = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }
  status = bbInitiate(device, BB_STREAMING, BB_STREAM_IQ);
  if (status != bbNoError) {
    printf("bbInitiate(device, BB_STREAMING, BB_STREAM_IQ) = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }
  int N;
  double bandwidth = -1;
  int samplesPerSecond = -1;
  status = bbQueryStreamInfo(device, &N, &bandwidth, &samplesPerSecond);
  if (status != bbNoError) {
    printf("bbQueryStreamInfo() = %d\r\n", status);
    bbCloseDevice(device);
    return 1;
  }
  float* iqData = (float*)malloc(2 * N * sizeof(float));
  //           01234567890123456789012345678901234567890123456789012345678901234567890123456789
  char buf[] = "I: [          |          ] Q: [          |          ]";
  for (;;) {
    status = bbFetchRaw(device, iqData, 0);
    if (status != bbNoError) {
      printf("\r\n\nbbFetchRaw = %d\r\n", status);
      break;
    }
    float sumI = 0;
    float sumQ = 0;
    for (int i = 0; i < N; i++) {
      sumI += iqData[2*i];
      sumQ += iqData[2*i+1];
    }
    double avgAbs = hypot(sumI, sumQ);
    float normI = sumI / avgAbs;
    float normQ = sumQ / avgAbs;
    avgAbs /= N;

    drawBar(normI, buf + 14, '-');
    drawBar(normQ, buf + 41, '-');

    printf("%s avgAbs=%.2lf output=%.1lf ref=%.1lf   \r", buf, avgAbs, outputlevel, inputref);

    drawBar(normI, buf + 14, ' ');
    drawBar(normQ, buf + 41, ' ');

    if (closing) {
      printf("\r\n\nCancelled.\r\n");
      break;
    }
  }

  free(iqData);
  bbCloseDevice(device);
}