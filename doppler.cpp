
#include "linux_bb60_sdk/include/bb_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "usbtg.h"
#include <unistd.h>
#include <signal.h>

volatile int closing = 0;

void sigint_handler(int signum) {
  closing = 1;
}

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

#define ACCUM_LEN 100

int main(int argc, const char** argv) {
  struct sigaction new_action;
  new_action.sa_handler = &sigint_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction(SIGINT, &new_action, NULL);

  setvbuf(stdout, NULL, _IONBF, 0);
  
  bbStatus status;
  int exitCode = 0;
  int wantTg = 2;
  int device;
  double center = 2.943e9;
  double outputLevel = -22; //dBm
  int inputRef = 0; //dBm
  inputRef -= (inputRef-1) % 5;

  status = bbOpenDevice(&device);
  if (status != bbNoError) {
    fprintf(stderr, "bbOpenDevice = %d\r\n", status);
    exitCode = 1;
    goto unwindFromStart;
  }

  if (wantTg) {
    status = tgOpen();
    if (status != bbNoError) {
      fprintf(stderr, "bbAttachTg = %d\r\n", status);
      exitCode = 1;
      goto unwindFromDevice;
    }

    status = tgSetTg(center, outputLevel);
    if (status != bbNoError) {
      fprintf(stderr, "bbSetTg = %d\r\n", status);
      exitCode = 1;
      goto unwindFromTg;
    }
    usleep(157000);
  }

  status = bbConfigureCenterSpan(device, center, 1e3);
  if (status != bbNoError) {
    fprintf(stderr, "bbConfigureCenterSpan = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  status = bbConfigureLevel(device, inputRef, -1);
  if (status != bbNoError) {
    fprintf(stderr, "bbConfigureLevel = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  status = bbConfigureGain(device, BB_AUTO_GAIN); // <------------------------------------------------------ *** *** ***
  if (status != bbNoError) {
    fprintf(stderr, "bbConfigureGain = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  status = bbConfigureIO(device, ((wantTg >= 2) ? BB_PORT1_EXT_REF_IN : BB_PORT1_INT_REF_OUT) | BB_PORT1_AC_COUPLED, BB_PORT2_IN_TRIGGER_RISING_EDGE);
  if (status != bbNoError) {
    fprintf(stderr, "bbConfigureIO = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  status = bbConfigureIQ(device, 512, BB_MIN_IQ_BW);
  if (status != bbNoError) {
    fprintf(stderr, "bbConfigureIQ = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  status = bbInitiate(device, BB_STREAMING, BB_STREAM_IQ);
  if (status != bbNoError) {
    fprintf(stderr, "bbInitiate(device, BB_STREAMING, BB_STREAM_IQ) = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  if (wantTg >= 2) {
    //status = tgSetTgReference(0x00);
    status = tgSetTgReference(0x02);
    if (status != bbNoError) {
      fprintf(stderr, "bbSetTgReference(device, TG_REF_EXTERNAL_IN) = %d\r\n", status);
      exitCode = 1;
      goto unwindFromTg;
    }
  }

  {
    int N;
    double bandwidth = -1;
    int samplesPerSecond = -1;
    status = bbQueryStreamInfo(device, &N, &bandwidth, &samplesPerSecond);
    if (status != bbNoError) {
      fprintf(stderr, "bbQueryStreamInfo() = %d\r\n", status);
      exitCode = 1;
      goto unwindFromTg;
    }
    fprintf(stderr, "N = %d\r\n", N);
    float* iqData = (float*)malloc(2 * N * sizeof(float));

    //           01234567890123456789012345678901234567890123456789012345678901234567890123456789
    char buf[] = "I: [          |          ] Q: [          |          ]";
    struct timespec tsPrev;
    struct timespec tsBefore;
    struct timespec tsAfter;
    long sd = 0;
    long bd = 0;
    double prevAngle = -1;
    double angleOffset = 0;
    double accumI = 0;
    double accumQ = 0;
    double accumCount = ACCUM_LEN;
    int accumDivisor = ACCUM_LEN;
    double offsetI = 0;
    double offsetQ = 0;
    for (int frame = 0;; frame = 1) {
      status = bbFetchRaw(device, iqData, 0);
      timespec_get(&tsBefore, TIME_UTC);
      int isOverflow = 0;
      if (status == bbADCOverflow) {
        // Take special action.
        fprintf(stderr, "\r\nOverflow\r\n");
        isOverflow = 1;
      } else if (status != bbNoError) {
        fprintf(stderr, "\r\n\nbbFetchRaw = %d\r\n", status);
        exitCode = 1;
        break;
      }
      float sumI = -offsetI;
      float sumQ = -offsetQ;
      for (int i = 0; i < N; i++) {
        sumI += iqData[2*i];
        sumQ += iqData[2*i+1];
        if (frame >= 500-5)
        printf("%.5e, %.5e\r\n", (double)(iqData[2*i]), (double)(iqData[2*i+1]));
      }
      double avgAbs = hypot(sumI, sumQ);
recompute:
      float normI = (cos(angleOffset) * sumI / avgAbs + sin(angleOffset) * sumQ / avgAbs);
      float normQ = (cos(angleOffset) * sumQ / avgAbs - sin(angleOffset) * sumI / avgAbs);
      float angle = atan2(normI, normQ);
      if (prevAngle >= 0) {
        angleOffset = fmod(angleOffset + prevAngle - angle + (M_PI * 2), M_PI * 2);;
        prevAngle = -1;
        goto recompute;
      }
      avgAbs /= N;

      int highSet = (int)floor(4 * log10(avgAbs * 111.1 * 1.25)) * 5 - 4;
      int lowSet = (int)floor(4 * log10(avgAbs * 1.1 / 1.25)) * 5 - 4;
      if (highSet < -54) {
        highSet = -54;
      }
      if (isOverflow && lowSet < inputRef + 5) {
        lowSet = inputRef + 5;
      }

      drawBar(normI, buf + 14, '-');
      drawBar(normQ, buf + 41, '-');

      fprintf(stderr, "%s%cmag=%.2fdBm @ %3.1f°%cavgAbs=%.2le output=%.1lf ref=%d %d~%d %.1f %.1f                 \r", buf, isOverflow ? '*' : ' ', 20 * log10(avgAbs), (angle / M_PI + 1) * 180, (accumCount == 0) ? ' ' : '*' , avgAbs, outputLevel, inputRef, highSet, lowSet, sd / 1.0e6, bd / 1.0e6);
      printf("%lf,%lf\r\n", (angle / M_PI + 1) * 180, 20 * log10(avgAbs)); 

      drawBar(normI, buf + 14, ' ');
      drawBar(normQ, buf + 41, ' ');

      if (closing) {
        fprintf(stderr, "\r\n\nCancelled.\r\n");
        break;
      }

      int changedLevel = 0;
      /*
      if (lowSet > inputRef) {
        inputRef = lowSet;
        changedLevel = 1;
      } else if (highSet < inputRef) {
        inputRef = highSet;
        changedLevel = 1;
  }*/
      if (changedLevel) {
        status = bbConfigureLevel(device, inputRef, -1);
        if (status != bbNoError) {
          fprintf(stderr, "bbConfigureLevel = %d\r\n", status);
          exitCode = 1;
          goto unwindFromTg;
        }
        status = bbInitiate(device, BB_STREAMING, BB_STREAM_IQ);
        if (status != bbNoError) {
          fprintf(stderr, "bbInitiate(device, BB_STREAMING, BB_STREAM_IQ) = %d\r\n", status);
          exitCode = 1;
          goto unwindFromTg;
        }
        prevAngle = angle;
      }


      if (accumCount != 0) {
        accumI += sumI;
        accumQ += sumQ;
        if (--accumCount == 0) {
          offsetI = accumI / accumDivisor;
          offsetQ = accumQ / accumDivisor;
          accumI = 0;
          accumQ = 0;
          accumDivisor = 0;
        }
      }

      char ich = 0; //getch();
      if (ich == '0') {
        accumCount += ACCUM_LEN;
        accumDivisor += ACCUM_LEN;
      }

      timespec_get(&tsAfter, TIME_UTC);

      if (frame != 0) {
       sd += (tsAfter.tv_sec - tsBefore.tv_sec) * 1000000000L + (tsAfter.tv_nsec - tsBefore.tv_nsec);
       bd += (tsAfter.tv_sec - tsPrev.tv_sec) * 1000000000L + (tsAfter.tv_nsec - tsPrev.tv_nsec);
      }

      tsPrev.tv_sec = tsAfter.tv_sec;
      tsPrev.tv_nsec = tsAfter.tv_nsec;
    }

    free(iqData);
  }

unwindFromTg:
  if (wantTg) {
    tgSetTg(0, -100);
    tgClose();
  }

unwindFromDevice:
  bbCloseDevice(device);

unwindFromStart:
  return exitCode;
}
