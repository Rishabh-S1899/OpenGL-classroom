// room_combined.cpp
// Combined: new room dimensions + textured benches, but old lighting & object handling
// Compile example (Linux): g++ room_combined.cpp glad.c -Iinclude -lglfw -ldl -lGL -pthread -o room.exe

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLFWwindow* window = nullptr;

// Use new room resolution or old? keep big window; you can change it
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera (mix of old and new reasonable defaults)
glm::vec3 cameraPos   = glm::vec3(0.0f, 3.0f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = -15.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct Mesh {
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    size_t indexCount = 0;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    bool hasTexture = false;
    unsigned int textureID = 0;
    std::string logicalName; // "bench", "podium", "greenboard", etc
    std::string shapeName;   // shape name inside OBJ
};

std::vector<Mesh> sceneMeshes;
unsigned int roomVAO = 0, roomVBO = 0, roomEBO = 0;
unsigned int projectorVAO = 0, projectorVBO = 0, projectorEBO = 0;
unsigned int lightBoxVAO = 0, lightBoxVBO = 0, lightBoxEBO = 0;

// Old-style single light used by old shader
glm::vec3 lightPos(0.0f, 2.5f, 0.0f);

// prototypes
void framebuffer_size_callback(GLFWwindow*, int width, int height);
void mouse_callback(GLFWwindow*, double xpos, double ypos);
void scroll_callback(GLFWwindow*, double, double yoffset);
void processInput(GLFWwindow *window);
unsigned int createShaderProgram();
void setupGeometry();
void drawScene(unsigned int shaderProgram, unsigned int ceilingTexture, unsigned int floorTexture);
std::vector<Mesh> loadOBJModels(const std::string& path, const std::string& logicalName, const std::string& texPath = "");
Mesh loadOBJShape(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
                  const std::string& logicalName, const std::string& texPath = "");
unsigned int loadTexture(const char* path);

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Room Combined", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShaderProgram();

    // Using old script textures names/paths for floor/ceiling (adapt paths as needed)
    unsigned int ceilingTexture = loadTexture("assets/ceiling_tile.png");
    if (ceilingTexture == 0) std::cerr << "Warning: ceiling texture load failed\n";
    unsigned int floorTexture = loadTexture("assets/floor_tile.png");
    if (floorTexture == 0) std::cerr << "Warning: floor texture load failed\n";

    setupGeometry();

    std::cout << "Loading models..." << std::endl;

    // Podium (use old 'podium_new.obj' and color from old script)
    auto podiums = loadOBJModels("assets/podium_new.obj", "podium", "");
    for (auto& m : podiums) {
        if (!m.hasTexture) m.color = glm::vec3(0.82f, 0.71f, 0.55f);
        sceneMeshes.push_back(m);
    }

    // Greenboard (old greenboard_new.obj)
    auto gbs = loadOBJModels("assets/greenboard_new.obj", "greenboard", "");
    for (auto& m : gbs) {
        std::string low = m.shapeName;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        m.color = (low.find("green") != std::string::npos) ? glm::vec3(0.0f, 0.4f, 0.0f) : glm::vec3(0.78f,0.78f,0.78f);
        sceneMeshes.push_back(m);
    }

    // Benches - use bench.obj and apply bench texture (as requested)
    auto benches = loadOBJModels("assets/bench.obj", "bench", "assets/bench_texture.jpg");
    // set bench color fallback if texture missing
    for (auto& m : benches) {
        if (!m.hasTexture) m.color = glm::vec3(0.6f, 0.4f, 0.25f);
        sceneMeshes.push_back(m);
    }

    std::cout << "Loaded meshes: " << sceneMeshes.size() << std::endl;

    // main loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Old script uniforms (single light)
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &cameraPos[0]);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        drawScene(shaderProgram, ceilingTexture, floorTexture);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &roomVAO);
    glDeleteBuffers(1, &roomVBO);
    glDeleteBuffers(1, &roomEBO);
    glDeleteVertexArrays(1, &projectorVAO);
    glDeleteBuffers(1, &projectorVBO);
    glDeleteBuffers(1, &projectorEBO);
    glDeleteVertexArrays(1, &lightBoxVAO);
    glDeleteBuffers(1, &lightBoxVBO);
    glDeleteBuffers(1, &lightBoxEBO);

    for (auto &m : sceneMeshes) {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.textureID) glDeleteTextures(1, &m.textureID);
    }

    glDeleteProgram(createShaderProgram()); // optional double-check
    glfwTerminate();
    return 0;
}

