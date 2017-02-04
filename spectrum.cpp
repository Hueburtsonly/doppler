#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "images/monospace.h"

#include "spectrum_cmd.cpp"

#define SHADER_SOURCE(text) "#version 130\n" #text

const char* vertexSource = SHADER_SOURCE(
  in vec4 position;
  uniform vec2 windowSize;
  uniform vec2 origin;

  out vec2 relative;

  void main() {
    gl_Position = vec4((position.xy / windowSize)*vec2(2.0,-2.0)+vec2(-1.0,1.0), 0.0, 1.0);
    relative = vec2(position.x - origin.x, origin.y - position.y);
  }
);

const char* plotFragmentSource = SHADER_SOURCE(
  out vec4 fragColor;
  in  vec2 relative;
  uniform sampler2D data;
  uniform vec2 yAxis; // plotHeight, tickSpacing  stpq

  void main() {
    if ((relative.x < 0 && relative.x >= -1) || relative.y < 0) {
      fragColor = vec4(0.0, 0.6, 0.2, 1.0);
    } else {
      float fromRef = yAxis.s - relative.y;
      bool onGrid = mod(fromRef, yAxis.t) < 1.0;
      if (relative.x >= 0) {
  vec2 range = texelFetch(data, ivec2(relative.x,1), 0).rg * 1024.0; // from top, small then large
  //  range = vec2(0.125, 0.5) * 1024.0;
  if (fromRef > range.s && fromRef < range.t) {
    fragColor = vec4(0.0, 1.0, 0.3, 1.0);
    return;
  }
      }
        fragColor = onGrid ? vec4(0.0, 0.6, 0.2, 1.0) : vec4(0.0);


      //      fragColor = vec4(sin(relative.x*2.03*3.141596)*0.5+0.5, cos(relative.y*2.03*3.141596)*0.5+0.5, 0.0, 1.0) * vec4(range.rrr, 1.0);
      //
      //vec4 texel = texelFetch(data, ivec2(round(relative.x), 1), 0);
      //}
    }
  }
);

const char* textFragmentSource = SHADER_SOURCE(
  out vec4 fragColor;
  in  vec2 relative;
  uniform sampler2D font;
  uniform sampler1D text;
  uniform ivec4 params; // char width, char height, str len, cursor?  stpq


  void main() {
    int fromRef = params.t - 1 - int(relative.y);
    int index = int(relative.x) / int(params.s);
    int px = int(relative.x) % int(params.s);
    int chr = int(256*texelFetch(text, index, 0).r);

    int cx = chr % 16;
    int cy = chr / 16;

    if (params.q * params.s == int(relative.x)) {
      fragColor = vec4(1.0);
      return;
    }
    fragColor = texelFetch(font, ivec2(16*cx + px, 16*cy + int(fromRef)), 0).rrrr + vec4(0.1, 0.2, 0.3, 0.7);
    //fragColor = vec4(relative.xy * 0.1, 0.0, 1.0);
  }
);

static int pPlot, pText;
static int upWindowSize;
static int upOrigin;
static int upData;
static int upYAxis;
static int utWindowSize;
static int utOrigin;
static int utFont;
static int utText;
static int utParams;
static GLuint texData, texFont, texText;
static GLuint sampler;



static void glCompileShaderWithCheck(int shader) {
  glCompileShader(shader);
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE) {
    printf("Compile successful.\r\n");
    return;
  }
  char buf[2000];
  GLsizei len;
  glGetShaderInfoLog(shader, 1999, &len, buf);
  printf("Compile failed:\r\n\r\n%s\r\n\r\n", buf);
  exit(1);
}

static float datums[4096*4];

void setupShaders(void) {
  int vs, pfs, tfs;
  vs = glCreateShader(GL_VERTEX_SHADER);
  pfs = glCreateShader(GL_FRAGMENT_SHADER);
  tfs = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vs, 1, &vertexSource, NULL);
  glShaderSource(pfs, 1, &plotFragmentSource, NULL);
  glShaderSource(tfs, 1, &textFragmentSource, NULL);

  glCompileShaderWithCheck(vs);
  glCompileShaderWithCheck(pfs);
  glCompileShaderWithCheck(tfs);

  pPlot = glCreateProgram();
  glAttachShader(pPlot, vs);
  glAttachShader(pPlot, pfs);
  glLinkProgram(pPlot);

  upWindowSize = glGetUniformLocation(pPlot, "windowSize");
  upOrigin = glGetUniformLocation(pPlot, "origin");
  upData = glGetUniformLocation(pPlot, "data");
  upYAxis = glGetUniformLocation(pPlot, "yAxis");
  glGenTextures(1, &texData);
  glGenSamplers(1, &sampler);


  pText = glCreateProgram();
  glAttachShader(pText, vs);
  glAttachShader(pText, tfs);
  glLinkProgram(pText);

  utWindowSize = glGetUniformLocation(pText, "windowSize");
  utOrigin = glGetUniformLocation(pText, "origin");
  utFont = glGetUniformLocation(pText, "font");
  utText = glGetUniformLocation(pText, "text");
  utParams = glGetUniformLocation(pText, "params");
  glGenTextures(1, &texFont);
  glGenTextures(1, &texText);

  glBindTexture(GL_TEXTURE_2D, texFont);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, bitmapMonospace);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int windowHeight, windowWidth;

#define FH 16
#define FW 8

