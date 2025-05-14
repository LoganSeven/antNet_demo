#ifndef __GLES3_GL3_STUB_H__
#define __GLES3_GL3_STUB_H__
#include <stddef.h>  

#include "gl2.h"  // GLES3 is a superset of GLES2
typedef ptrdiff_t GLsizeiptr;

#ifdef __cplusplus
extern "C" {
#endif

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_ARRAY_BUFFER    0x8892
#define GL_DYNAMIC_DRAW    0x88E8
#define GL_FLOAT           0x1406
#define GL_FALSE           0

typedef char GLchar;

GLuint glCreateShader(GLenum type);
void   glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
void   glCompileShader(GLuint shader);
void   glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
void   glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void   glDeleteShader(GLuint shader);

GLuint glCreateProgram(void);
void   glAttachShader(GLuint program, GLuint shader);
void   glLinkProgram(GLuint program);
void   glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void   glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void   glDeleteProgram(GLuint program);

void glGenBuffers(GLsizei n, GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
void* glMapBuffer(GLenum target, GLenum access);
GLboolean glUnmapBuffer(GLenum target);

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
void glEnableVertexAttribArray(GLuint index);
void glPointSize(GLfloat size);

#ifdef __cplusplus
}
#endif
#endif /* __GLES3_GL3_STUB_H__ */
