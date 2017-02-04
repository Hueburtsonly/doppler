#include <stdlib.h>
#include "linux_bb60_sdk/include/bb_api.h"


extern const char* readback;

int updateFlag = 1; // Set to 1 if updatings.
double centre = 5.5e9; // Hz
double span = 1e9; // Hz
double refLevel = -10; // dBm


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