/* -------------------- input / callbacks -------------------- */
void processInput(GLFWwindow *window) {
    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos.y += cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cameraPos.y -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos; lastY = (float)ypos; firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity; yoffset *= sensitivity;
    yaw += xoffset; pitch += yoffset;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
void scroll_callback(GLFWwindow*, double, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}

/* -------------------- shader (old lighting style) -------------------- */
unsigned int createShaderProgram() {
    const char* vShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
        }
    )";

    const char* fShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;

        uniform vec3 objectColor;
        uniform vec3 lightColor;
        uniform vec3 lightPos;
        uniform vec3 viewPos;

        uniform sampler2D textureSampler;
        uniform bool hasTexture;

        void main() {
            vec4 surfaceColor;
            if (hasTexture) surfaceColor = texture(textureSampler, TexCoord);
            else surfaceColor = vec4(objectColor, 1.0);

            float ambientStrength = 0.2;
            vec3 ambient = ambientStrength * lightColor;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * surfaceColor.rgb;
            FragColor = vec4(result, 1.0);
        }
    )";

    auto compile = [](const char* src, GLenum type) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(s, 1024, NULL, log);
            std::cerr << "Shader compile error: " << log << std::endl;
        }
        return s;
    };

    GLuint vs = compile(vShaderSrc, GL_VERTEX_SHADER);
    GLuint fs = compile(fShaderSrc, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(prog, 1024, NULL, log);
        std::cerr << "Program link error: " << log << std::endl;
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

/* -------------------- geometry (room = new dimensionality) -------------------- */
void setupGeometry() {
    // New room dims: w=10, h=5, d=10 (matches new script)
    float w = 10.0f, h = 5.0f, d = 8.0f;
    const float roomVerts[] = {
        // floor (y=0) normal up
        -w, 0.0f, -d,  0,1,0,  0.0f, 0.0f,
         w, 0.0f, -d,  0,1,0,  1.0f, 0.0f,
         w, 0.0f,  d,  0,1,0,  1.0f, 1.0f,
        -w, 0.0f,  d,  0,1,0,  0.0f, 1.0f,

        // ceiling (y=h) normal down
        -w, h, -d,  0,-1,0,  0.0f, 0.0f,
         w, h, -d,  0,-1,0,  1.0f, 0.0f,
         w, h,  d,  0,-1,0,  1.0f, 1.0f,
        -w, h,  d,  0,-1,0,  0.0f, 1.0f,

        // back (z = d)
        -w, 0.0f, d,  0,0,-1,  0.0f, 0.0f,
         w, 0.0f, d,  0,0,-1,  1.0f, 0.0f,
         w,  h,  d,  0,0,-1,  1.0f, 1.0f,
        -w,  h,  d,  0,0,-1,  0.0f, 1.0f,

        // front (z = -d)
        -w, 0.0f, -d,  0,0,1, 0.0f, 0.0f,
         w, 0.0f, -d,  0,0,1, 1.0f, 0.0f,
         w,  h, -d,  0,0,1, 1.0f, 1.0f,
        -w,  h, -d,  0,0,1, 0.0f, 1.0f,

        // left (x=-w)
        -w, 0.0f, -d,  1,0,0,  0.0f, 0.0f,
        -w, 0.0f,  d,  1,0,0,  1.0f, 0.0f,
        -w,  h,  d,  1,0,0,  1.0f, 1.0f,
        -w,  h, -d,  1,0,0,  0.0f, 1.0f,

        // right (x=w)
         w, 0.0f, -d,  -1,0,0,  0.0f, 0.0f,
         w, 0.0f,  d,  -1,0,0,  1.0f, 0.0f,
         w,  h,  d,  -1,0,0,  1.0f, 1.0f,
         w,  h, -d,  -1,0,0,  0.0f, 1.0f
    };

    const unsigned int roomInds[] = {
        0,1,2, 2,3,0,    // floor
        4,5,6, 6,7,4,    // ceiling
        8,9,10, 10,11,8, // back
        12,13,14, 14,15,12, // front
        16,17,18, 18,19,16, // left
        20,21,22, 22,23,20  // right
    };

    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glGenBuffers(1, &roomEBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roomVerts), roomVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, roomEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(roomInds), roomInds, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // projector sheet (simple quad)
    float pw = 1.0f, ph = 0.6f;
    float projVerts[] = {
        -pw/2, -ph/2, 0,  0,0,1, 0,0,
         pw/2, -ph/2, 0,  0,0,1, 1,0,
         pw/2,  ph/2, 0,  0,0,1, 1,1,
        -pw/2,  ph/2, 0,  0,0,1, 0,1
    };
    unsigned int projInds[] = { 0,1,2, 2,3,0 };
    glGenVertexArrays(1, &projectorVAO);
    glGenBuffers(1, &projectorVBO);
    glGenBuffers(1, &projectorEBO);
    glBindVertexArray(projectorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, projectorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(projVerts), projVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, projectorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(projInds), projInds, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // light box (small flat rectangular light)
    float lw = 1.5f, lh = 0.1f;
    float boxVerts[] = {
        -lw/2, -lh/2, -lw/2, 0,-1,0,  0,0,
         lw/2, -lh/2, -lw/2, 0,-1,0,  1,0,
         lw/2, -lh/2,  lw/2, 0,-1,0,  1,1,
        -lw/2, -lh/2,  lw/2, 0,-1,0,  0,1,
        -lw/2,  lh/2, -lw/2, 0,1,0,   0,0,
         lw/2,  lh/2, -lw/2, 0,1,0,   1,0,
         lw/2,  lh/2,  lw/2, 0,1,0,   1,1,
        -lw/2,  lh/2,  lw/2, 0,1,0,   0,1
    };
    unsigned int boxInds[] = {
        0,1,2, 2,3,0, 4,5,6, 6,7,4,
        0,1,5, 5,4,0, 2,3,7, 7,6,2,
        0,3,7, 7,4,0, 1,2,6, 6,5,1
    };
    glGenVertexArrays(1, &lightBoxVAO);
    glGenBuffers(1, &lightBoxVBO);
    glGenBuffers(1, &lightBoxEBO);
    glBindVertexArray(lightBoxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightBoxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxInds), boxInds, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

/* -------------------- draw scene -------------------- */
void drawScene(unsigned int shaderProgram, unsigned int ceilingTex, unsigned int floorTex) {
    auto setTexture = [&](bool enabled, unsigned int tex, glm::vec3 col = glm::vec3(1.0f)) {
        glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), enabled ? GL_TRUE : GL_FALSE);
        if (enabled && tex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
        } else {
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, &col[0]);
        }
    };

    // Draw room (floor, ceiling, walls). Use new dims layout
    glBindVertexArray(roomVAO);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // floor
    setTexture(true, floorTex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0));
    // ceiling
    setTexture(true, ceilingTex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(unsigned int)));
    // back+front (walls) - no texture, subtle colors
    setTexture(false, 0, glm::vec3(0.95f));
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void*)(12 * sizeof(unsigned int)));
    setTexture(false, 0, glm::vec3(0.90f));
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void*)(24 * sizeof(unsigned int)));
    glBindVertexArray(0);

    // Draw light boxes (use one light element at the ceiling in the center)
    // glBindVertexArray(lightBoxVAO);
    // setTexture(false, 0, glm::vec3(0.95f, 0.95f, 0.85f));
    // model = glm::translate(glm::mat4(1.0f), lightPos);
    // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    // glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    // glBindVertexArray(0);

    // Draw podium, greenboard, benches (sceneMeshes)
    for (auto &mesh : sceneMeshes) {
        if (!mesh.VAO || mesh.indexCount == 0) continue;
        glBindVertexArray(mesh.VAO);

        if (mesh.logicalName == "bench") {

            // setTexture(mesh.hasTexture, mesh.textureID, mesh.color);

            // // Big room: Z goes from -10 (front) to +10 (back)
            // // Put last row near z = +9, move forward by row spacing
            // float backZ = 9.0f;
            // float rowSpacing = 3.0f;       // space between bench rows
            // float colSpacing = 3.5f;       // space between columns
            // float startX = -7.0f;          // left-most bench
            // float startY = 0.5f;           // height

            // for (int r = 0; r < 5; ++r) {
            //     for (int c = 0; c < 5; ++c) {

            //         float x = startX + c * colSpacing;
            //         float y = startY;
            //         float z = backZ - r * rowSpacing;

            //         glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
            //         model = glm::rotate(glm::scale(model, glm::vec3(0.35f)), glm::radians(180.0f), glm::vec3(0, 1, 0));

            //         glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            //         glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
            //     }
            // }
            setTexture(mesh.hasTexture, mesh.textureID, mesh.color);

            // Layout configuration
            const int cols = 4;
            const float d_room = 8.0f;                   // 4 columns total
            const float backZ = d_room-1.0f;             // back-most z
            const float rowSpacing = 2.0f;        // distance between centers of consecutive rows
            const float benchScale = 0.35f;
            const float y = 0.68f;

            // Per-column row counts (index 0 = leftmost column)
            // You requested: first 2 columns -> 5 rows, others -> 6 rows
            int rowCount[cols] = { 5, 5, 6, 6 };

            // Column layout (middle 2 joined, symmetric outer gaps)
            const float benchCenterSep = 3.5f;    // center-to-center for adjacent benches in middle pair
            const float outerGap = 1.0f;          // surface-to-surface gap between outer and middle pair

            float mid_left  = -benchCenterSep * 0.5f;
            float mid_right =  benchCenterSep * 0.5f;
            float centerDistOuterToMiddle = benchCenterSep + outerGap;

            float colX[4];
            colX[0] = mid_left - centerDistOuterToMiddle; // left outer
            colX[1] = mid_left;                            // middle-left (joined)
            colX[2] = mid_right;                           // middle-right (joined)
            colX[3] = mid_right + centerDistOuterToMiddle; // right outer

            // Draw benches per column using per-column rowCount
            for (int c = 0; c < cols; ++c) {
                int rows_here = rowCount[c];
                for (int r = 0; r < rows_here; ++r) {
                    // z = backZ - r * rowSpacing keeps back-most aligned at backZ for every column
                    // float z = backZ - r * rowSpacing;
                    // glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(colX[c], y, z));
                    // model = glm::rotate(glm::scale(model, glm::vec3(benchScale)), glm::radians(180.0f), glm::vec3(0,1,0));
                    // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    // glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
                    float z = backZ - r * rowSpacing; // back-aligned per column
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(colX[c], y, z));
                    model = glm::rotate(glm::scale(model, glm::vec3(benchScale)), glm::radians(180.0f), glm::vec3(0,1,0));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
                }
            }
        } else if (mesh.logicalName == "greenboard") {
            // use old transform from original code
            // glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, -2.6f));
            // m = glm::scale(m, glm::vec3(0.25f, 0.1f, 0.2f)); // approximate small board scale
            // // But since new room distances are bigger, place board at front wall:
            // m = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 2.5f, -9.8f)), glm::vec3(0.8f,0.8f,1.0f));
            // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
            // setTexture(mesh.hasTexture, mesh.textureID, mesh.color);
            // glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);

            float boardInset = 0.1f;
            // glm::vec3 boardPos = glm::vec3(0.0f, 2.2f, -9.2f);
            glm::vec3 boardPos = glm::vec3(0.0f, 2.5f, -8.0f + boardInset);
            glm::vec3 boardScale = glm::vec3(0.6f, 0.19f, 0.6f); // X=width, Y=height, Z=depth scale
            glm::mat4 m = glm::translate(glm::mat4(1.0f), boardPos);
            m = glm::scale(m, boardScale);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
            setTexture(mesh.hasTexture, mesh.textureID, mesh.color);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
        } else if (mesh.logicalName == "podium") {
            // old podium placement logic (scaled & positioned)
            // float podiumInset = 2.0f;
            // glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.75f, 0.6f, -1.25f)), glm::vec3(0.2f));
            // // but adapt to room far front similar to new script
            // m = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 0.5f, -8.0f)), glm::vec3(0.35f));
            // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
            // setTexture(mesh.hasTexture, mesh.textureID, mesh.color);
            // glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);


            float podiumInset = 2.5f;
            glm::vec3 podiumPos = glm::vec3(-5.0f, 1.15f, -(8.0f - podiumInset)); // z = -6.0
            float podiumScale = 0.35f;

            glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), podiumPos), glm::vec3(podiumScale));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
            setTexture(mesh.hasTexture, mesh.textureID, mesh.color);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
        } else {
            // fallback: draw at origin
            glm::mat4 m = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
            setTexture(mesh.hasTexture, mesh.textureID, mesh.color);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
    }

    // projector sheet (small pale quad)
    glBindVertexArray(projectorVAO);
    glm::mat4 pm = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 2.8f, -9.7f)), glm::vec3(1.8f, 1.2f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(pm));
    setTexture(false, 0, glm::vec3(0.92f, 0.92f, 0.88f));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

