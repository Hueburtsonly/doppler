#include "linux_bb60_sdk/include/bb_api.h"

bbStatus tgOpen();
bbStatus tgSetTg(double center, double level);
bbStatus tgSetTgReference(unsigned char setting);
void tgClose();
