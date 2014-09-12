#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include "prtime.h"

static EGLNativeWindowType sNativeWin = 0;
static EGLDisplay sEGLDisplay;
static EGLConfig sEGLConfig;
static EGLContext sEGLContext;
static EGLSurface sEGLWindowSurface;
static GLuint sTextureY;
static GLuint sTextureU;
static GLuint sTextureV;
static GLuint sVertexShader;
static GLuint sFragmentShader;
static GLuint sShaderProgram;
static int sWidth;
static int sHeight;
static GLint sPosAttrib;
static PRTime sLastUpdate;

static GLfloat sVertices[] = {
  -1.0f, -1.0f,
  1.0f, -1.0f,
  1.0f, 1.0f,
  -1.0f, 1.0f
};

static GLfloat sTexcoord[] = {
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

  sEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGL_CHECK(eglInitialize(sEGLDisplay, &majorVersion, &minorVersion));

  RLOG("EGL v%d.%d\n", (int)majorVersion, (int)minorVersion);

  EGL_CHECK(eglGetConfigs(sEGLDisplay, NULL, 0, &numConfigs));
  EGL_CHECK(eglChooseConfig(sEGLDisplay, configAttribs, &sEGLConfig, 1, &numConfigs));

  sEGLContext = EGL_CHECK(eglCreateContext(sEGLDisplay, sEGLConfig, EGL_NO_CONTEXT, contextAttribs));

  sEGLWindowSurface = EGL_CHECK(eglCreateWindowSurface(sEGLDisplay, sEGLConfig, sNativeWin, NULL));

  EGL_CHECK(eglQuerySurface(sEGLDisplay, sEGLWindowSurface, EGL_WIDTH, &sWidth));
  EGL_CHECK(eglQuerySurface(sEGLDisplay, sEGLWindowSurface, EGL_HEIGHT, &sHeight));

  RLOG("Window: %d x %d\n", sWidth, sHeight);

  EGL_CHECK(eglMakeCurrent(sEGLDisplay, sEGLWindowSurface, sEGLWindowSurface, sEGLContext));

  GLboolean hasCompiler = GL_FALSE;
  //GL_CHECK(glGetBooleanv(GL_SHADER_COMPILER, &hasCompiler));
  RLOG("Has compiler: %s\n", (hasCompiler == GL_TRUE ? "True" : "False"));

  GL_CHECK(glViewport(0, 0, sWidth, sHeight));

  sVertexShader = GL_CHECK(glCreateShader(GL_VERTEX_SHADER));
  GL_CHECK(glShaderSource(sVertexShader, 1, &vertexSource, NULL));
  GL_CHECK(glCompileShader(sVertexShader));
  GLint status;
  GL_CHECK(glGetShaderiv(sVertexShader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    GLint logLength = 0;
    GL_CHECK(glGetShaderiv(sVertexShader, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      char *buffer = new char[logLength + 1];
      GL_CHECK(glGetShaderInfoLog(sVertexShader, logLength, NULL, buffer));
      RLOG("Vertex compiler error[%d]: %s\n", (int)logLength, buffer);
      delete []buffer;
    }
    else {
      RLOG("Vertex compiler error: No log available.\n");
    }
  }

  sFragmentShader = GL_CHECK(glCreateShader(GL_FRAGMENT_SHADER));
  GL_CHECK(glShaderSource(sFragmentShader, 1, &fragmentSource, NULL));
  GL_CHECK(glCompileShader(sFragmentShader));
  GL_CHECK(glGetShaderiv(sFragmentShader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    GLint logLength = 0;
    GL_CHECK(glGetShaderiv(sFragmentShader, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      char *buffer = new char[logLength + 1];
      GL_CHECK(glGetShaderInfoLog(sFragmentShader, logLength, NULL, buffer));
      RLOG("Fragment compiler error[%d]: %s\n", (int)logLength, buffer);
      delete []buffer;
    }
    else {
      RLOG("Fragment compiler error: No log available.\n");
    }
  }

  sShaderProgram = GL_CHECK(glCreateProgram());
  GL_CHECK(glAttachShader(sShaderProgram, sVertexShader));
  GL_CHECK(glAttachShader(sShaderProgram, sFragmentShader));
  GL_CHECK(glLinkProgram(sShaderProgram));
  GL_CHECK(glGetProgramiv(sShaderProgram, GL_LINK_STATUS, &status));
  if (status != GL_TRUE) {
    GLint logLength = 0;
    GL_CHECK(glGetProgramiv(sShaderProgram, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      char *buffer = new char[logLength + 1];
      GL_CHECK(glGetProgramInfoLog(sShaderProgram, logLength, NULL, buffer));
      RLOG("Program error[%d]: %s\n", (int)logLength, buffer);
      delete []buffer;
    }
    else {
      RLOG("Program link error: No log available.\n");
    }
  }

  GL_CHECK(glUseProgram(sShaderProgram));

  GL_CHECK(glGenTextures(1, &sTextureY));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureY));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GL_CHECK(glGenTextures(1, &sTextureU));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureU));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GL_CHECK(glGenTextures(1, &sTextureV));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureV));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  sPosAttrib = GL_CHECK(glGetAttribLocation(sShaderProgram, "position"));
  GLint texAttrib = GL_CHECK(glGetAttribLocation(sShaderProgram, "texcoord"));
  GL_CHECK(glEnableVertexAttribArray(sPosAttrib));
  GL_CHECK(glVertexAttribPointer(sPosAttrib, 2, GL_FLOAT, GL_FALSE, 0, sVertices));
  GL_CHECK(glEnableVertexAttribArray(texAttrib));
  GL_CHECK(glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, sTexcoord));
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureY));
  GL_CHECK(glActiveTexture(GL_TEXTURE1));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureU));
  GL_CHECK(glActiveTexture(GL_TEXTURE2));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureV));
  GLint texY = GL_CHECK(glGetUniformLocation(sShaderProgram, "texY"));
  GL_CHECK(glUniform1i(texY, 0));
  GLint texU = GL_CHECK(glGetUniformLocation(sShaderProgram, "texU"));
  GL_CHECK(glUniform1i(texU, 1));
  GLint texV = GL_CHECK(glGetUniformLocation(sShaderProgram, "texV"));
  GL_CHECK(glUniform1i(texV, 2));
}

