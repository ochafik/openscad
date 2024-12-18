//#include "OpenGLContext.h"

// #include "system-gl.h"
#include <GL/gl.h>
#include <GL/glu.h>

// #include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <math.h>

// // /emsdk/upstream/bin/llvm-nm /emsdk/upstream/emscripten/cache/sysroot/lib/wasm32-emscripten/libregal.a | grep CheckFramebufferStatusEXT

extern "C" {
    
extern const GLubyte *rglErrorStringREGAL(GLenum error);
const GLubyte *gluErrorString(GLenum error) {
    return rglErrorStringREGAL(error);
}

extern GLenum rglCheckFramebufferStatusEXT(GLenum target);
GLenum glCheckFramebufferStatusEXT(GLenum target) {
    return rglCheckFramebufferStatusEXT(target);    
}

extern void rglBindFramebufferEXT(GLenum target, GLuint framebuffer);
void glBindFramebufferEXT(GLenum target, GLuint framebuffer) {
    return rglBindFramebufferEXT(target, framebuffer);    
}

extern void rglBindRenderbufferEXT(GLenum target, GLuint renderbuffer);
void glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) {
    return rglBindRenderbufferEXT(target, renderbuffer);    
}

extern void rglFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    return rglFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);    
}

extern void rglGenFramebuffersEXT(GLsizei n, GLuint *framebuffers);
void glGenFramebuffersEXT(GLsizei n, GLuint *framebuffers) {
    return rglGenFramebuffersEXT(n, framebuffers);    
}

extern void rglGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers);
void glGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers) {
    return rglGenRenderbuffersEXT(n, renderbuffers);    
}

extern void rglRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    return rglRenderbufferStorageEXT(target, internalformat, width, height);    
}

extern void rglColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
    return glColor3f(red, green, blue);
}

extern void rglGetDoublev(GLenum pname, GLdouble *data);
void glGetDoublev(GLenum pname, GLdouble *data) {
    return glGetDoublev(pname, data);
}

extern void rglOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    return glOrtho(left, right, bottom, top, zNear, zFar);
}

extern void rglRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
    return glRotated(angle, x, y, z);
}

extern void rglTranslated(GLdouble x, GLdouble y, GLdouble z);
void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
    return glTranslated(x, y, z);
}

extern void rglColor4v(const GLfloat *v);
void glColor4v(const GLfloat *v) {
    return rglColor4v(v);
}

extern void rglEnableClientState(GLenum cap);
void glEnableClientState(GLenum cap) {
    return rglEnableClientState(cap);
}

extern void rglDisableClientState(GLenum cap);
void glDisableClientState(GLenum cap) {
    return rglDisableClientState(cap);
}

extern void rglNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    return rglNormalPointer(type, stride, pointer);
}

extern void rglColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    return rglColorPointer(size, type, stride, pointer);
}

extern void rglVertex2f(GLfloat x, GLfloat y);
void glVertex2f(GLfloat x, GLfloat y) {
    return rglVertex2f(x, y);
}

extern void rglVertex3d(GLdouble x, GLdouble y, GLdouble z);
void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
    return rglVertex3d(x, y, z);
}

extern void rglEnd();
void glEnd() {
    return rglEnd();
}

extern void rglBegin(GLenum mode);
void glBegin(GLenum mode) {
    return rglBegin(mode);
}

extern void rglPushAttrib(GLbitfield mask);
void glPushAttrib(GLbitfield mask) {
    return rglPushAttrib(mask);
}

extern void rglPopAttrib();
void glPopAttrib() {
    return rglPopAttrib();
}

extern void rglLineStipple(GLint factor, GLushort pattern);
void glLineStipple(GLint factor, GLushort pattern) {
    return rglLineStipple(factor, pattern);
}

extern void rglVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    return rglVertex4d(x, y, z, w);
}

extern void rglColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
    return rglColor4d(red, green, blue, alpha);
}

extern void rglColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    return rglColor4f(red, green, blue, alpha);
}

extern void rglColor3d(GLdouble red, GLdouble green, GLdouble blue);
void glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
    return rglColor3d(red, green, blue);
}

// // gluPerspective implementation
// glm::mat4 gluPerspective(float fovy, float aspect, float zNear, float zFar) {
//     float tanHalfFovy = tan(fovy / 2.0f);
    
//     glm::mat4 proj = glm::mat4(0.0f);
//     proj[0][0] = 1.0f / (aspect * tanHalfFovy);
//     proj[1][1] = 1.0f / tanHalfFovy;
//     proj[2][2] = -(zFar + zNear) / (zFar - zNear);
//     proj[2][3] = -1.0f;
//     proj[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    
//     return proj;
// }