void drawText(int x, int y, const char* text, int len, int borders, int bordert, int borderb, int cursor) {
  glGetError();

  glUseProgram(pText);
  glUniform2f(utWindowSize, windowWidth, windowHeight);
  glUniform2f(utOrigin, x, y);
  glUniform4i(utParams, FW, FH, len, cursor);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texFont);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glUniform1i(utFont, 1);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_1D, texText);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  static char buf[128];
  memset(buf, 0, 128);
  strncpy(buf, text, 128);


  glTexImage1D(GL_TEXTURE_1D, 0, GL_R8, 128, 0, GL_RED, GL_UNSIGNED_BYTE, buf);
  glUniform1i(utText, 2);



  glBegin(GL_POLYGON);
  float x1 = x - borders;
  float x2 = x + len * FW + borders;
  float y1 = y + borderb;
  float y2 = y - FH - bordert;
    glVertex3f(x1, y1, 0.0);
    glVertex3f(x2, y1, 0.0);
    glVertex3f(x2, y2, 0.0);
    glVertex3f(x1, y2, 0.0);
    glEnd();


}

void drawTextBlock(int x, int y, const char* block, int border) {
  int numLines = 0;
  int longestLine = 0;
  const char* it = block;
  const char* pit = block;
  while (*it) {
    if ('\n' == *it++) {
      ++numLines;
    }
  }

  int line;
  it = block;
  for (line = 0; line < numLines; line++) {
    const char* nit = strchr(it, '\n');
    drawText(x, y - ((numLines-1-line) * FH), it, nit-it, border, (line == 0) ? border : 0, (line == numLines - 1) ? border : 0, -1);
    it = nit + 1;
  }
  if (*it != '\0') {

    printf("Num lines = %d\r\n", numLines);
    printf("strlen = %ld\r\n", strlen(block));
    printf("it-block = %ld\r\n", it-block);

    drawText(x, y, "MISSING LINE TERM", 17, 0, 0, 0, -1);
  }
}

void displayMe(void) {
  windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
  windowWidth = glutGet(GLUT_WINDOW_WIDTH);

  // ------------------ onResize above




glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);




  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(100.45, 100.45, 0);


  glClear(GL_COLOR_BUFFER_BIT);


  glUseProgram(pPlot);
  glUniform2f(upWindowSize, windowWidth, windowHeight);
  glUniform2f(upOrigin, 50, windowHeight - 50);
  glUniform2f(upYAxis, windowHeight-100, (windowHeight-100)/10);

  glBindTexture(GL_TEXTURE_2D, texData);

  int i;
  for (i=0; i < 4096*2; i++) {
    datums[i] = (rand() & 0xffff) / (float)(0xffff);
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 2048, 2, 0, GL_RG, GL_FLOAT, datums);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);


  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texData);
  glUniform1i(upData, 0);

  glBegin(GL_POLYGON);
  float x1 = 49 - 10;
  float x2 = windowWidth - 49;
  float y1 = 49;
  float y2 = windowHeight - 49;
    glVertex3f(x1, y1, 0.0);
    glVertex3f(x2, y1, 0.0);
    glVertex3f(x2, y2, 0.0);
    glVertex3f(x1, y2, 0.0);
  glEnd();


  char spanBuf[160];
  sprintf(spanBuf, "Centre:%12.9f GHz   Start:%12.9f GHz\n  Span:%12.9f GHz    Stop:%12.9f GHz\n", centre / 1e9, (centre - span/2)/1e9, span / 1e9, (centre + span/2)/1e9);
  drawTextBlock(windowWidth / 2, windowHeight - 15, spanBuf, 0);

  sprintf(spanBuf, "Ref: %5.1f dBm\n", refLevel);
  drawTextBlock(windowWidth / 2, 31, spanBuf, 0);

  int cmdLen = strlen(cmdBuf);
  if (readback) {
    drawTextBlock(15, windowHeight - 45, readback, 5);
  }
  if (readback || cmdLen > 0) {
    drawText(15, windowHeight - 15, cmdBuf, cmdLen, 5, 5, 5, cmdCursor);
  }

  glutSwapBuffers();
  glFlush();
}

int main(int argc, char** argv) {

  cmdReset();

  glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(1000, 700);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Teh awesomes");
  glewInit();



   if (glewIsSupported("GL_VERSION_3_3"))
        printf("Ready for OpenGL 3.3\n");
    else {
        printf("OpenGL 3.3 not supported\n");
        //exit(1);
    }
  if (GLEW_ARB_fragment_shader)
    printf("Ready for GLSL\n");
  else {
    printf("Not totally ready :( \n");
    exit(1);
  }

  if (glewIsSupported("GL_VERSION_2_0"))
    printf("Ready for OpenGL 2.0\n");
  else {
    printf("OpenGL 2.0 not supported\n");
    exit(1);
  }



  glutDisplayFunc(displayMe);
  //  glutIdleFunc(displayMe);

  glutKeyboardFunc(keyboardFunc);
  glutSpecialFunc(specialFunc);
printf("%s\r\n", glGetString(GL_VENDOR));
printf("%s\r\n", glGetString(GL_RENDERER));
printf("%s\r\n", glGetString(GL_VERSION));
printf("%s\r\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  printf("%s\r\n", glGetString(GL_EXTENSIONS));

    usleep(50000);
    system("wmctrl -r :ACTIVE: -b add,maximized_vert,maximized_horz");

    setupShaders();

    glutMainLoop();
    return 0;
}



