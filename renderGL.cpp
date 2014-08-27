#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>

EGLNativeWindowType native_win = 0;
static EGLDisplay   g_EGLDisplay;
static EGLConfig    g_EGLConfig;
static EGLContext   g_EGLContext;
static EGLSurface   g_EGLWindowSurface;
static GLuint textureY;
static GLuint textureU;
static GLuint textureV;
static GLuint vertexShader;
static GLuint fragmentShader;
static GLuint shaderProgram;

static GLfloat vertices[] = {
  -1.0f, -1.0f,
  1.0f, -1.0f,
  1.0f, 1.0f,
  -1.0f, 1.0f
};

static GLfloat texcoord[] = {
  0.0f, 1.0f,
  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f
};

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

const GLchar* fragmentSourceGray =
    "varying vec2 varTexcoord;"
    "uniform sampler2D texY;"
    "uniform sampler2D texU;"
    "uniform sampler2D texV;"
    "void main() {"
    "   vec4 c = texture2D(texY, varTexcoord);"
    "   gl_FragColor = vec4(c.r, c.r, c.r, 1.0);"
    "}";

const GLchar *fragmentSource =
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


namespace render {

void
Initialize()
{
  static const EGLint configAttribs[] = {
    EGL_RENDERABLE_TYPE,     EGL_OPENGL_ES2_BIT,
    EGL_BUFFER_SIZE,        32,
    EGL_RED_SIZE,       8,
    EGL_GREEN_SIZE,     8,
    EGL_BLUE_SIZE,      8,
    EGL_ALPHA_SIZE,     8,
    EGL_DEPTH_SIZE,     0,
    EGL_STENCIL_SIZE,   0,
    EGL_SAMPLE_BUFFERS, 0,
    EGL_NONE
  };

  static const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;

  g_EGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGL_CHECK(eglInitialize(g_EGLDisplay, &majorVersion, &minorVersion));

  RLOG("EGL v%d.%d\n", (int)majorVersion, (int)minorVersion);

  EGL_CHECK(eglGetConfigs(g_EGLDisplay, NULL, 0, &numConfigs));
  EGL_CHECK(eglChooseConfig(g_EGLDisplay, configAttribs, &g_EGLConfig, 1, &numConfigs));

  g_EGLContext = EGL_CHECK(eglCreateContext(g_EGLDisplay, g_EGLConfig, EGL_NO_CONTEXT, contextAttribs));

  g_EGLWindowSurface = EGL_CHECK(eglCreateWindowSurface(g_EGLDisplay, g_EGLConfig, native_win, NULL));

  int width;
  int height;
  EGL_CHECK(eglQuerySurface(g_EGLDisplay, g_EGLWindowSurface, EGL_WIDTH, &width));
  EGL_CHECK(eglQuerySurface(g_EGLDisplay, g_EGLWindowSurface, EGL_HEIGHT, &height));

  RLOG("Window: %d x %d\n", width, height);

  EGL_CHECK(eglMakeCurrent(g_EGLDisplay, g_EGLWindowSurface, g_EGLWindowSurface, g_EGLContext));

  GLboolean hasCompiler = GL_FALSE;
  //GL_CHECK(glGetBooleanv(GL_SHADER_COMPILER, &hasCompiler));
  RLOG("Has compiler: %s\n", (hasCompiler == GL_TRUE ? "True" : "False"));

  GL_CHECK(glViewport(0, 0, width, height));

  vertexShader = GL_CHECK(glCreateShader(GL_VERTEX_SHADER));
  GL_CHECK(glShaderSource(vertexShader, 1, &vertexSource, NULL));
  GL_CHECK(glCompileShader(vertexShader));
  GLint status;
  GL_CHECK(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    GLint logLength = 0;
    GL_CHECK(glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      char *buffer = new char[logLength + 1];
      GL_CHECK(glGetShaderInfoLog(vertexShader, logLength, NULL, buffer));
      RLOG("Vertex compiler error[%d]: %s\n", (int)logLength, buffer);
      delete []buffer;
    }
    else {
      RLOG("Vertex compiler error: No log available.\n");
    }
  }

  fragmentShader = GL_CHECK(glCreateShader(GL_FRAGMENT_SHADER));
  GL_CHECK(glShaderSource(fragmentShader, 1, &fragmentSource, NULL));
  GL_CHECK(glCompileShader(fragmentShader));
  GL_CHECK(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    GLint logLength = 0;
    GL_CHECK(glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      char *buffer = new char[logLength + 1];
      GL_CHECK(glGetShaderInfoLog(fragmentShader, logLength, NULL, buffer));
      RLOG("Fragment compiler error[%d]: %s\n", (int)logLength, buffer);
      delete []buffer;
    }
    else {
      RLOG("Fragment compiler error: No log available.\n");
    }
  }

  shaderProgram = GL_CHECK(glCreateProgram());
  GL_CHECK(glAttachShader(shaderProgram, vertexShader));
  GL_CHECK(glAttachShader(shaderProgram, fragmentShader));
  GL_CHECK(glLinkProgram(shaderProgram));
  GL_CHECK(glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status));
  if (status != GL_TRUE) {
    GLint logLength = 0;
    GL_CHECK(glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      char *buffer = new char[logLength + 1];
      GL_CHECK(glGetProgramInfoLog(shaderProgram, logLength, NULL, buffer));
      RLOG("Program error[%d]: %s\n", (int)logLength, buffer);
      delete []buffer;
    }
    else {
      RLOG("Program link error: No log available.\n");
    }
  }

  GL_CHECK(glUseProgram(shaderProgram));

  GL_CHECK(glGenTextures(1, &textureY));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureY));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GL_CHECK(glGenTextures(1, &textureU));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureU));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GL_CHECK(glGenTextures(1, &textureV));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureV));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

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
  GL_CHECK(glUniform1i(texY, 0));
  GLint texU = GL_CHECK(glGetUniformLocation(shaderProgram, "texU"));
  GL_CHECK(glUniform1i(texU, 1));
  GLint texV = GL_CHECK(glGetUniformLocation(shaderProgram, "texV"));
  GL_CHECK(glUniform1i(texV, 2));
}

void
Draw(const unsigned char* aImage, int size, int aWidth, int aHeight)
{

  RLOG("Got %d x %d size: %d\n", aWidth, aHeight, size);

  const unsigned char* chanY = aImage;
  glBindTexture(GL_TEXTURE_2D, textureY);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, aWidth, aHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanY);

  const unsigned char* chanU = aImage + (aWidth * aHeight);
  glBindTexture(GL_TEXTURE_2D, textureU);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, aWidth / 2, aHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanU);

  const unsigned char* chanV = aImage + (aWidth * aHeight) + (aWidth * aHeight / 4);
  glBindTexture(GL_TEXTURE_2D, textureV);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, aWidth / 2, aHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanV);

  GL_CHECK(glClearColor ( 0.0, 0.0, 0.0, 1.0 ));
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
  GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
  GL_CHECK(eglSwapBuffers(g_EGLDisplay, g_EGLWindowSurface));
}

bool
KeepRunning()
{
  return true;
}

void
Shutdown()
{
  GL_CHECK(glDeleteProgram(shaderProgram));
  GL_CHECK(glDeleteShader(fragmentShader));
  GL_CHECK(glDeleteShader(vertexShader));
}

}
