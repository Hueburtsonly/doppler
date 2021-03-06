#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

#include "linux_bb60_sdk/include/bb_api.h"

class MutexLock {
  pthread_mutex_t* m_mutex;
public:
  MutexLock(pthread_mutex_t* mutex) : m_mutex(mutex) {
    pthread_mutex_lock(m_mutex);
  }
  ~MutexLock() {
    pthread_mutex_unlock(m_mutex);
  }
};


float calTemperature = -FLT_MAX;
float temperature = -FLT_MAX;
float usbVoltage = -FLT_MAX;
float usbCurrent = -FLT_MAX;
volatile int enabled = 1;
volatile int logging = 0;
volatile char* logComment = 0;
volatile int quitting = 0;
volatile int safeToExit = 0;
volatile int singleShot = 0;

extern const char* readback;

pthread_mutex_t mutexConfig = PTHREAD_MUTEX_INITIALIZER;
int updateFlag = 1; // Set to 1 if updatings.
double centre = 5.5e9; // Hz
double span = 1e9; // Hz
double refLevel = -10; // dBm

#define MAX_TRACE_LEN 2048
#define MAX_RAW_LEN 524288

typedef struct {
  float min[MAX_TRACE_LEN]; // dBm
  float max[MAX_TRACE_LEN]; // dBm
  unsigned int traceLen;
  double actualStart;
  double binSize;
  double refLevel; // dBm
  bool overflow;
  int index;
} sweep;

pthread_mutex_t mutexTrace = PTHREAD_MUTEX_INITIALIZER;
sweep traceBuffers[3];
int readBuffer = 0; // The buffer that is currently being read by the GUI, and therefore musn't be written to.
int writeBuffer = -99999; // The buffer that is currently being written into by HW. Shall never be equal to readBuffer or freshestBuffer.
int freshestBuffer = 0; // The buffer that has the freshest information, the GUI should use this buffer whenever possible.

sweep* getFreshSweep(void) {
  MutexLock egg(&mutexTrace);
  sweep* ret = traceBuffers + (readBuffer = freshestBuffer);
  return ret;
}

sweep* claimWritableSweep(void) {
  MutexLock egg(&mutexTrace);
  // read buffer = 0, freshest buffer = 0 -> writeBuffer = 1 or 2 are equiv.
  // read buffer = 0, freshest buffer = 1 -> writeBuffer = 2
  int i;
  for (i=0; i < 3; i++) {
    if (readBuffer != i && freshestBuffer != i) break;
  }
  sweep* ret = traceBuffers + (writeBuffer = i);
  return ret;
}

void releaseWritableSweep(void) {
  MutexLock egg(&mutexTrace);
  freshestBuffer = writeBuffer;
  writeBuffer = -99998;
}

void setFreqCentre(double d) {
  if (d < BB60_MIN_FREQ + 100e3) {
    readback = "Too low.\n";
  } else if (d > BB60_MAX_FREQ - 100e3) {
    readback = "Too high.\n";
  } else {
    MutexLock egg(&mutexConfig);
    updateFlag = 1;
    centre = d;
    if (centre + span / 2 > BB60_MAX_FREQ) {
      span = (BB60_MAX_FREQ - centre) * 2;
      readback = "Note: Span adjusted due to sweep stop.\n";
    }
    if (centre - span / 2 < BB60_MIN_FREQ) {
      span = (centre - BB60_MIN_FREQ) * 2;
      readback = "Note: Span adjusted due to sweep start.\n";
    }
  }
}

