#include <GL/glut.h>

#include "spectrum_hw.cpp"

int cmdCursor = 0;

char cmdBuf[81];

const char* const help = "\
Hotkeys:                              \n\
  Left/Right: Change centre frequency.\n\
  Up/Down:    Change reference level. \n\
  +/-:        Change span.            \n\
  \\:          Single-shot capture.    \n\
                                      \n\
Commands:                             \n\
  ? :     Display this help           \n\
  c ... : Change centre frequency.    \n\
  s ... : Change span.                \n\
  r ... : Change reference level.     \n\
  e :     Pause/resume sweep.         \n\
  l :     Toggle logging.             \n\
  / ... : Add comment.                \n\
  q :     Quit.                       \n\
";

const char* const malformed = "Unknown command. Enter '?' for help.\n";
const char* const badFreq = "Bad frequency. Must be double, followed by optional k, M or G.\n";
const char* const loggingDisabled = "Logging disabled.\n";
const char* const loggingEnabled = "Logging enabled.\n";

const char* readback = 0;

double readFreq(char* s) {
  char* suffixPtr;
  double v = strtod(s, &suffixPtr);
  if (s == suffixPtr) {
    readback = badFreq;
    return -999;
  }
  while (*suffixPtr) {
    switch (*suffixPtr++) {
    case 'k':
      v = v * 1e3;
      break;
    case 'M':
      v = v * 1e6;
      break;
    case 'G':
    case 'g':
      v = v * 1e9;
      break;
    default:
      readback = badFreq;
      return -999;
    }
  }
  return v;
}

void handleCmd() {
  printf("Handling '%s'\r\n", cmdBuf);
  readback = 0;
  if (cmdBuf[0] == '\0') {
    return;
  }
  if (cmdBuf[0] == '?') {
    if (cmdBuf[1] == '\0') {
      printf("Disp readback\r\n");
      readback = help;
      return;
    }
    return;
  }
  /*if (cmdBuf[1] != ' ') {
    readback = malformed;
    return;
  }*/
  if ((cmdBuf[0] | ' ') == 'c') {
    // Set centre

    double d = readFreq(cmdBuf + 1);
    if (d > 0) {
      setFreqCentre(d);
    }
    return;
  }
  if ((cmdBuf[0] | ' ') == 's') {
    // Set span
    double d = readFreq(cmdBuf + 1);
    if (d > 0) {
      setFreqSpan(d);
    }
    return;
  }
  if ((cmdBuf[0] | ' ') == 'r') {
    // Set ref level
    double d = readFreq(cmdBuf + 1);
    if (d > -998) {
      setRefLevel(d);
    }
    return;
  }
  if (cmdBuf[0] == '/') {
    // Log comment
    static char commentBuf[80];
    strncpy(commentBuf, cmdBuf + 1, 75);
    logComment = commentBuf;
    return;
  }
  if ((cmdBuf[0] | ' ') == 'q' && cmdBuf[1] == '\0') {
    hwExit();
    exit(0);
    return;
  }
  if ((cmdBuf[0] | ' ') == 'e' && cmdBuf[1] == '\0') {
    enabled = !enabled;
    logging = false;
    return;
  }
  if ((cmdBuf[0] | ' ') == 'l' && cmdBuf[1] == '\0') {
    logging = enabled && !logging;
    readback = logging ? loggingEnabled : loggingDisabled;
    return;
  }
  readback = malformed;
}

void cmdReset() {
  memset(cmdBuf, 0, 81);
  cmdCursor = 0;
}

int keyboardFuncEC(unsigned char key, int x, int y) {
  switch (key) {
  case '+':
  case '=':
     setFreqSpan(span / 2);
     break;
  case '-':
  case '_':
     setFreqSpan(span * 2);
     break;
  case '\\':
    singleShot = 1;
    break;
  default:
    return 0;
  }
  return 1;
}

void keyboardFunc(unsigned char key, int x, int y) {
  if (cmdBuf[0] == '\0') {
    if (keyboardFuncEC(key, x, y)) {
      glutPostRedisplay();
      return;
    }
  }
  if (key == 8) {
    if (cmdCursor == 0) return;
    int i = --cmdCursor;
    for (;;) {
      cmdBuf[i] = cmdBuf[i+1];
      if (cmdBuf[i] == '\0') break;
      ++i;
    }
  } else if (key == '\r' || key == '\n') {
    handleCmd();
    cmdReset();
  } else if (cmdCursor == 80) {
    // Overflow banned.
  } else {
    int i = cmdCursor;
    //for (;;) {
    //  if (cmdBuf[i] == '\0') break;
    //  cmdBuf[i+1]
    //}
    cmdBuf[cmdCursor++] = key;
  }
  glutPostRedisplay();
}

int specialFuncEC(int key, int x, int y) {
  switch (key) {
  case GLUT_KEY_LEFT:
    setFreqCentre(centre - span / 10);
    break;
  case GLUT_KEY_RIGHT:
    setFreqCentre(centre + span / 10);
    break;
   case GLUT_KEY_UP:
    setRefLevel(refLevel + 10);
    break;
   case GLUT_KEY_DOWN:
    setRefLevel(refLevel - 10);
    break;
  default:
    return 0;
  }
  return 1;
}

void specialFunc(int key, int x, int y) {
  if (cmdBuf[0] == '\0') {
    if (specialFuncEC(key, x, y)) {
      glutPostRedisplay();
      return;
    }
  }
  switch (key) {
  case GLUT_KEY_LEFT:
    if (cmdCursor > 0) --cmdCursor;
    break;
  case GLUT_KEY_RIGHT:
    if (cmdBuf[cmdCursor] != '\0') ++cmdCursor;
    break;
   case GLUT_KEY_HOME:
    cmdCursor = 0;
    break;
   case GLUT_KEY_END:
    cmdCursor = strlen(cmdBuf);
    break;

   default:
    return;
  }
  glutPostRedisplay();
}
