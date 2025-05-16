/* Relative Path: src/c/rendering/heatmap_renderer.c */
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/rendering/heatmap_renderer.h"

static const char *VS_SRC =
"#version 300 es\n"
"layout(location=0) in vec2 aPos;\n"
"layout(location=1) in float aVal;\n"
"out float vVal;\n"
"void main(){\n"
"   gl_Position = vec4(aPos, 0.0, 1.0);\n"
"   gl_PointSize = 150.0;\n"  // Larger point size for smooth blending
"   vVal = aVal;\n"
"}\n";

static const char *FS_SRC =
"#version 300 es\n"
"precision highp float;\n"
"in float vVal;\n"
"out vec4 FragColor;\n"
"\n"
"float jet_r(float x) {\n"
"  return clamp(1.5 - abs(4.0 * x - 3.0), 0.0, 1.0);\n"
"}\n"
"float jet_g(float x) {\n"
"  return clamp(1.5 - abs(4.0 * x - 2.0), 0.0, 1.0);\n"
"}\n"
"float jet_b(float x) {\n"
"  return clamp(1.5 - abs(4.0 * x - 1.0), 0.0, 1.0);\n"
"}\n"
"\n"
"void main() {\n"
"  float d = length(gl_PointCoord - vec2(0.5));\n"
"  if (d > 0.2) discard;\n" // Circular cutoff
"  float sigma = 0.1;\n"    // Optimal diffusion
"  float alpha = exp(-d * d / (2.0 * sigma * sigma));\n"
"  float r = jet_r(vVal);\n"
"  float g = jet_g(vVal);\n"
"  float b = jet_b(vVal);\n"
"  FragColor = vec4(r, g, b, 1.0) * alpha;\n"
"}\n";

struct HeatmapRenderer {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    GLuint prog, vbo;
    int W, H;
};

static GLuint compile(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "[shader] compile error: %s\n", log);
        return 0;
    }
    return s;
}

static GLuint link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, NULL, log);
        fprintf(stderr, "[link] error: %s\n", log);
        return 0;
    }
    return p;
}

HeatmapRenderer *hr_create(int width, int height) {
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    EGLint pbuffer_attribs[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_NONE,
    };

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, NULL, NULL);

    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(dpy, config_attribs, &config, 1, &num_config);

    EGLSurface surface = eglCreatePbufferSurface(dpy, config, pbuffer_attribs);
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, context_attribs);
    eglMakeCurrent(dpy, surface, surface, ctx);

    GLuint vs = compile(GL_VERTEX_SHADER, VS_SRC);
    GLuint fs = compile(GL_FRAGMENT_SHADER, FS_SRC);
    GLuint prog = link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    HeatmapRenderer *hr = calloc(1, sizeof(*hr));
    hr->display = dpy;
    hr->context = ctx;
    hr->surface = surface;
    hr->prog = prog;
    hr->vbo = vbo;
    hr->W = width;
    hr->H = height;
    return hr;
}

int hr_render(HeatmapRenderer *hr,
              const float *pts,
              const float *val,
              size_t n,
              unsigned char *out_rgba,
              int W,
              int H)
{
    if (!hr || !pts || !val || !out_rgba) return -1;
    eglMakeCurrent(hr->display, hr->surface, hr->surface, hr->context);
    glViewport(0, 0, W, H);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(hr->prog);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glBindBuffer(GL_ARRAY_BUFFER, hr->vbo);
    glBufferData(GL_ARRAY_BUFFER, n * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    float *dst = glMapBufferRange(GL_ARRAY_BUFFER, 0, n * 3 * sizeof(float), GL_MAP_WRITE_BIT);
    if (!dst) return -2;
    for (size_t i = 0; i < n; ++i) {
        dst[3*i+0] = pts[2*i+0];
        dst[3*i+1] = pts[2*i+1];
        dst[3*i+2] = val[i];
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_POINTS, 0, (GLsizei)n);

    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba);
    return 0;
}

void hr_destroy(HeatmapRenderer *hr) {
    if (!hr) return;
    glDeleteBuffers(1, &hr->vbo);
    glDeleteProgram(hr->prog);
    eglDestroyContext(hr->display, hr->context);
    eglDestroySurface(hr->display, hr->surface);
    eglTerminate(hr->display);
    free(hr);
}
