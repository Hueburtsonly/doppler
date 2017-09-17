
#include "linux_bb60_sdk/include/bb_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "usbtg.h"
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

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

int parseSi(const char* const str, double* out) {
  double value = 0;
  char suffix[4];

  int count = sscanf(str, "%lf%1s", &value, suffix);

  if (count == 0) {
    return 1;
  }
  
  if (count == 1) {
    *out = value;
    return 0;
  }

  switch (suffix[0]) {
    case 'f': *out = value * 1e-15; return 0;
    case 'p': *out = value * 1e-12; return 0;
    case 'n': *out = value * 1e-9; return 0;
    case 'u': *out = value * 1e-6; return 0;
    case 'm': *out = value * 1e-3; return 0;
    
    case 'k': *out = value * 1e3; return 0;
    case 'M': *out = value * 1e6; return 0;
    case 'G': *out = value * 1e9; return 0;
    case 'T': *out = value * 1e12; return 0;
    default:
      fprintf (stderr, "Unexpected suffix `%c'.\n", suffix[0]);
      return 1;
  }
  

  return 0;
}

int main(int argc, char* const* argv) {
  struct sigaction new_action;
  new_action.sa_handler = &sigint_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction(SIGINT, &new_action, NULL);

  setvbuf(stdout, NULL, _IONBF, 0);


  double center = 2.943e9; // Hz
  double length = 10; // seconds
  double inputRef = 0; // dBm
  int index;
  int c;
  int framecount = 1;

  opterr = 0;

  while ((c = getopt (argc, argv, "c:l:r:")) != -1)
    switch (c)
    {
      case 'c':
        if (parseSi(optarg, &center)) {
          return 1;
        }

        break;
      case 'l':
        if (parseSi(optarg, &length)) {
          return 1;
        }

        break;
      case 'r':
        if (1 != sscanf(optarg, "%lf", &inputRef)) {
          fprintf (stderr,
                   "Invalid reference level '%s'.\n",
                   optarg);
          return 1;
        }
        break;
      case '?':
        if (optopt == '?')
          fprintf(stderr, "HELP\n");
        else if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
    }

  printf ("length = %lf, inputRef = %lf, cvalue = %lf\n",
          length, inputRef, center);

  for (index = optind; index < argc; index++) {
    printf ("Unexpected argument '%s'\n", argv[index]);
    return 1;
  }
    
  bbStatus status;
  int exitCode = 0;
  //int wantTg = 2;
  int device;
  int overflowCount = 0;
  FILE* outfile = 0;
  char filename[80];
  int frameCount = 0;
  
  status = bbOpenDevice(&device);
  if (status != bbNoError) {
    fprintf(stderr, "bbOpenDevice = %d\r\n", status);
    exitCode = 1;
    goto unwindFromStart;
  }
  /*
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
    }*/

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
  //  status = bbConfigureIO(device, ((wantTg >= 2) ? BB_PORT1_EXT_REF_IN : BB_PORT1_INT_REF_OUT) | BB_PORT1_AC_COUPLED, BB_PORT2_IN_TRIGGER_RISING_EDGE);
  //  if (status != bbNoError) {
  //    fprintf(stderr, "bbConfigureIO = %d\r\n", status);
  //    exitCode = 1;
  //    goto unwindFromTg;
  //  }
  status = bbConfigureIQ(device, 1, 27e6);
  if (status != bbNoError) {
    fprintf(stderr, "bbConfigureIQ = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }

  {
    time_t t;
    struct tm* tmp;

    t = time(NULL);
    tmp = localtime(&t);
    strftime(filename, sizeof(filename), "%Y%m%d-%H%M%S.iq", tmp);

    outfile = fopen(filename, "wb");
    if (!outfile) {
      fprintf(stderr, "Error opening output file.\r\n");
      exitCode = 1;
      goto unwindFromTg;
    }
  }

  status = bbInitiate(device, BB_STREAMING, BB_STREAM_IQ);
  if (status != bbNoError) {
    fprintf(stderr, "bbInitiate(device, BB_STREAMING, BB_STREAM_IQ) = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  //  if (wantTg >= 2) {
  //    //status = tgSetTgReference(0x00);
  //    status = tgSetTgReference(0x02);
  //    if (status != bbNoError) {
  //      fprintf(stderr, "bbSetTgReference(device, TG_REF_EXTERNAL_IN) = %d\r\n", status);
  //      exitCode = 1;
  //      goto unwindFromTg;
  //    }
  //  }

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
    fprintf(stderr, "samplesPerSecond = %d\r\n", samplesPerSecond);
    size_t bufferLen = 2 * N * sizeof(float);
    float* iqData = (float*)malloc(bufferLen);

    frameCount = (int)((samplesPerSecond * length + N - 1) / N);
    length = 0; //frameCount * (double)N / samplesPerSecond;
    fprintf(stderr, "frameCount = %d\r\n", frameCount);
    
    for (int frame = 0; frame < frameCount; frame++) {
      // fprintf(stderr, "frame = %d\r\n", frame);
      status = bbFetchRaw(device, iqData, 0);
      if (status == bbADCOverflow) {
        // Take special action.
        fprintf(stderr, "\r\nOverflow\r\n");
        overflowCount++;
      } else if (status != bbNoError) {
        fprintf(stderr, "\r\n\nbbFetchRaw = %d\r\n", status);
        exitCode = 1;
        break;
      }

      fwrite((void*)iqData, bufferLen, 1, outfile);
      length = (frame+1) * (double)N / samplesPerSecond;

      if (closing) {
        fprintf(stderr, "\r\n\nCancelled.\r\n");
        exitCode = 2;
        break;
      }

    }

    free(iqData);
  }


  {
    FILE* logfile = 0;
    logfile = fopen("iqrecord.log", "a");
    if (!logfile) {
      fprintf(stderr, "Error opening logfile.\r\n");
      exitCode = 1;
      goto unwindFromTg;
    }

    if (0 == ftell(logfile)) {
      // This file is brand new, add a header.
      fprintf(logfile, "\"Filename\", \"UnixTime\", \"Center\", \"InputRef\", \"Length\", \"Overflow\"\r\n");
    }

    fprintf(logfile, "\"%s\", %lld, %e, %f, %f, %d\r\n", filename, (long long)(time(0)), center, inputRef, length, overflowCount);

    fclose(logfile);
  }

  if (overflowCount > 0) {
    fprintf(stderr, "\r\n\n** Encountered %d (%.0f) ADC overflows. **\r\n", overflowCount, (overflowCount * 100.0 / frameCount));
  }

unwindFromTg:
  //  if (wantTg) {
  //    tgSetTg(0, -100);
  //    tgClose();
  //  }
  if (outfile) fclose(outfile);
unwindFromDevice:
  bbCloseDevice(device);

unwindFromStart:
  return exitCode;
}