void setFreqSpan(double d) {
  if (d < 20) {
        readback = "Too low.\n";
  } else if (d > BB60_MAX_SPAN) {
        readback = "Too high.\n";
  } else {
    if (d < 200e3) {
      readback = "WARNING: Span below suggested minimum.\n";
    }
    MutexLock egg(&mutexConfig);
    updateFlag = 1;
    span = d;
    if (centre + span / 2 > BB60_MAX_FREQ) {
      centre = BB60_MAX_FREQ - span / 2;
      readback = "Note: Centre adjust due to sweep stop.\n";
    }
    if (centre - span / 2 < BB60_MIN_FREQ) {
      centre = BB60_MIN_FREQ + span / 2;
      readback = "Note: Centre adjust due to sweep start.\n";
    }
  }
}

void setRefLevel(double d) {
  if (d < -100) {
    readback = "Too low.\n";
  } else if (d > 20) {
    readback = "Too high.\n";
  } else {
    MutexLock egg(&mutexConfig);
    updateFlag = 1;
    refLevel = d;
  }
}


void* hwMain(void* arg) {
  // TODO: Check for temperature drift/recal.

  bbStatus status;
  int exitCode = 0;
  int device;
  int decimation, oTraceLen;
  int sweepCounter = 0;
  sweep* dest;

  status = bbOpenDevice(&device);
  if (status != bbNoError) {
    fprintf(stderr, "bbOpenDevice = %d\r\n", status);
    exitCode = 1;
    goto unwindFromStart;
  }

  while (!quitting) {
    while (!enabled && !singleShot) {
      usleep(50000);
      if (quitting) goto unwindFromTg;
    }
    singleShot = 0;
    { // lock scope
      MutexLock egg(&mutexConfig);
      if (updateFlag) {
        status = bbConfigureCenterSpan(device, centre, span);
        if (status != bbNoError) {
          fprintf(stderr, "bbConfigureCenterSpan = %d\r\n", status);
          exitCode = 1;
          goto unwindFromTg;
        }
        status = bbConfigureLevel(device, refLevel, -1);
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
        /*status = bbConfigureIO(device, ((wantTg >= 2) ? BB_PORT1_EXT_REF_IN : BB_PORT1_INT_REF_OUT) | BB_PORT1_AC_COUPLED, BB_PORT2_IN_TRIGGER_RISING_EDGE);
        if (status != bbNoError) {
          fprintf(stderr, "bbConfigureIO = %d\r\n", status);
          exitCode = 1;
          goto unwindFromTg;
        }*/
        /*  status = bbConfigureIQ(device, 512, BB_MIN_IQ_BW);
        if (status != bbNoError) {
          fprintf(stderr, "bbConfigureIQ = %d\r\n", status);
          exitCode = 1;
          goto unwindFromTg;
          }*/
        status = bbInitiate(device, BB_SWEEPING, 0);
        if (status != bbNoError) {
          fprintf(stderr, "bbInitiate(device, BB_SWEEPING, 0) = %d\r\n", status);
          exitCode = 1;
          goto unwindFromTg;
        }
        updateFlag = 0;
      }
    }
    pthread_mutex_lock(&mutexConfig);
    pthread_mutex_unlock(&mutexConfig);

    dest = claimWritableSweep();
    dest->index = sweepCounter++;
    dest->refLevel = refLevel;
    status = bbQueryTraceInfo(device, &(dest->traceLen), &(dest->binSize), &(dest->actualStart));
    if (status != bbNoError) {
      fprintf(stderr, "bbQueryTraceInfo = %d\r\n", status);
      exitCode = 1;
      goto unwindFromTg;
    }
    //printf("BTraceLen: %d\nBinSize: %f\nActualStart: %f\n", dest->traceLen, dest->binSize, dest->actualStart);
    oTraceLen = dest->traceLen;
    if (oTraceLen > MAX_RAW_LEN) {
      printf("Raw trace too large: %d.\r\n", oTraceLen);
      goto unwindFromTg;
    }
    decimation = 1;
    while (dest->traceLen > MAX_TRACE_LEN) {
      dest->traceLen /= 2;
      decimation *= 2;
      dest->binSize *= 2;
    }
    //printf("  -> After decimation of %d\nATraceLen: %d\nBinSize: %f\nActualStart: %f\n", decimation, dest->traceLen, dest->binSize, dest->actualStart);


    if (decimation == 0 /* TODO: enable by using 1 */) {
      // No decimation
      status = bbFetchTrace_32f(device, dest->traceLen, dest->min, dest->max);
    } else {
      // Decimation
      static float rawMin[MAX_RAW_LEN];
      static float rawMax[MAX_RAW_LEN];

      for (;;) {
	status = bbFetchTrace_32f(device, dest->traceLen, rawMin, rawMax);
	if (status != 8) break;
	printf("ERROR 8 IGNORED\n");
      }
      if (status == bbNoError || status == bbADCOverflow) {
        int i, j;
        for (i = 0; i < dest->traceLen; i++) {
          double vmin = DBL_MAX;
          double vmax = -DBL_MAX;
          for (j = i * decimation; j < (i+1) * decimation && j < oTraceLen; j++) {
            if (rawMax[j] > vmax) vmax = rawMax[j];
            if (rawMin[j] < vmin) vmin = rawMin[j];
          }
          dest->min[i] = vmin;
          dest->max[i] = vmax;
          //printf("   %7f  %7f\n", dest->min[i], dest->max[i]);
        }
      }
    }
    if (status != bbNoError && status != bbADCOverflow) {
        fprintf(stderr, "bbFetchTrace = %d\r\n", status);
        exitCode = 1;
        goto unwindFromTg;
    }
    dest->overflow = status == bbADCOverflow;

    releaseWritableSweep();

    status = bbGetDeviceDiagnostics(device, &temperature, &usbVoltage, &usbCurrent);
    if (status != bbNoError) {
      fprintf(stderr, "bbFetchTrace = %d\r\n", status);
      exitCode = 1;
      goto unwindFromTg;
    }

    if (calTemperature == -FLT_MAX) {
      calTemperature = temperature;
    } else if (fabs(calTemperature - temperature) >= 2.0) {
      updateFlag = 1;
    }
  }

 unwindFromTg:
 unwindFromDevice:
  bbCloseDevice(device);

 unwindFromStart:
  printf("BB60C closed.\r\n");
  safeToExit = 1;
  return NULL;
}

