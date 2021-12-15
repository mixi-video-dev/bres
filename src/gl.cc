#include "bres+client_ext.h"
// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace bres {
    typedef int EGLDisplay;
    typedef int EGLSurface;

    // struct
    typedef struct glInstance {
        EGLDisplay eglDpy;
        EGLSurface eglSurf;
        GLuint vertexArrayObject;
        GLuint vertexBufferObject;
        GLuint shaderObject;
        GLFWwindow* window;
    } glInstance_t, *glInstance_ptr;
};

#ifndef __BLACKMAGIC_OUTPUT__
#define __SHPG_FLIP_Y__ "  vec4 color = texture(image, texcoord);\n"
#else
#define __SHPG_FLIP_Y__ "  vec4 color = texture(image, vec2(texcoord.x, 1.0-texcoord.y));\n"
#endif


#define BRES_CLI_FS      "" \
"#version 150\n" \
"#extension GL_ARB_explicit_attrib_location : enable\n" \
"\n" \
"uniform sampler2D image;\n" \
"uniform float alpha;\n" \
"uniform float toblack;\n" \
"uniform float threashold;\n" \
"in vec2 texcoord;\n" \
"out vec4 fc;\n" \
"const vec3 chromaKeyColor = vec3(0.0, 1.0, 0.0);\n" \
"\n" \
"void main(void)\n" \
"{\n" \
__SHPG_FLIP_Y__ \
"  float diff = length(chromaKeyColor - color.xyz);\n" \
"  if(diff < threashold){\n" \
"     discard;\n" \
"  }else{\n" \
"    vec4 tmpcolor = vec4(color.xyz * toblack, color.w);\n" \
"    fc = tmpcolor * alpha;\n" \
"  }\n" \
"}"


#define BRES_CLI_VS      "" \
"#version 150\n" \
"\n" \
"in vec4 vertex;\n" \
"uniform mat4 projection;\n" \
"out vec2 texcoord;\n" \
"//\n" \
"void main()\n" \
"{\n" \
"  texcoord = vertex.zw;\n" \
"  gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n" \
"}"



static bres::glInstance_t glInst;