void
Draw(const unsigned char* aImage, int size, int aWidth, int aHeight)
{
  RLOG("Got %d x %d size: %d\n", aWidth, aHeight, size);

  if ((aWidth > 0) && (aHeight > 0)) {

    float wRatio = (float)sWidth / (float)aWidth;
    float hRatio = (float)sHeight / (float)aHeight;

    float ratio = (wRatio < hRatio ? wRatio : hRatio);

    float cWidth = (float)aWidth * ratio / (float)sWidth;
    float cHeight = (float)aHeight * ratio / (float)sHeight;

//    RLOG("calculated: %f x %f\n", (float)aWidth * ratio / (float)sWidth, (float)aHeight * ratio / (float)sHeight);
//    RLOG("calculated mapped: %f x %f\n", cWidth, cHeight);
//    RLOG("cr: %f ar: %f\n", cWidth / cHeight, (float) aWidth / (float)aHeight);

    sVertices[0] = -cWidth; sVertices[1] = -cHeight;
    sVertices[2] =  cWidth; sVertices[3] = -cHeight;
    sVertices[4] =  cWidth; sVertices[5] = cHeight;
    sVertices[6] = -cWidth; sVertices[7] = cHeight;

    GL_CHECK(glEnableVertexAttribArray(sPosAttrib));
    GL_CHECK(glVertexAttribPointer(sPosAttrib, 2, GL_FLOAT, GL_FALSE, 0, sVertices));

    const unsigned char* chanY = aImage;
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureY));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, aWidth, aHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanY));

    const unsigned char* chanU = aImage + (aWidth * aHeight);
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureU));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, aWidth / 2, aHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanU));

    const unsigned char* chanV = aImage + (aWidth * aHeight) + (aWidth * aHeight / 4);
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, sTextureV));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, aWidth / 2, aHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, chanV));

    GL_CHECK(glClearColor ( 0.0, 0.0, 0.0, 1.0 ));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
    GL_CHECK(eglSwapBuffers(sEGLDisplay, sEGLWindowSurface));

    sLastUpdate = PR_Now();
  }
}

bool
KeepRunning()
{
  return (sLastUpdate == 0) || ((PR_Now() - sLastUpdate) < 5000000);
}

void
Shutdown()
{
  GL_CHECK(glDeleteProgram(sShaderProgram));
  GL_CHECK(glDeleteShader(sFragmentShader));
  GL_CHECK(glDeleteShader(sVertexShader));
}

}