void hwExit() {
  quitting = 1;
  while (!safeToExit) {
    usleep(50000);
  }
  printf("Goodbye!\r\n");  
}

pthread_t hwThread;

void hwInit() {
  traceBuffers[0].min[0] = -60;
  traceBuffers[0].max[0] = -40;
  traceBuffers[0].min[1] = -40;
  traceBuffers[0].max[1] = -20;
  traceBuffers[0].min[2] = -60;
  traceBuffers[0].max[2] = -40;
  traceBuffers[0].traceLen = 3;
  traceBuffers[0].actualStart = 5.2e9;
  traceBuffers[0].binSize = 0.2e9;
  traceBuffers[0].refLevel = -0;

  pthread_create(&hwThread, NULL, hwMain, NULL);
}

// Channel Name, Band lower limit, Band upper limit
static const int wifiChannels[22][3] = {
  {36, 5170, 5190},
  {40, 5190, 5210},
  {44, 5210, 5230},
  {48, 5230, 5250},
  {52, 5250, 5270},
  {56, 5270, 5290},
  {60, 5290, 5310},
  {64, 5310, 5330},
  {100, 5490, 5510},
  {104, 5510, 5530},
  {108, 5530, 5550},
  {112, 5550, 5570},
  {116, 5570, 5590},
  {132, 5650, 5670},
  {136, 5670, 5690},
  {140, 5690, 5710},
  {144, 5710, 5730},
  {149, 5735, 5755},
  {153, 5755, 5775},
  {157, 5775, 5795},
  {161, 5795, 5815},
  {165, 5815, 5835}
};

#define WIFI_N 22
//(sizeof(wifiChannels)/sizeof(wifiChannels[0]))
