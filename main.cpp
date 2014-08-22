#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>

EGLNativeWindowType native_win = 0;
static EGLDisplay   g_EGLDisplay;
static EGLConfig    g_EGLConfig;
static EGLContext   g_EGLContext;
static EGLSurface   g_EGLWindowSurface;

const GLchar* vertexSource =
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 varTexcoord;\n"
    "void main() {\n"
    "   gl_Position = vec4(position, 0.0, 1.0);\n"
    "   varTexcoord = texcoord;\n"
    "}\n";

const GLchar* fragmentSourceGrey =
    "#version 120\n"
    "varying vec2 varTexcoord;"
    "uniform sampler2D texY;"
    "uniform sampler2D texU;"
    "uniform sampler2D texV;"
    "void main() {"
    "   vec4 c = texture2D(texY, varTexcoord);" //  * vec4(1.0, 0.0, 0.0, 1.0);"
    "   gl_FragColor = vec4(c.r, c.r, c.r, 1.0);" //texture2D(texture, varTexcoord);" //  * vec4(1.0, 0.0, 0.0, 1.0);"
    "}";

const GLchar *fragmentSource=
  "varying vec2 varTexcoord;\n"
  "uniform sampler2D texY;\n"
  "uniform sampler2D texU;\n"
  "uniform sampler2D texV;\n"
  "void main(void) {\n"
  "  float r,g,b,y,u,v;\n"
  "  y=texture2D(texY, varTexcoord).r;\n"
  "  u=texture2D(texU, varTexcoord).r;\n"
  "  v=texture2D(texV, varTexcoord).r;\n"

  "  y=1.1643*(y-0.0625);\n"
  "  u=u-0.5;\n"
  "  v=v-0.5;\n"

  "  r=y+1.5958*v;\n"
  "  g=y-0.39173*u-0.81290*v;\n"
  "  b=y+2.017*u;\n"

  "  gl_FragColor=vec4(r,g,b,1.0);\n"
  "}\n";


int main(int argc, char *argv[])
{
  const EGLint s_configAttribs[] =
  {
    EGL_DEPTH_SIZE,     0,
    EGL_NONE
  };


  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;

  g_EGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  eglInitialize(g_EGLDisplay, &majorVersion, &minorVersion);

fprintf(stderr, "OpenGL ES %d.%d\n", (int)majorVersion, (int)minorVersion);

  eglGetConfigs(g_EGLDisplay, NULL, 0, &numConfigs);
  eglChooseConfig(g_EGLDisplay, s_configAttribs, &g_EGLConfig, 1, &numConfigs);

  g_EGLContext = eglCreateContext(g_EGLDisplay, g_EGLConfig, NULL, NULL);

  g_EGLWindowSurface = eglCreateWindowSurface(g_EGLDisplay, g_EGLConfig, native_win, NULL);

  int width;
  int height;
  eglQuerySurface(g_EGLDisplay, g_EGLWindowSurface, EGL_WIDTH, &width);
  eglQuerySurface(g_EGLDisplay, g_EGLWindowSurface, EGL_HEIGHT, &height);

  eglMakeCurrent(g_EGLDisplay, g_EGLWindowSurface, g_EGLWindowSurface, g_EGLContext);

  glViewport(0, 0, width, height);
  GLfloat vertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f
  };

  GLfloat texcoord[] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f
  };

  const int YSize = 307200;
  const int UVSize = 76800;
  const int Width = 640;
  const int Height = 480;

  unsigned char chanY[YSize];
  unsigned char chanU[UVSize];
  unsigned char chanV[UVSize];

  FILE* file = fopen("pkg:/images/frame.i420", "r");
  if (file) {
    fread(chanY, YSize, 1, file);
    fread(chanU, UVSize, 1, file);
    fread(chanV, UVSize, 1, file);
    fclose(file);
    file = 0;
  }
  else {
    fprintf(stderr, "Failed to open file\n");
    exit(1);
  }

  GLuint textureY;
  glGenTextures(1, &textureY);
  glBindTexture(GL_TEXTURE_2D, textureY);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width, Height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanY);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLuint textureU;
  glGenTextures(1, &textureU);
  glBindTexture(GL_TEXTURE_2D, textureU);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width / 2, Height /2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanU);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLuint textureV;
  glGenTextures(1, &textureV);
  glBindTexture(GL_TEXTURE_2D, textureV);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width / 2, Height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanV);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);
  GLint status;
  GLint logLength;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
    char *buffer = new char[logLength + 1];
    glGetShaderInfoLog(vertexShader, logLength, NULL, buffer);
    fprintf(stderr, "Vertex compiler error: %s\n", buffer);
    delete []buffer;
  }

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
    char *buffer = new char[logLength + 1];
    glGetShaderInfoLog(fragmentShader, logLength, NULL, buffer);
    fprintf(stderr, "Fragment compiler error: %s\n", buffer);
    delete []buffer;
  }

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
    char *buffer = new char[logLength + 1];
    glGetProgramInfoLog(shaderProgram, logLength, NULL, buffer);
    fprintf(stderr, "Program error: %s\n", buffer);
    delete []buffer;
  }

  glUseProgram(shaderProgram);

  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureY);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, textureU);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, textureV);
  GLint texY = glGetUniformLocation(shaderProgram, "texY");
  glUniform1i(texY, /*GL_TEXTURE*/0);
  GLint texU = glGetUniformLocation(shaderProgram, "texU");
  glUniform1i(texU, /*GL_TEXTURE*/1);
  GLint texV = glGetUniformLocation(shaderProgram, "texV");
  glUniform1i(texV, /*GL_TEXTURE*/2);

  bool done = false;

  while (!done) {
    glClearColor ( 0.0, 0.0, 0.0, 1.0 );
    glClear ( GL_COLOR_BUFFER_BIT );
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    eglSwapBuffers(g_EGLDisplay, g_EGLWindowSurface);
  }

  glDeleteProgram(shaderProgram);
  glDeleteShader(fragmentShader);
  glDeleteShader(vertexShader);

  return 0;
}
