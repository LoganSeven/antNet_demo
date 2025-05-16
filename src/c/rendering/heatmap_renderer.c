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
"  if (d > 0.2) discard; // Circular cutoff\n"
"  float sigma = 0.1;    // Optimal diffusion\n"
"  float alpha = exp(-d * d / (2.0 * sigma * sigma));\n"
"  float r = jet_r(vVal);\n"
"  float g = jet_g(vVal);\n"
"  float b = jet_b(vVal);\n"
"  // This fragment color is partially transparent, so points can fade into each other.\n"
"  FragColor = vec4(r, g, b, alpha);\n"
"}\n";

/* Pass-through vertex shader for the final compositing step. */
static const char *COMPOSITE_VS_SRC =
"#version 300 es\n"
"layout(location=0) in vec2 aPos;\n"
"layout(location=1) in vec2 aTex;\n"
"out vec2 vTex;\n"
"void main(){\n"
"    vTex = aTex;\n"
"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
"}\n";

/* Fragment shader that reads the offscreen texture and enforces opacity if alpha>0. */
static const char *COMPOSITE_FS_SRC =
"#version 300 es\n"
"precision highp float;\n"
"in vec2 vTex;\n"
"out vec4 FragColor;\n"
"uniform sampler2D uTex;\n"
"void main(){\n"
"    vec4 c = texture(uTex, vTex);\n"
"    if (c.a > 0.0) {\n"
"       FragColor = vec4(c.rgb, 1.0);\n"
"    } else {\n"
"       discard;\n"
"    }\n"
"}\n";

/*
 * Internal data for the two-pass rendering:
 *   - display/context/surface for EGL
 *   - original point-drawing prog + VBO
 *   - offscreen FBO + color texture
 *   - composite program + quad VBO
 */
struct HeatmapRenderer {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;

    GLuint prog;   // For drawing points
    GLuint vbo;    // For point data
    int W, H;

    // 2-Pass infrastructure:
    GLuint fbo;        // Offscreen framebuffer
    GLuint colorTex;   // RGBA texture for pass 1
    GLuint compositeProg; // For compositing pass
    GLuint quadVBO;    // Fullscreen quad
    GLuint quadVAO;
};

/* Helper: compile a shader from source. */
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

/* Helper: link a program from vertex + fragment shaders. */
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

/* Creates an offscreen framebuffer and color texture for pass 1. */
static void createOffscreenFBO(HeatmapRenderer *hr, int width, int height) {
    glGenTextures(1, &hr->colorTex);
    glBindTexture(GL_TEXTURE_2D, hr->colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create an FBO and attach the texture
    glGenFramebuffers(1, &hr->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, hr->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, hr->colorTex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "[FBO] Incomplete framebuffer: 0x%x\n", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* Creates the simple composite program + fullscreen quad. */
static void createCompositeResources(HeatmapRenderer *hr) {
    // Compile & link
    GLuint vs = compile(GL_VERTEX_SHADER, COMPOSITE_VS_SRC);
    GLuint fs = compile(GL_FRAGMENT_SHADER, COMPOSITE_FS_SRC);
    hr->compositeProg = link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Fullscreen quad geometry: (pos.x, pos.y, tex.s, tex.t)
    float quadVertices[] = {
        //   X     Y     U     V
        -1.0f, -1.0f,  0.0f,  0.0f,
         1.0f, -1.0f,  1.0f,  0.0f,
        -1.0f,  1.0f,  0.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,
    };

    glGenVertexArrays(1, &hr->quadVAO);
    glGenBuffers(1, &hr->quadVBO);

    glBindVertexArray(hr->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hr->quadVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute -> location=0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texcoord attribute -> location=1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

HeatmapRenderer *hr_create(int width, int height) {
    /*
     * This config requests a multisample buffer. If the driver/hardware supports MSAA
     * in OpenGL ES, it will be handled automatically. No glEnable(GL_MULTISAMPLE) call
     * is needed in ES, since that constant may be unavailable.
     */
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_SAMPLE_BUFFERS,  1,
        EGL_SAMPLES,         4,
        EGL_NONE
    };

    EGLint pbuffer_attribs[] = {
        EGL_WIDTH,  width,
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

    // Compile point-drawing program
    GLuint vs = compile(GL_VERTEX_SHADER, VS_SRC);
    GLuint fs = compile(GL_FRAGMENT_SHADER, FS_SRC);
    GLuint prog = link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Create VBO for point data
    GLuint vbo;
    glGenBuffers(1, &vbo);

    HeatmapRenderer *hr = (HeatmapRenderer*)calloc(1, sizeof(*hr));
    hr->display = dpy;
    hr->context = ctx;
    hr->surface = surface;
    hr->prog = prog;
    hr->vbo = vbo;
    hr->W = width;
    hr->H = height;

    // Create offscreen pass (FBO + texture)
    createOffscreenFBO(hr, width, height);

    // Create composite pass resources
    createCompositeResources(hr);

    // No explicit glEnable(GL_MULTISAMPLE) needed for ES3.

    return hr;
}

/*
 * hr_render
 * Performs a two-pass rendering of points:
 *  1) Renders the heatmap points offscreen with normal alpha blending,
 *     so they fade among themselves. The offscreen background is transparent.
 *  2) Composites that offscreen result to the main surface, forcing full
 *     opacity where alpha>0 so the background color does not bleed.
 * Finally reads pixels back into out_rgba.
 */
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

    /* PASS 1: offscreen */
    glBindFramebuffer(GL_FRAMEBUFFER, hr->fbo);
    glViewport(0, 0, W, H);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(hr->prog);
    glBindBuffer(GL_ARRAY_BUFFER, hr->vbo);
    glBufferData(GL_ARRAY_BUFFER, n * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    float *dst = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, n * 3 * sizeof(float), GL_MAP_WRITE_BIT);
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

    /* PASS 2: composite to main surface */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, W, H);

    glClearColor(0.02f, 0.02f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_BLEND);

    glUseProgram(hr->compositeProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hr->colorTex);

    GLint loc = glGetUniformLocation(hr->compositeProg, "uTex");
    glUniform1i(loc, 0);

    glBindVertexArray(hr->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba);

    return 0;
}

/*
 * hr_destroy
 * Releases all GPU and EGL resources, then frees the HeatmapRenderer struct.
 */
void hr_destroy(HeatmapRenderer *hr) {
    if (!hr) return;

    glDeleteBuffers(1, &hr->vbo);
    glDeleteProgram(hr->prog);
    glDeleteProgram(hr->compositeProg);
    glDeleteBuffers(1, &hr->quadVBO);
    glDeleteVertexArrays(1, &hr->quadVAO);
    glDeleteFramebuffers(1, &hr->fbo);
    glDeleteTextures(1, &hr->colorTex);

    eglDestroyContext(hr->display, hr->context);
    eglDestroySurface(hr->display, hr->surface);
    eglTerminate(hr->display);

    free(hr);
}