// // gluLookAt implementation
// glm::mat4 gluLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
//     glm::vec3 f = glm::normalize(center - eye);
//     glm::vec3 s = glm::normalize(glm::cross(f, up));
//     glm::vec3 u = glm::cross(s, f);

//     glm::mat4 view = glm::mat4(1.0f);
//     view[0][0] = s.x;
//     view[1][0] = s.y;
//     view[2][0] = s.z;
//     view[0][1] = u.x;
//     view[1][1] = u.y;
//     view[2][1] = u.z;
//     view[0][2] = -f.x;
//     view[1][2] = -f.y;
//     view[2][2] = -f.z;
//     view[3][0] = -glm::dot(s, eye);
//     view[3][1] = -glm::dot(u, eye);
//     view[3][2] = glm::dot(f, eye);

//     return view;
// }


// // gluProject implementation
// bool gluProject(glm::vec3 obj, 
//                 glm::mat4 model, 
//                 glm::mat4 proj, 
//                 glm::vec4 viewport, 
//                 glm::vec3& winCoord) {
//     // Perform the model-view-projection transformation
//     glm::vec4 tmp = proj * model * glm::vec4(obj, 1.0f);
    
//     // Perform perspective division
//     if (tmp.w == 0.0f) {
//         return false;  // Invalid transformation
//     }
    
//     tmp /= tmp.w;
    
//     // Map to viewport
//     winCoord.x = viewport[0] + viewport[2] * (tmp.x + 1.0f) / 2.0f;
//     winCoord.y = viewport[1] + viewport[3] * (tmp.y + 1.0f) / 2.0f;
//     winCoord.z = (tmp.z + 1.0f) / 2.0f;
    
//     return true;
// }


#define __glPi 3.14159265358979323846

static void __gluMakeIdentityd(GLdouble m[16])
{
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

static void __gluMakeIdentityf(GLfloat m[16])
{
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

static void normalize(float v[3])
{
    float r;

    r = sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
    if (r == 0.0) return;

    v[0] /= r;
    v[1] /= r;
    v[2] /= r;
}

static void cross(float v1[3], float v2[3], float result[3])
{
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static void __gluMultMatrixVecd(const GLdouble matrix[16], const GLdouble in[4],
		      GLdouble out[4])
{
    int i;

    for (i=0; i<4; i++) {
	out[i] = 
	    in[0] * matrix[0*4+i] +
	    in[1] * matrix[1*4+i] +
	    in[2] * matrix[2*4+i] +
	    in[3] * matrix[3*4+i];
    }
}

GLint gluProject(GLdouble objx, GLdouble objy, GLdouble objz, 
	      const GLdouble modelMatrix[16], 
	      const GLdouble projMatrix[16],
              const GLint viewport[4],
	      GLdouble *winx, GLdouble *winy, GLdouble *winz)
{
    double in[4];
    double out[4];

    in[0]=objx;
    in[1]=objy;
    in[2]=objz;
    in[3]=1.0;
    __gluMultMatrixVecd(modelMatrix, in, out);
    __gluMultMatrixVecd(projMatrix, out, in);
    if (in[3] == 0.0) return(GL_FALSE);
    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];
    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;

    /* Map x,y to viewport */
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];

    *winx=in[0];
    *winy=in[1];
    *winz=in[2];
    return(GL_TRUE);
}

void gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx,
	  GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy,
	  GLdouble upz)
{
    float forward[3], side[3], up[3];
    GLfloat m[4][4];

    forward[0] = centerx - eyex;
    forward[1] = centery - eyey;
    forward[2] = centerz - eyez;

    up[0] = upx;
    up[1] = upy;
    up[2] = upz;

    normalize(forward);

    /* Side = forward x up */
    cross(forward, up, side);
    normalize(side);

    /* Recompute up as: up = side x forward */
    cross(side, forward, up);

    __gluMakeIdentityf(&m[0][0]);
    m[0][0] = side[0];
    m[1][0] = side[1];
    m[2][0] = side[2];

    m[0][1] = up[0];
    m[1][1] = up[1];
    m[2][1] = up[2];

    m[0][2] = -forward[0];
    m[1][2] = -forward[1];
    m[2][2] = -forward[2];

    glMultMatrixf(&m[0][0]);
    glTranslated(-eyex, -eyey, -eyez);
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GLdouble m[4][4];
    double sine, cotangent, deltaZ;
    double radians = fovy / 2 * __glPi / 180;

    deltaZ = zFar - zNear;
    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
	return;
    }
    cotangent = cos(radians) / sine;

    __gluMakeIdentityd(&m[0][0]);
    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;
    glMultMatrixd(&m[0][0]);
}

/*

wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/Renderer.cc.o: undefined symbol: glColor4fv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glNormal3f
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glTranslatef
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glScaled
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glLightfv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glLightfv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glLightfv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glLightfv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glColorMaterial
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glMateriali
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glDeleteLists
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glPointSize
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/GLView.cc.o: undefined symbol: glScaled
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glCallList

*/

