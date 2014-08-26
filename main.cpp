#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>

EGLNativeWindowType native_win = 0;
static EGLDisplay   g_EGLDisplay;
static EGLConfig    g_EGLConfig;
static EGLContext   g_EGLContext;
static EGLSurface   g_EGLWindowSurface;

#define RLOG(format, ...) fprintf(stderr, format, ##__VA_ARGS__);

#define EGL_CHECK(x) x; egl_check(__FILE__, __LINE__)

static void
egl_check(const char* file, int line)
{
  EGLint error = eglGetError();
  if (error != EGL_SUCCESS) {
    RLOG("Error %s(%d):", file, line);
    switch (error) {
      case EGL_SUCCESS:
        RLOG("EGL_SUCCESS\n");
      break;
      case EGL_NOT_INITIALIZED:
        RLOG("EGL_NOT_INITIALIZED\n");
      break;
      case EGL_BAD_ACCESS:
        RLOG("EGL_BAD_ACCESS\n");
      break;
      case EGL_BAD_ALLOC:
        RLOG("EGL_BAD_ALLOC\n");
      break;
      case EGL_BAD_ATTRIBUTE:
        RLOG("EGL_BAD_ATTRIBUTE\n");
      break;
      case EGL_BAD_CONFIG:
        RLOG("EGL_BAD_CONFIG\n");
      break;
      case EGL_BAD_CONTEXT:
        RLOG("EGL_BAD_CONTEXT\n");
      break;
      case EGL_BAD_CURRENT_SURFACE:
        RLOG("EGL_BAD_CURRENT_SURFACE\n");
      break;
      case EGL_BAD_DISPLAY:
        RLOG("EGL_BAD_DISPLAY\n");
      break;
      case EGL_BAD_MATCH:
        RLOG("EGL_BAD_MATCH\n");
      break;
      case EGL_BAD_NATIVE_PIXMAP:
        RLOG("EGL_BAD_NATIVE_PIXMAP\n");
      break;
      case EGL_BAD_NATIVE_WINDOW:
        RLOG("EGL_BAD_NATIVE_WINDOW\n");
      break;
      case EGL_BAD_PARAMETER:
        RLOG("EGL_BAD_PARAMETER\n");
      break;
      case EGL_BAD_SURFACE:
        RLOG("EGL_BAD_SURFACE\n");
      break;
      case EGL_CONTEXT_LOST:
        RLOG("EGL_CONTEXT_LOST\n");
      break;
      default:
        RLOG("UNKNOWN: %d\n", error);
    }
  }
}

#define GL_CHECK(x) x; gl_check(__FILE__, __LINE__)

