#ifndef __GLES2_GL2_STUB_H__
#define __GLES2_GL2_STUB_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef int GLint;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_POINTS 0x0000
#define GL_BLEND 0x0BE2
#define GL_ONE    1

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClear(GLbitfield mask);
void glUseProgram(GLuint program);
void glEnable(GLenum cap);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

#ifdef __cplusplus
}
#endif
#endif /* __GLES2_GL2_STUB_H__ */