extern void rglColor4fv(const GLfloat *v);
void glColor4fv(const GLfloat *v) {
    return rglColor4fv(v);
}

extern void rglNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    return rglNormal3f(nx, ny, nz);
}

extern void rglTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    return rglTranslatef(x, y, z);
}

extern void rglScaled(GLdouble x, GLdouble y, GLdouble z);
void glScaled(GLdouble x, GLdouble y, GLdouble z) {
    return rglScaled(x, y, z);
}

extern void rglLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
    return rglLightfv(light, pname, params);
}

extern void rglMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    return rglMaterialfv(face, pname, params);
}

extern void rglDeleteLists(GLuint list, GLsizei range);
void glDeleteLists(GLuint list, GLsizei range) {
    return rglDeleteLists(list, range);
}

extern void rglPointSize(GLfloat size);
void glPointSize(GLfloat size) {
    return rglPointSize(size);
}

extern void rglCallList(GLuint list);
void glCallList(GLuint list) {
    return rglCallList(list);
}

extern void rglColorMaterial(GLenum face, GLenum mode);
void glColorMaterial(GLenum face, GLenum mode) {
    return rglColorMaterial(face, mode);
}

extern void rglMateriali(GLenum face, GLenum pname, GLint param);
void glMateriali(GLenum face, GLenum pname, GLint param) {
    return rglMateriali(face, pname, param);
}

/*
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glColor3ub
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glEndList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glGenLists
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glNewList
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glNormal3dv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluDeleteTess
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluNewTess
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessBeginContour
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessBeginPolygon
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessCallback
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessEndContour
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessEndPolygon
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessNormal
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessProperty
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: gluTessVertex
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glVertex3dv
wasm-ld: error: CMakeFiles/OpenSCAD.dir/src/glview/cgal/CGALRenderer.cc.o: undefined symbol: glVertex3f
*/

extern void rglColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
    return rglColor3ub(red, green, blue);
}

extern void rglEndList();
void glEndList() {
    return rglEndList();
}

extern GLuint rglGenLists(GLsizei range);
GLuint glGenLists(GLsizei range) {
    return rglGenLists(range);
}

extern void rglNewList(GLuint list, GLenum mode);
void glNewList(GLuint list, GLenum mode) {
    return rglNewList(list, mode);
}

extern void rglNormal3dv(const GLdouble *v);
void glNormal3dv(const GLdouble *v) {
    return rglNormal3dv(v);
}

extern void rglDeleteTess(GLUtesselator *tess);
void gluDeleteTess(GLUtesselator *tess) {
    return rglDeleteTess(tess);
}

extern GLUtesselator *rgluNewTess();
GLUtesselator *gluNewTess() {
    return rgluNewTess();
}

extern void rgluTessBeginContour(GLUtesselator *tess);
void gluTessBeginContour(GLUtesselator *tess) {
    return rgluTessBeginContour(tess);
}

extern void rgluTessBeginPolygon(GLUtesselator *tess, GLvoid *polygon_data);
void gluTessBeginPolygon(GLUtesselator *tess, GLvoid *polygon_data) {
    return rgluTessBeginPolygon(tess, polygon_data);
}

extern void rgluTessCallback(GLUtesselator *tess, GLenum which, void (*fn)());
void gluTessCallback(GLUtesselator *tess, GLenum which, void (*fn)()) {
    return rgluTessCallback(tess, which, fn);
}

extern void rgluTessEndContour(GLUtesselator *tess);
void gluTessEndContour(GLUtesselator *tess) {
    return rgluTessEndContour(tess);
}

extern void rgluTessEndPolygon(GLUtesselator *tess);
void gluTessEndPolygon(GLUtesselator *tess) {
    return rgluTessEndPolygon(tess);
}

extern void rgluTessNormal(GLUtesselator *tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
void gluTessNormal(GLUtesselator *tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ) {
    return rgluTessNormal(tess, valueX, valueY, valueZ);
}

extern void rgluTessProperty(GLUtesselator *tess, GLenum which, GLdouble data);
void gluTessProperty(GLUtesselator *tess, GLenum which, GLdouble data) {
    return rgluTessProperty(tess, which, data);
}

extern void rgluTessVertex(GLUtesselator *tess, GLdouble *coords, GLvoid *data);
void gluTessVertex(GLUtesselator *tess, GLdouble *coords, GLvoid *data) {
    return rgluTessVertex(tess, coords, data);
}

extern void rglVertex3dv(const GLdouble *v);
void glVertex3dv(const GLdouble *v) {
    return rglVertex3dv(v);
}

extern void rglVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    return rglVertex3f(x, y, z);
}


} // extern "C"