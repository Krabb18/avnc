#include "RenderObject.h"

void RenderObject::Init()
{
    const char *vertexSource[] = {vertex};
    const char* fragmentSource[] = {fragment};

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, vertexSource, NULL);
    glCompileShader(vertexShader);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, fragmentSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgramm = glCreateProgram();
    glAttachShader(shaderProgramm, vertexShader);
    glAttachShader(shaderProgramm, fragmentShader);
    glLinkProgram(shaderProgramm);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

    // Position (3 floats)
    GLint posLoc = glGetAttribLocation(shaderProgramm, "inPosition");
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(posLoc);

    // Color (3 floats)
    GLint posLoc2 = glGetAttribLocation(shaderProgramm, "inColor");
    glVertexAttribPointer(posLoc2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(posLoc2);

    // TexCoord (2 floats)
    GLint posLoc3 = glGetAttribLocation(shaderProgramm, "inTexCoord");
    glVertexAttribPointer(posLoc3, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(posLoc3);
    glBindVertexArray(0); // Optional
}

void RenderObject::Render()
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    glUseProgram(shaderProgramm);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgramm, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgramm, "myTexture"), 0);

    __android_log_print(ANDROID_LOG_INFO, "MyTag", "Texture ID: %u", textureID);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void RenderObject::Delete()
{
    glDeleteProgram(shaderProgramm);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}