static void
gl_check(const char* file, int line)
{
  GLint error = glGetError();
  if (error != GL_NO_ERROR) {
    RLOG("GL Error %s(%d): ", file, line);
    switch(error) {
      case GL_INVALID_ENUM:
        RLOG("GL_INVALID_ENUM\n");
      break;
      case GL_INVALID_VALUE:
        RLOG("GL_INVALID_VALUE\n");
      break;
      case GL_INVALID_OPERATION:
        RLOG("GL_INVALID_OPERATION\n");
      break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        RLOG("GL_INVALID_FRAMEBUFFER_OPERATION\n");
      break;
      case GL_OUT_OF_MEMORY:
        RLOG("GL_OUT_OF_MEMORY\n");
      break;
      default:
        RLOG("UNKNOWN ERROR: %d\n", error);
    }
  }
}

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

  EGL_CHECK(eglInitialize(g_EGLDisplay, &majorVersion, &minorVersion));

  RLOG("EGL v%d.%d\n", (int)majorVersion, (int)minorVersion);

  EGL_CHECK(eglGetConfigs(g_EGLDisplay, NULL, 0, &numConfigs));
  EGL_CHECK(eglChooseConfig(g_EGLDisplay, s_configAttribs, &g_EGLConfig, 1, &numConfigs));

  g_EGLContext = EGL_CHECK(eglCreateContext(g_EGLDisplay, g_EGLConfig, NULL, NULL));

  g_EGLWindowSurface = EGL_CHECK(eglCreateWindowSurface(g_EGLDisplay, g_EGLConfig, native_win, NULL));

  int width;
  int height;
  EGL_CHECK(eglQuerySurface(g_EGLDisplay, g_EGLWindowSurface, EGL_WIDTH, &width));
  EGL_CHECK(eglQuerySurface(g_EGLDisplay, g_EGLWindowSurface, EGL_HEIGHT, &height));

  RLOG("Window: %d x %d\n", width, height);

  EGL_CHECK(eglMakeCurrent(g_EGLDisplay, g_EGLWindowSurface, g_EGLWindowSurface, g_EGLContext));

  GL_CHECK(glViewport(0, 0, width, height));

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
    RLOG("Failed to open file\n");
    exit(1);
  }

  GLuint textureY;
  GL_CHECK(glGenTextures(1, &textureY));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureY));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width, Height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanY));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GLuint textureU;
  GL_CHECK(glGenTextures(1, &textureU));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureU));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width / 2, Height /2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanU));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GLuint textureV;
  GL_CHECK(glGenTextures(1, &textureV));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureV));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width / 2, Height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanV));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GLuint vertexShader = GL_CHECK(glCreateShader(GL_VERTEX_SHADER));
  GL_CHECK(glShaderSource(vertexShader, 1, &vertexSource, NULL));
  GL_CHECK(glCompileShader(vertexShader));
  GLint status;
  GLint logLength;
  GL_CHECK(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
    char *buffer = new char[logLength + 1];
    glGetShaderInfoLog(vertexShader, logLength, NULL, buffer);
    RLOG("Vertex compiler error: %s\n", buffer);
    delete []buffer;
  }

  GLuint fragmentShader = GL_CHECK(glCreateShader(GL_FRAGMENT_SHADER));
  GL_CHECK(glShaderSource(fragmentShader, 1, &fragmentSource, NULL));
  GL_CHECK(glCompileShader(fragmentShader));
  GL_CHECK(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
    char *buffer = new char[logLength + 1];
    glGetShaderInfoLog(fragmentShader, logLength, NULL, buffer);
    RLOG("Fragment compiler error: %s\n", buffer);
    delete []buffer;
  }

  GLuint shaderProgram = GL_CHECK(glCreateProgram());
  GL_CHECK(glAttachShader(shaderProgram, vertexShader));
  GL_CHECK(glAttachShader(shaderProgram, fragmentShader));
  GL_CHECK(glLinkProgram(shaderProgram));
  GL_CHECK(glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status));
  if (status != GL_TRUE) {
    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
    char *buffer = new char[logLength + 1];
    glGetProgramInfoLog(shaderProgram, logLength, NULL, buffer);
    RLOG("Program error: %s\n", buffer);
    delete []buffer;
  }

  GL_CHECK(glUseProgram(shaderProgram));

  GLint posAttrib = GL_CHECK(glGetAttribLocation(shaderProgram, "position"));
  GLint texAttrib = GL_CHECK(glGetAttribLocation(shaderProgram, "texcoord"));
  GL_CHECK(glEnableVertexAttribArray(posAttrib));
  GL_CHECK(glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices));
  GL_CHECK(glEnableVertexAttribArray(texAttrib));
  GL_CHECK(glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord));
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureY));
  GL_CHECK(glActiveTexture(GL_TEXTURE1));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureU));
  GL_CHECK(glActiveTexture(GL_TEXTURE2));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureV));
  GLint texY = GL_CHECK(glGetUniformLocation(shaderProgram, "texY"));
  GL_CHECK(glUniform1i(texY, /*GL_TEXTURE*/0));
  GLint texU = GL_CHECK(glGetUniformLocation(shaderProgram, "texU"));
  GL_CHECK(glUniform1i(texU, /*GL_TEXTURE*/1));
  GLint texV = GL_CHECK(glGetUniformLocation(shaderProgram, "texV"));
  GL_CHECK(glUniform1i(texV, /*GL_TEXTURE*/2));

  bool done = false;

  while (!done) {
    GL_CHECK(glClearColor ( 0.0, 0.0, 0.0, 1.0 ));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
    GL_CHECK(eglSwapBuffers(g_EGLDisplay, g_EGLWindowSurface));
  }

  GL_CHECK(glDeleteProgram(shaderProgram));
  GL_CHECK(glDeleteShader(fragmentShader));
  GL_CHECK(glDeleteShader(vertexShader));

  return 0;
}