/* -------------------- OBJ loader (per-shape) -------------------- */
Mesh loadOBJShape(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
                  const std::string& logicalName, const std::string& texPath) {
    Mesh mesh;
    mesh.logicalName = logicalName;
    mesh.shapeName = shape.name;

    if (shape.mesh.indices.empty()) return mesh;

    // decide to load texture for benches only (or if texPath provided)
    if (!texPath.empty() && logicalName == "bench") {
        mesh.textureID = loadTexture(texPath.c_str());
        mesh.hasTexture = (mesh.textureID != 0);
    }

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(shape.mesh.indices.size() * 8);
    indices.reserve(shape.mesh.indices.size());

    for (const auto &idx : shape.mesh.indices) {
        // position
        if (idx.vertex_index >= 0 && (size_t)(3 * idx.vertex_index + 2) < attrib.vertices.size()) {
            vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
            vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
            vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
        } else {
            vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        }

        // normal
        if (idx.normal_index >= 0 && (size_t)(3 * idx.normal_index + 2) < attrib.normals.size()) {
            vertices.push_back(attrib.normals[3 * idx.normal_index + 0]);
            vertices.push_back(attrib.normals[3 * idx.normal_index + 1]);
            vertices.push_back(attrib.normals[3 * idx.normal_index + 2]);
        } else {
            vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        }

        // texcoord
        if (idx.texcoord_index >= 0 && (size_t)(2 * idx.texcoord_index + 1) < attrib.texcoords.size()) {
            vertices.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
            vertices.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
        } else {
            vertices.push_back(0.0f); vertices.push_back(0.0f);
        }

        indices.push_back((unsigned int)indices.size());
    }

    mesh.indexCount = indices.size();
    if (vertices.empty()) return mesh;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

std::vector<Mesh> loadOBJModels(const std::string& path, const std::string& logicalName, const std::string& texPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), nullptr, true)) {
        std::cerr << "Failed to load OBJ: " << path << " warn: " << warn << " err: " << err << std::endl;
        return {};
    }
    std::vector<Mesh> out;
    out.reserve(shapes.size());
    for (const auto &s : shapes) {
        out.push_back(loadOBJShape(attrib, s, logicalName, texPath));
    }
    return out;
}

/* -------------------- texture loader (stb_image) -------------------- */
unsigned int loadTexture(const char* path) {
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (!data) {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        return 0;
    }
    GLenum format = GL_RGB;
    if (nrComponents == 1) format = GL_RED;
    else if (nrComponents == 3) format = GL_RGB;
    else if (nrComponents == 4) format = GL_RGBA;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}
