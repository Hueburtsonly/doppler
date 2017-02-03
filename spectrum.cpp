#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

const char* vertexSource = "\
#version 130\r\n\
\r\n\
 \r\n\
in vec4 position;\r\n\
uniform vec2 windowSize;\r\n\
uniform vec2 plotCorner;\r\n\
 \r\n\
out vec2 plotCoord;\r\n\
 \r\n\
void main()                                                                             \r\n\
{                                                                                       \r\n\
gl_Position = vec4((position.xy / windowSize)*vec2(2.0,-2.0)+vec2(-1.0,1.0), 0.0, 1.0); \r\n\
plotCoord = vec2(position.x - plotCorner.x, plotCorner.y - position.y);                 \r\n\
}                                                                                       \r\n\
";

const char* fragmentSource = "\
#version 130\r\n\
#define M_PI 3.141596\r\n\
\r\n\
out vec4 fragColor;\r\n\
in  vec2 plotCoord;\r\n\
uniform sampler2D datass;\r\n\
\r\n\
void main()\r\n\
{\r\n\
    if (plotCoord.x < 0 || plotCoord.y < 0) {\
  fragColor = vec4(0.0, 0.0, 0.2, 1.0);\
} else {\
//fragColor = vec4(0.0, 1.0, 0.0, 0.0);\r\n\
fragColor = texture2D(datass, vec2(0.5, 0.5)); \r\n\
//    fragColor = texelFetch(datass, ivec2(1,1), 0);\r\n\
//vec4 texel = vec4(1.0, 0.0, 0.0, 0.0);\r\n\
  //  fragColor = vec4(sin(plotCoord.x*2.03*M_PI)/2.0+0.5, cos(plotCoord.y*2.03*M_PI)/2.0+0.5, 0.0, 1.0) * vec4(texel.rrr, 1.0);\r\n\
} \
}\r\n\
";

static int vs, fs, p;
static int uWindowSize;
static int uPlotCorner;
static int uData;
static GLuint texData;
static GLuint sampler;

static uint8_t datums[4096*4];

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



void setupShaders(void) {
  vs = glCreateShader(GL_VERTEX_SHADER);
  fs = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vs, 1, &vertexSource, NULL);
  glShaderSource(fs, 1, &fragmentSource, NULL);

  glCompileShaderWithCheck(vs);
  glCompileShaderWithCheck(fs);

  p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  glUseProgram(p);
  uWindowSize = glGetUniformLocation(p, "windowSize");
  uPlotCorner = glGetUniformLocation(p, "plotCorner");
  uData = glGetUniformLocation(p, "datass");

  glGenTextures(1, &texData);
  glGenSamplers(1, &sampler);


  int i;
  for (i=0; i < 4096*4; i++) {
    datums[i] = 0x7f;
  }
}



void displayMe(void) {
  int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
  int windowWidth = glutGet(GLUT_WINDOW_WIDTH);

  glGetError();
  
  glUniform2f(uWindowSize, windowWidth, windowHeight);
  glUniform2f(uPlotCorner, 50, windowHeight - 50);


  glBindTexture(GL_TEXTURE_2D, texData);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, datums);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);


  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texData);
  glUniform1i(uData, 0);

  //glBindSampler(3, sampler) ;
  printf("Error: %d, btw %d\r\n", glGetError(), uData);
  

  // ------------------ onResize above
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(100.45, 100.45, 0);

  
  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_POLYGON);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(windowWidth, 0.0, 0.0);
  glVertex3f(windowWidth, windowHeight, 0.0);
  glVertex3f(0.0, windowHeight, 0.0);
  glEnd();
  glBegin(GL_POLYGON);
  //  glVertex3f(0.0, 0.0, 1.0);
  // glVertex3f(-0.9, 0.0, 1.0);
  //glVertex3f(-0.9, -0.9, 1.0);
  //glVertex3f(0.0, -0.9, 1.0);
  glEnd();
  glFlush();
  glutSwapBuffers();
}

int main(int argc, char** argv) {
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



