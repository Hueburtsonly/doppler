#include <stdlib.h>
#include <pthread.h>
#include "linux_bb60_sdk/include/bb_api.h"
#include <float.h>

extern const char* readback;

pthread_mutex_t mutexConfig = PTHREAD_MUTEX_INITIALIZER;
int updateFlag = 1; // Set to 1 if updatings.
double centre = 5.5e9; // Hz
double span = 1e9; // Hz
double refLevel = -10; // dBm

#define MAX_TRACE_LEN 2048
#define MAX_RAW_LEN 65536

typedef struct {
  double min[MAX_TRACE_LEN]; // dBm
  double max[MAX_TRACE_LEN]; // dBm
  unsigned int traceLen;
  double actualStart;
  double binSize;
  double refLevel; // dBm
} sweep;

pthread_mutex_t mutexTrace = PTHREAD_MUTEX_INITIALIZER;
sweep traceBuffers[3];
int readBuffer = 0; // The buffer that is currently being read by the GUI, and therefore musn't be written to.
int writeBuffer = -99999; // The buffer that is currently being written into by HW. Shall never be equal to readBuffer or freshestBuffer.
int freshestBuffer = 0; // The buffer that has the freshest information, the GUI should use this buffer whenever possible.

sweep* getFreshSweep(void) {
  pthread_mutex_lock(&mutexTrace);
  sweep* ret = traceBuffers + (readBuffer = freshestBuffer);
  pthread_mutex_unlock(&mutexTrace);
  return ret;
}

sweep* claimWritableSweep(void) {
  pthread_mutex_lock(&mutexTrace);
  // read buffer = 0, freshest buffer = 0 -> writeBuffer = 1 or 2 are equiv.
  // read buffer = 0, freshest buffer = 1 -> writeBuffer = 2
  int i;
  for (i=0; i < 3; i++) {
    if (readBuffer != i && freshestBuffer != i) break;
  }
  sweep* ret = traceBuffers + (writeBuffer = i);
  pthread_mutex_unlock(&mutexTrace);
  return ret;
}

void releaseWritableSweep(void) {
  pthread_mutex_lock(&mutexTrace);
  freshestBuffer = writeBuffer;
  writeBuffer = -99998;
  pthread_mutex_unlock(&mutexTrace);
}

void setFreqCentre(double d) {
  if (d < BB60_MIN_FREQ + 100e3) {
    readback = "Too low.\n";
  } else if (d > BB60_MAX_FREQ - 100e3) {
    readback = "Too high.\n";
  } else {
    // TODO LOCK
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
    // TODO LOCK
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
    // TODO lock
    refLevel = d;
  }
}


void* hwMain(void* arg) {
  // TODO: Check for temperature drift/recal.

  bbStatus status;
  int exitCode = 0;
  int device;
  int decimation, oTraceLen;
  sweep* dest;

  status = bbOpenDevice(&device);
  if (status != bbNoError) {
    fprintf(stderr, "bbOpenDevice = %d\r\n", status);
    exitCode = 1;
    goto unwindFromStart;
  }


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


  dest = claimWritableSweep();
  dest->refLevel = refLevel;
  status = bbQueryTraceInfo(device, &(dest->traceLen), &(dest->binSize), &(dest->actualStart));
  if (status != bbNoError) {
    fprintf(stderr, "bbQueryTraceInfo = %d\r\n", status);
    exitCode = 1;
    goto unwindFromTg;
  }
  printf("BTraceLen: %d\nBinSize: %f\nActualStart: %f\n", dest->traceLen, dest->binSize, dest->actualStart);
  oTraceLen = dest->traceLen;
  decimation = 1;
  while (dest->traceLen > MAX_TRACE_LEN) {
    dest->traceLen /= 2;
    decimation *= 2;
    dest->binSize *= 2;
  }
  printf("  -> After decimation of %d\nATraceLen: %d\nBinSize: %f\nActualStart: %f\n", decimation, dest->traceLen, dest->binSize, dest->actualStart);


  if (decimation == 0 /* TODO: enable by using 1 */) {
    // No decimation
    status = bbFetchTrace(device, dest->traceLen, dest->min, dest->max);
  } else {
    // Decimation
    double rawMin[MAX_RAW_LEN];
    double rawMax[MAX_RAW_LEN];

    status = bbFetchTrace(device, dest->traceLen, rawMin, rawMax);
    if (status == bbNoError) {
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
  if (status != bbNoError) {
      fprintf(stderr, "bbFetchTrace = %d\r\n", status);
      exitCode = 1;
      goto unwindFromTg;
  }

  releaseWritableSweep();

  unwindFromTg:
  unwindFromDevice:
    bbCloseDevice(device);

  unwindFromStart:
  return (void*)exitCode;

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