GLFWwindow*
bres::gl_init(void)
{
    // init GLFW
    glfwInit();
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glInst.window = glfwCreateWindow(WND_WIDTH_FHD, WND_HEIGHT_FHD, "bres", NULL, NULL);
    glfwMakeContextCurrent(glInst.window);
    glfwSwapInterval(1);
    
    LOG("GL Version: %s", glGetString(GL_VERSION));
    LOG("GLSL Version:  %s",  glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOG("Vendor:  %s", glGetString(GL_VENDOR));
    LOG("Renderer:  %s",  glGetString(GL_RENDERER));

    // Init GLEW
    glewExperimental = GL_TRUE;
    glewInit();
    glViewport(0, 0, WND_WIDTH_FHD, WND_HEIGHT_FHD);
    // Set OpenGL options
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glAlphaFunc(GL_GREATER, 0.01);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &glInst.vertexArrayObject);
    glBindVertexArray(glInst.vertexArrayObject);
    const unsigned tlocation = 0;
    //
    glInst.shaderObject = gl_generateShader(BRES_CLI_VS, BRES_CLI_FS);
    glUseProgram(glInst.shaderObject);
    glm::mat4 projection = glm::ortho(
        0.0f,
        GLfloat(WND_WIDTH_FHD),
        0.0f,
        GLfloat(WND_HEIGHT_FHD)
    );
    auto uniform = glGetUniformLocation(
        glInst.shaderObject,
        "projection"
    );
    glUniformMatrix4fv(
        uniform, 
        1, 
        GL_FALSE, 
        glm::value_ptr(projection)
    );
    LOG("uniform : %u : %d",
            glInst.shaderObject,
            uniform);
    uniform = glGetUniformLocation(
        glInst.shaderObject,
        "alpha"
    );
    glUniform1f(
        uniform,
        1.0f
    );
    LOG("uniform(alpha) : %u : %d",
        glInst.shaderObject,
        uniform);
    uniform = glGetUniformLocation(
        glInst.shaderObject,
        "toblack"
    );
    glUniform1f(
        uniform, 
        0.0f
    );
    LOG("uniform(toblack) : %u : %d",
        glInst.shaderObject,
        uniform);
    uniform = glGetUniformLocation(
        glInst.shaderObject,
        "threashold"
    );
    glUniform1f(
        uniform, 
        0.0f
    );
    LOG("uniform(threashold) : %u : %d",
        glInst.shaderObject,
        uniform);
    //
    glGenBuffers(1, &glInst.vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, glInst.vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4 * 1, NULL, GL_DYNAMIC_DRAW);
    int attr = glGetAttribLocation(glInst.shaderObject, "vertex");
    LOG("loc : %d/ vbo: %d", attr, glInst.vertexBufferObject);
    glEnableVertexAttribArray(attr);
    glVertexAttribPointer(attr, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    // clear selected.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    //
    return(glInst.window);
}
//
void
bres::gl_generate_texture(
    instance_ptr inst
)
{
    // texture movie
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &inst->movieTexture);
    glBindTexture(GL_TEXTURE_2D, inst->movieTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        inst->width,
        inst->height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        inst->init_image
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//
int
bres::gl_generateShader(
    const char* vShaderCode,
    const char* fShaderCode
)
{
    // compile shaders
    unsigned int vertex = -1, fragment = -1;
    //    vertex shader
    if (vShaderCode) {
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
    }
    // fragment Shader
    if (fShaderCode) {
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
    }
    // shader Program
    auto id = glCreateProgram();
    if (vertex != -1) {
        glAttachShader(id, vertex);
    }
    if (fragment != -1) {
        glAttachShader(id, fragment);
    }
    glLinkProgram(id);
    // delete the shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    //
    return(id);
}
//
void
bres::gl_movie_play(
    instance_ptr inst,
    char* frame,
    float alpha,
    float toblack,
    float x,
    float y,
    float w,
    float h,
    float scalex,
    float scaley
)
{
    glUseProgram(glInst.shaderObject);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(glInst.vertexArrayObject);
    //
    glUniform1f(
        glGetUniformLocation(
            glInst.shaderObject,
            "alpha"
        ),
        alpha
    );
    glUniform1f(
        glGetUniformLocation(
            glInst.shaderObject,
            "toblack"
        ),
        toblack
    );
    GLfloat threashold = inst->chromakey;
    glUniform1f(
        glGetUniformLocation(
            glInst.shaderObject,
            "threashold"
        ),
        threashold
    );
    glUniform3f(
        glGetUniformLocation(
            glInst.shaderObject,
            "chromakeyColor"),
        0.f,
        1.f,
        0.f);

    GLfloat vertices[6][4];
    calculateVec(x, y, w * scalex, h * scaley, vertices);
    glBindTexture(GL_TEXTURE_2D, inst->movieTexture);
    glBindBuffer(GL_ARRAY_BUFFER, glInst.vertexBufferObject);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        w,
        h,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        frame
    );
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}
//
void bres::calculateVec(
    GLfloat x,                          // [in] X position
    GLfloat y,                          // [in] Y position
    GLfloat w,                          // [in] width
    GLfloat h,                          // [in] height
    GLfloat box[6][4]                   // [in/out] calculated vector
)
{
    GLfloat ret[6][4] = 
        {
            { x,     WND_HEIGHT - (y),       0.0, 0.0 },
            { x,     WND_HEIGHT - (y + h),   0.0, 1.0 },
            { x + w, WND_HEIGHT - (y + h),   1.0, 1.0 },

            { x + w, WND_HEIGHT - (y),       1.0, 0.0 },
            { x,     WND_HEIGHT - (y),       0.0, 0.0 },
            { x + w, WND_HEIGHT - (y + h),   1.0, 1.0 },
        };
    memcpy(box, ret, sizeof(GLfloat) * 6 * 4);
}
//
void
bres::gl_quit(
    instance_ptr inst
)
{
    glDeleteTextures(1, &inst->movieTexture);
}