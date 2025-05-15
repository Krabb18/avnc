//
// Created by Leon on 11.05.2025.
//

#ifndef AVNC_RENDEROBJECT_H
#define AVNC_RENDEROBJECT_H

#include <GLES3/gl3.h> // OpenGL ES 3.0 Header

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include <android/log.h>

class RenderObject
{
private:
    const char* vertex =
            "#version 300 es\n"
            "layout(location = 0) in vec3 inPosition;\n"
            "layout(location = 1) in vec3 inColor;\n"
            "layout(location = 2) in vec2 inTexCoord;\n"
            "out vec2 fragUV;\n"
            "out vec3 fragColor;\n"
            "uniform mat4 model;\n"
            "void main() {\n"
            "    fragUV = inTexCoord;\n"
            "    fragColor = inColor;\n"
            "    gl_Position = model * vec4(inPosition, 1.0);\n"
            "}\n";

    const char* fragment =
            "#version 300 es\n"
            "precision mediump float;\n"
            "in vec2 fragUV;\n"
            "in vec3 fragColor;\n"
            "uniform sampler2D uTexture;\n"
            "out vec4 outColor;\n"
            "void main() {\n"
            "    outColor = texture(uTexture, fragUV);\n"
            "}\n";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint shaderProgramm;
    GLuint VAO, VBO, EBO;

    GLfloat vertices[32] =
            { //     COORDINATES     /        COLORS      /   TexCoord  //
                    -0.5f, -0.5f, 0.0f,     1.0f, 0.0f, 0.0f,	0.0f, 0.0f, // Lower left corner
                    -0.5f,  0.5f, 0.0f,     0.0f, 1.0f, 0.0f,	0.0f, 1.0f, // Upper left corner
                    0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,	1.0f, 1.0f, // Upper right corner
                    0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,	1.0f, 0.0f  // Lower right corner
            };

    GLuint indices[6] = {
            0, 1, 2,
            0, 2, 3
    };

public:
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    GLuint textureID;

    void Init();
    void Render();
    void Delete();
};

#endif //AVNC_RENDEROBJECT_H
