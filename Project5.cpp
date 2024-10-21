#include <iostream>
#include <cmath>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// OpenGL
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif


// GLFW
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <utility>
#include <sstream>

#include <SOIL/SOIL.h>

// Other includes
#include "Shader.h"
#include "Camera.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void do_movement();
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
GLFWwindow* windowInit();
void SetupOpenGLState(Shader& ourShader, GLint& objectColorLoc, GLint& modelLoc);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

Camera  camera(glm::vec3(-0.0226796f, 0.883629f, 1.91857f), glm::vec3(0.0f, 1.0f, 0.0f), -90.1667f, -6.66667f);
// Camera
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
GLfloat lastX =  WIDTH  / 2.0;
GLfloat lastY =  HEIGHT / 2.0;
bool    keys[1024];

// Light attributes
glm::vec3 lightPos(-0.5f, 2.5f, 1.3f);

// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

bool mouseMovementEnabled = true;

// Camera positions
std::vector<glm::vec3> cameraPositions = {
    glm::vec3(0.0f, 1.5f, 5.0f),  // Front view
    glm::vec3(5.0f, 1.5f, 0.0f),  // Side view
    glm::vec3(0.0f, 10.0f, 0.0f)  // Top-down view
};
int currentCameraIndex = 0;

// Utility functions to convert units
float inchesToMeters(float inches) {
    return inches * 0.0254f;
}

float feetToMeters(float feet) {
    return feet * 0.3048f;
}

struct Towel {
    GLuint VAO, VBO;
    glm::vec3 position;
    glm::vec3 angle;
    glm::vec3 scale;
    glm::vec4 color;
    std::vector<unsigned int> indices;

    Towel(glm::vec3 position = glm::vec3(0.0f), 
         glm::vec3 scale = glm::vec3(1.0f), 
         glm::vec3 angle = glm::vec3(0.0f, 0.0f, 0.0f), 
         glm::vec4 color = glm::vec4(1.0f), 
         const std::string &objFilePath = nullptr)  // Added texturePath parameter
         : position(position), angle(angle), scale(scale), color(color) {
        loadObjModel(objFilePath);
    }

    void loadObjModel(const std::string &objFilePath) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(objFilePath, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        std::vector<float> vertices;

        // Process each mesh
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[i];

            // Process vertices
            for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                aiVector3D position = mesh->mVertices[j];
                vertices.push_back(position.x);
                vertices.push_back(position.y);
                vertices.push_back(position.z);

                // Process texture coordinates (if available)
                if (mesh->mTextureCoords[0]) {
                    aiVector3D texCoord = mesh->mTextureCoords[0][j];
                    vertices.push_back(texCoord.x);
                    vertices.push_back(texCoord.y);
                } else {
                    vertices.push_back(0.0f); // Default texture coordinates
                    vertices.push_back(0.0f);
                }
            }

            // Process indices
            for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }
        }

        // Create and bind VAO/VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Vertex positions
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Texture coordinates
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Create element buffer object (EBO)
        GLuint EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0); // Unbind VAO
    }

    // Load texture
    GLuint loadTexture(const char* path) const {
        // Load and create a texture 
        GLuint texture1;
        // ====================
        // Texture 1
        // ====================
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
        // Set our texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Load, create texture and generate mipmaps
        int width, height;
        unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
        if (image) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            glGenerateMipmap(GL_TEXTURE_2D);
            SOIL_free_image_data(image);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
        return texture1;
    }

    void draw(Shader &shader, const GLint &objectColorLoc, const GLint &modelLoc) const {
        // Set the brighter to false
        glUniform1f(glGetUniformLocation(shader.Program, "brighter"), true);
        glActiveTexture(GL_TEXTURE0);
        GLuint texture = loadTexture("./container_original copy.jpg"); // This works
        glBindTexture(GL_TEXTURE_2D, texture); // No texture
        glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 0); // disable texture usage
        // Create a default model to adjust
        glm::mat4 model = glm::mat4(1.0f);
        // Set the position for this Trapezoid
        model = glm::translate(model, position);
        // Apply rotations in a specific order
        model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around x-axis
        model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around y-axis
        model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around z-axis
        // Set the scale for this Trapezoid
        model = glm::scale(model, scale);  
        // Set the model for this Trapezoid
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        // Set the color for this Trapezoid
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
     
        glDrawElements(GL_TRIANGLES, indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    }
};


// Cube structure
struct Cube {
    // Cube information
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale; 
    glm::vec3 angle;
    glm::vec4 color;
    GLuint VBO, VAO;
    GLuint textureID;  // Texture ID for the drawer
    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat cubeVertices[288] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f
    };

    // Cube constructor
    Cube(glm::vec3 position = glm::vec3(0.0f), 
         glm::vec3 rotation = glm::vec3(1.0f, 0.3f, 0.5f), 
         glm::vec3 scale = glm::vec3(1.0f), 
         glm::vec3 angle = glm::vec3(0.0f, 0.0f, 0.0f), 
         glm::vec4 color = glm::vec4(1.0f), 
         const char* texturePath = nullptr)  // Added texturePath parameter
         : position(position), rotation(rotation), angle(angle), scale(scale), color(color) {
        
        std::pair<GLuint, GLuint> buffers = createVAOAndVBO();
        VAO = buffers.first;
        VBO = buffers.second;

        if (texturePath != nullptr) {
            std::cout << "Loading texture: " << texturePath << std::endl;
            textureID = loadTexture(texturePath);  // Load texture if path is provided
        } else {
            textureID = -1;
        }
    }

    // Load texture
    GLuint loadTexture(const char* path) const {
        // Load and create a texture 
        GLuint texture1;
        // ====================
        // Texture 1
        // ====================
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
        // Set our texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Load, create texture and generate mipmaps
        int width, height;
        unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
        if (image) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            glGenerateMipmap(GL_TEXTURE_2D);
            SOIL_free_image_data(image);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
        return texture1;
    }

    // Create and return VAO and VBO based on cubeVertices
    std::pair<GLuint, GLuint> createVAOAndVBO() {
        GLuint VAO, VBO;

        // Generate and bind VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind and upload vertex data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        // Bind the VAO and configure the vertex attributes
        glBindVertexArray(VAO);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // TexCoord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        // Unbind the VAO
        glBindVertexArray(0);

        // Return both VAO and VBO as a pair
        return std::make_pair(VAO, VBO);
    }

    // Draw the cube
    void draw(Shader &shader, const GLint &objectColorLoc, const GLint &modelLoc) const {
        // Set the brighter to false
        glUniform1f(glGetUniformLocation(shader.Program, "brighter"), false);
        // Bind the texture for the drawer
        if (textureID != -1) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
            glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 1); // Enable texture usage
        } else {
            glActiveTexture(GL_TEXTURE0);
            GLuint texture = loadTexture("./container_original copy.jpg"); // This works
            glBindTexture(GL_TEXTURE_2D, texture); // No texture
            glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
            glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 0); // disable texture usage
        }

        // Create a default model to adjust
        glm::mat4 model = glm::mat4(1.0f);
        // Set the position for this cube
        model = glm::translate(model, position);
        // Apply rotations in a specific order
        model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);

        // Pass the model matrix to the shader
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        // Set the object color
        glUniform4fv(objectColorLoc, 1, glm::value_ptr(color));
        glUniform3f(glGetUniformLocation(shader.Program, "objectColor"), color.x, color.y, color.z); // White

        // Draw the cube
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        
        if (textureID != -1) {
            // Unbind the texture
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
};

// wiiGame structure
struct wiiGame {
    // Cube information
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale; 
    glm::vec3 angle;
    glm::vec4 color;
    GLuint VBO, VAO;
    GLuint textureID;  // Texture ID for the drawer
    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat wiiGameVertices[288] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f
    };

    // wiiGame constructor
    wiiGame(glm::vec3 position = glm::vec3(0.0f), 
         glm::vec3 rotation = glm::vec3(1.0f, 0.3f, 0.5f), 
         glm::vec3 scale = glm::vec3(1.0f), 
         glm::vec3 angle = glm::vec3(0.0f, 0.0f, 0.0f), 
         glm::vec4 color = glm::vec4(1.0f), 
         const char* texturePath = nullptr)  // Added texturePath parameter
         : position(position), rotation(rotation), angle(angle), scale(scale), color(color) {
        
        std::pair<GLuint, GLuint> buffers = createVAOAndVBO();
        VAO = buffers.first;
        VBO = buffers.second;

        if (texturePath != nullptr) {
            std::cout << "Loading texture: " << texturePath << std::endl;
            textureID = loadTexture(texturePath);  // Load texture if path is provided
        } else {
            textureID = -1;
        }
    }

    // Load texture
    GLuint loadTexture(const char* path) const {
        // Load and create a texture 
        GLuint texture1;
        // ====================
        // Texture 1
        // ====================
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
        // Set our texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Load, create texture and generate mipmaps
        int width, height;
        unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
        if (image) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            glGenerateMipmap(GL_TEXTURE_2D);
            SOIL_free_image_data(image);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
        return texture1;
    }

    // Create and return VAO and VBO based on cubeVertices
    std::pair<GLuint, GLuint> createVAOAndVBO() {
        GLuint VAO, VBO;

        // Generate and bind VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind and upload vertex data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(wiiGameVertices), wiiGameVertices, GL_STATIC_DRAW);

        // Bind the VAO and configure the vertex attributes
        glBindVertexArray(VAO);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // TexCoord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        // Unbind the VAO
        glBindVertexArray(0);

        // Return both VAO and VBO as a pair
        return std::make_pair(VAO, VBO);
    }

    // Draw the wiiGame
    void draw(Shader &shader, const GLint &objectColorLoc, const GLint &modelLoc) const {
        // Set the brighter to false
        glUniform1f(glGetUniformLocation(shader.Program, "brighter"), true);
        // Bind the texture for the drawer
        if (textureID != -1) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
            glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 1); // Enable texture usage
        } else {
            glActiveTexture(GL_TEXTURE0);
            GLuint texture = loadTexture("./container_original copy.jpg"); // This works
            glBindTexture(GL_TEXTURE_2D, texture); // No texture
            glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
            glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 0); // disable texture usage
        }

        // Create a default model to adjust
        glm::mat4 model = glm::mat4(1.0f);
        // Set the position for this cube
        model = glm::translate(model, position);
        // Apply rotations in a specific order
        model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);

        // Pass the model matrix to the shader
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        // Set the object color
        glUniform4fv(objectColorLoc, 1, glm::value_ptr(color));
        glUniform3f(glGetUniformLocation(shader.Program, "objectColor"), color.x, color.y, color.z); // White

        // Draw the wiiGame
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        
        if (textureID != -1) {
            // Unbind the texture
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
};

// Pyramid structure
struct Pyramid {
    // Pyramid information
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale; 
    glm::vec3 angle;
    glm::vec4 color;
    GLuint VBO, VAO;
    
    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat pyramidVertices[144] = {
        // Base square (two triangles)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // Bottom left
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // Bottom right
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // Top right
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // Bottom left
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // Top right
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // Top left

        // Side triangles (4 faces)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.447f,  0.894f, 0.0f, 0.0f, // Base bottom left
         0.5f, -0.5f, -0.5f,  0.0f,  0.447f,  0.894f, 1.0f, 0.0f, // Base bottom right
         0.0f,  0.5f,  0.0f,  0.0f,  0.447f,  0.894f, 1.0f, 1.0f, // Apex

         0.5f, -0.5f, -0.5f,  0.894f,  0.447f,  0.0f, 1.0f, 1.0f, // Base bottom right
         0.5f, -0.5f,  0.5f,  0.894f,  0.447f,  0.0f, 0.0f, 1.0f, // Base top right
         0.0f,  0.5f,  0.0f,  0.894f,  0.447f,  0.0f, 0.0f, 0.0f, // Apex

         0.5f, -0.5f,  0.5f,  0.0f,  0.447f, -0.894f, 0.0f, 0.0f, // Base top right
        -0.5f, -0.5f,  0.5f,  0.0f,  0.447f, -0.894f, 1.0f, 0.0f, // Base top left
         0.0f,  0.5f,  0.0f,  0.0f,  0.447f, -0.894f, 1.0f, 1.0f, // Apex

        -0.5f, -0.5f,  0.5f, -0.894f,  0.447f,  0.0f, 1.0f, 1.0f, // Base top left
        -0.5f, -0.5f, -0.5f, -0.894f,  0.447f,  0.0f, 0.0f, 1.0f, // Base bottom left
         0.0f,  0.5f,  0.0f, -0.894f,  0.447f,  0.0f, 0.0f, 0.0f, // Apex
    };

    // Pyramid constructor
    Pyramid(glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.1f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(1.0f), glm::vec3 angle = glm::vec3(0.0f), glm::vec4 color = glm::vec4(1.0f))
         : position(position), rotation(rotation), angle(angle), scale(scale), color(color) {
            std::pair<GLuint, GLuint> buffers = createVAOAndVBO();
            VAO = buffers.first;
            VBO = buffers.second;
        }

    // Create and return VAO and VBO based on pyramidVertices
    std::pair<GLuint, GLuint> createVAOAndVBO() {
        GLuint VAO, VBO;

        // Generate and bind VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind and upload vertex data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);

        // Bind the VAO and configure the vertex attributes
        glBindVertexArray(VAO);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // TexCoord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        // Unbind the VAO
        glBindVertexArray(0);

        // Return both VAO and VBO as a pair
        return std::make_pair(VAO, VBO);
    }

    // Load texture
    GLuint loadTexture(const char* path) const {
        // Load and create a texture 
        GLuint texture1;
        // ====================
        // Texture 1
        // ====================
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
        // Set our texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Load, create texture and generate mipmaps
        int width, height;
        unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
        if (image) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            glGenerateMipmap(GL_TEXTURE_2D);
            SOIL_free_image_data(image);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
        return texture1;
    }

    // Draw the pyramid
    void draw(Shader &shader, const GLint &objectColorLoc, const GLint &modelLoc) const {
        // Set the brighter to false
        glUniform1f(glGetUniformLocation(shader.Program, "brighter"), false);
        glActiveTexture(GL_TEXTURE0);
        GLuint texture = loadTexture("./container_original copy.jpg"); // This works
        glBindTexture(GL_TEXTURE_2D, texture); // No texture
        glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 0); // disable texture usage
        // Create a default model to adjust
        glm::mat4 model = glm::mat4(1.0f);
        // Set the position for this pyramid
        model = glm::translate(model, position);
        // Set the rotation for this pyramid
        model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around x-axis
        model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around y-axis
        model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around z-axis
        // Set the scale for this pyramid
        model = glm::scale(model, scale);  
        // Set the model for this pyramid
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        // Set the color for this pyramid
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
        
        // Draw the pyramid using triangles
        glDrawArrays(GL_TRIANGLES, 0, 18); // 6 triangles for the pyramid (1 base + 4 sides)
    }
};

// Trapezoid structure
struct Trapezoid {
    // Trapezoid information
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale; 
    glm::vec3 angle;
    glm::vec4 color;
    GLuint VBO, VAO;
    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat trapezoidVertices[336] = {
        // Back face
        -2.0f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
        0.25f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        0.25f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -2.0f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        -2.0f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

        // Front face (smaller base)
        -2.0f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
        0.25f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        0.25f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -2.0f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
        -2.0f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        // Left face (vertical)
        -2.0f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -2.0f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -2.0f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -2.0f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -2.0f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -2.0f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        // Right face
        0.25f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        0.25f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        0.25f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        // Bottom face
        -2.0f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
        -2.0f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
        -2.0f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,

        // Top face (narrower)
        -2.0f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         0.25f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         0.25f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.25f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
        -2.0f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        -2.0f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f
    };

    // Trapezoid constructor
    Trapezoid(glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(1.0f, 0.3f, 0.5f), glm::vec3 scale = glm::vec3(1.0f), glm::vec3 angle = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4 color = glm::vec4(1.0f))
         : position(position), rotation(rotation), angle(angle), scale(scale), color(color) {
            std::pair<GLuint, GLuint> buffers = createVAOAndVBO();
            VAO = buffers.first;
            VBO = buffers.second;
         }

    // Load texture
    GLuint loadTexture(const char* path) const {
        // Load and create a texture 
        GLuint texture1;
        // ====================
        // Texture 1
        // ====================
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
        // Set our texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Load, create texture and generate mipmaps
        int width, height;
        unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
        if (image) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            glGenerateMipmap(GL_TEXTURE_2D);
            SOIL_free_image_data(image);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
        return texture1;
    }

    // Create and return VAO and VBO based on pyramidVertices
    std::pair<GLuint, GLuint> createVAOAndVBO() {
        GLuint VAO, VBO;

        // Generate and bind VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind and upload vertex data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(trapezoidVertices), trapezoidVertices, GL_STATIC_DRAW);

        // Bind the VAO and configure the vertex attributes
        glBindVertexArray(VAO);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // TexCoord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        // Unbind the VAO
        glBindVertexArray(0);

        // Return both VAO and VBO as a pair
        return std::make_pair(VAO, VBO);
    }

    // Draw the Trapezoid
    void draw(Shader &shader, const GLint &objectColorLoc, const GLint &modelLoc) const{
        // Set the brighter to false
        glUniform1f(glGetUniformLocation(shader.Program, "brighter"), false);
        glActiveTexture(GL_TEXTURE0);
        GLuint texture = loadTexture("./container_original copy.jpg"); // This works
        glBindTexture(GL_TEXTURE_2D, texture); // No texture
        glUniform1i(glGetUniformLocation(shader.Program, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), 0); // disable texture usage
        // Create a default model to adjust
        glm::mat4 model = glm::mat4(1.0f);
        // Set the position for this Trapezoid
        model = glm::translate(model, position);
        // Apply rotations in a specific order
        model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around x-axis
        model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around y-axis
        model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around z-axis
        // Set the scale for this Trapezoid
        model = glm::scale(model, scale);  
        // Set the model for this Trapezoid
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        // Set the color for this Trapezoid
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
        
        // Draw the Trapezoid using triangles
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
};

// Main function
int main()
{
    // Initialize Window
    GLFWwindow* window = windowInit();

    // Set the desired mouse sensitivity
    //camera.setMouseSensitivity(0.1f);

    //glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);

    // Build and compile our shader program
    Shader ourShader("Project5.vs", "Project5.frag");

    // Object Color and Model Location
    GLint objectColorLoc, modelLoc;

    // Create the background ------------------------------------------------------
    Cube Wall(
        glm::vec3(0.0f, 1.0f, -0.55f), 
        glm::vec3(1.0f, 0.3f, 0.5f), 
        glm::vec3(3.0f, 2.0f, 0.2f), 
        glm::vec3(0.0f), 
        glm::vec4(0.876f, 0.848f, 0.784f, 1.0f),
        "wall.jpg"
    );

    Cube Trim(
        glm::vec3(0.0f, 0.0f, -0.549f), 
        glm::vec3(1.0f, 0.3f, 0.5f), 
        glm::vec3(3.0f, 0.3f, 0.2f), 
        glm::vec3(0.0f), 
        glm::vec4(0.24f, 0.236f, 0.228f, 1.0f)
    );

    Cube Floor(
        glm::vec3(0.0f, -0.1f, 0.0f), 
        glm::vec3(1.0f, 0.3f, 0.5f), 
        glm::vec3(3.0f, 2.0f, 0.2f), 
        glm::vec3(90.0f, 0.0f, 0.0f), 
        glm::vec4(0.464f, 0.372f, 0.3f, 1.0f),
        "./floor.jpg"
    );
    // ----------------------------------------------------------------------------

    // Create TV Stand Parts ------------------------------------------------------
    std::vector<Cube> tvStandParts;

    // 1. Drawer Section
    Cube drawer(
        glm::vec3(0.0f, inchesToMeters(4.0f + 12.0f / 2), 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation (Uniform)
        glm::vec3(feetToMeters(4.81f), inchesToMeters(10.0f), feetToMeters(2.0f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(1.0f),                    // Color (wooden)
        "./wood_grain_rot.jpg"
    );
    tvStandParts.push_back(drawer);

    Cube drawerBottom(
        glm::vec3(0.0f, inchesToMeters(9.0f / 2), 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.81f), inchesToMeters(1.0f), feetToMeters(2.0f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    );
    tvStandParts.push_back(drawerBottom);

    Cube drawerSide(
        glm::vec3(0.0f, inchesToMeters(9.5f + 12.0f / 2), 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.81f), inchesToMeters(1.0f), feetToMeters(2.0f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        "./side.jpg"
    );
    tvStandParts.push_back(drawerSide);

    Cube drawerEdge(
        glm::vec3(-0.7f, inchesToMeters(9.521f + 12.0f / 2), 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.805f)/40, inchesToMeters(0.99f), feetToMeters(1.99f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        "./side.jpg"
    );
    tvStandParts.push_back(drawerEdge);

    Cube drawerReflection(
        glm::vec3(-0.64f, inchesToMeters(9.521f + 12.0f / 2), 0.0007f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.805f)/40, inchesToMeters(0.99f), feetToMeters(2.05f)), // Scale
        glm::vec3(0.0f, 15.0f, 0.0f),                                                    // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 0.6f),
        "./reflect.jpg"
    );
    tvStandParts.push_back(drawerReflection);
    Cube drawerReflection2(
        glm::vec3(0.0f, inchesToMeters(9.521f + 12.0f / 2), 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.805f)/20, inchesToMeters(0.99f), feetToMeters(1.99f)), // Scale
        glm::vec3(0.0f, 0.0f, 0.0f),                                                    // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 0.6f),
        "./reflect.jpg"
    );
    tvStandParts.push_back(drawerReflection2);

    Cube drawerTop(
        glm::vec3(0.0f, inchesToMeters(9.52f + 12.0f / 2), 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.805f), inchesToMeters(0.99f), feetToMeters(1.99f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 0.9f),
        "./base.jpg"
    );
    tvStandParts.push_back(drawerTop);

    // 1. Drawer Section
    Cube innerDrawer(
        glm::vec3(0.0f, inchesToMeters(4.0f + 12.0f / 2), 0.05f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(4.0f), inchesToMeters(10.0f), feetToMeters(1.8f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.288f, 0.188f, 0.16f, 1.0f),                    // Color (wooden)
        "./wood_grain.jpg"
    );
    tvStandParts.push_back(innerDrawer);

    // 1. Drawer Section
    Cube handle(
        glm::vec3(0.0f, inchesToMeters(4.0f + 12.0f / 2), 0.37f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.8f), inchesToMeters(1.0f), feetToMeters(0.1f)), // Scale
        glm::vec3(0.0f),                                                     // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),                       // Color (Black)
        "./handle.jpg"
    );
    tvStandParts.push_back(handle);

    Cube handleLeft(
        glm::vec3(-0.11f, inchesToMeters(4.0f + 12.0f / 2), 0.3f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(inchesToMeters(1.0f), inchesToMeters(1.0f), feetToMeters(0.4f)), // Scale
        glm::vec3(0.0f),                                          // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),                       // Color (Black)
        "./innerHandle.jpg"                     
    );
    tvStandParts.push_back(handleLeft);

    Cube handleRight(
        glm::vec3(0.11f, inchesToMeters(4.0f + 12.0f / 2), 0.3f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(inchesToMeters(1.0f), inchesToMeters(1.0f), feetToMeters(0.4f)), // Scale
        glm::vec3(0.0f),                                          // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),                       // Color (Black)
        "./innerHandle.jpg"
    );
    tvStandParts.push_back(handleRight);

    float xPos = 2.2f;
    float zPos = 0.95f;
    // Feet
    std::vector<glm::vec3> feetPositions = {
        glm::vec3(feetToMeters(-xPos), inchesToMeters(4.0f / 2), feetToMeters(-zPos + 0.05f)),
        glm::vec3(feetToMeters(xPos), inchesToMeters(4.0f / 2), feetToMeters(-zPos + 0.05f)),
        glm::vec3(feetToMeters(-xPos), inchesToMeters(4.0f / 2), feetToMeters(zPos - 0.05f)),
        glm::vec3(feetToMeters(xPos), inchesToMeters(4.0f / 2), feetToMeters(zPos - 0.05f))
    };
    for (auto& pos : feetPositions) {
        Cube foot(
            pos,
            glm::vec3(1.0f, 0.3f, 0.5f),
            glm::vec3(inchesToMeters(5.0f), inchesToMeters(4.0f), inchesToMeters(1.5f)),
            glm::vec3(0.0f), 
            glm::vec4(0.228f, 0.152f, 0.128f, 1.0f),
            "./wood_grain_rot.jpg"     
        );
        tvStandParts.push_back(foot);
    }

    // Posts
    std::vector<glm::vec3> postPositions = {
        glm::vec3(feetToMeters(-xPos-0.16f), inchesToMeters(4.0f + 12.0f + 12.0f / 2), feetToMeters(-zPos)),
        glm::vec3(feetToMeters(xPos+0.16f), inchesToMeters(4.0f + 12.0f + 12.0f / 2), feetToMeters(-zPos)),
        glm::vec3(feetToMeters(-xPos), inchesToMeters(4.0f + 12.0f + 12.0f / 2), feetToMeters(zPos)),
        glm::vec3(feetToMeters(xPos), inchesToMeters(4.0f + 12.0f + 12.0f / 2), feetToMeters(zPos)),
        glm::vec3(feetToMeters(0.0f), inchesToMeters(4.0f + 12.0f + 12.0f / 2), feetToMeters(-zPos))
    };
    int i = 0;
    glm::vec3 scale;
    for (auto& pos : postPositions) {
        if(i == 2 || i == 3) {
            scale = glm::vec3(inchesToMeters(5.0f), inchesToMeters(13.0f), inchesToMeters(0.75f));
        } else if (i == 4) {
            scale = glm::vec3(inchesToMeters(3.0f), inchesToMeters(13.0f), inchesToMeters(0.75f));
        } else {
            scale = glm::vec3(inchesToMeters(1.0f), inchesToMeters(13.0f), inchesToMeters(0.75f));
        }
        Cube post(
            pos,
            glm::vec3(1.0f, 0.3f, 0.5f),
            scale,
            glm::vec3(0.0f),
            glm::vec4(0.228f, 0.152f, 0.128f, 1.0f),
            "./wood_grain_rot.jpg"
        );
        i++;
        tvStandParts.push_back(post);
    }

    // Top Shelf
    Cube topShelfHighlight(
        glm::vec3(-0.58f, inchesToMeters(4.83f + 12.0f + 12.0f + 0.75f / 2), 0.0f),
        glm::vec3(1.0f, 0.3f, 0.5f),
        glm::vec3(feetToMeters(1.2f), inchesToMeters(0.1f), feetToMeters(2.0f)),
        glm::vec3(0.0f), 
        glm::vec4(1.0f, 1.0f, 1.0f, 0.6f),
        "./wall.jpg"
    );
    tvStandParts.push_back(topShelfHighlight);

    // Top Shelf
    Cube topShelf(
        glm::vec3(0.0f, inchesToMeters(4.5f + 12.0f + 12.0f + 0.75f / 2), 0.0f),
        glm::vec3(1.0f, 0.3f, 0.5f),
        glm::vec3(feetToMeters(5.0f), inchesToMeters(0.75f), feetToMeters(2.0f)),
        glm::vec3(0.0f), 
        glm::vec4(0.392f, 0.392f, 0.352f, 0.6f)
    );
    tvStandParts.push_back(topShelf);
    // ----------------------------------------------------------------------------

    // Draw the wii sensor bar ----------------------------------------------------

    float topShelfYPos = inchesToMeters(4.5f + 12.0f + 12.0f + 0.75f / 2);
    std::vector<Cube> sensorBar;

    Cube sensorBarCenter(
        glm::vec3(0.0f, topShelfYPos + inchesToMeters(0.5f), 0.0f), // Centered position
        glm::vec3(1.0f, 0.0f, 0.0f),   // Rotation axis (no rotation needed)
        glm::vec3(inchesToMeters(6.0f), inchesToMeters(0.5f), inchesToMeters(0.7f)), // Scale (6 inches wide)
        glm::vec3(0.0f),               // No rotation
        glm::vec4(0.75f, 0.75f, 0.75f, 1.0f)  // Color (Light gray)
    );
    sensorBar.push_back(sensorBarCenter);

    Cube sensorBarLeft(
        glm::vec3(-inchesToMeters(4.0f), topShelfYPos + inchesToMeters(0.5f), 0.0f), // Positioned left of the center (half of the center + half of the left)
        glm::vec3(1.0f, 0.0f, 0.0f),    // Rotation axis (no rotation needed)
        glm::vec3(inchesToMeters(2.0f), inchesToMeters(0.5f), inchesToMeters(0.7f)), // Scale (2 inches wide)
        glm::vec3(0.0f),                // No rotation
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)  // Color (Black)
    );
    sensorBar.push_back(sensorBarLeft);

    Cube sensorBarRight(
        glm::vec3(inchesToMeters(4.0f), topShelfYPos + inchesToMeters(0.5f), 0.0f), // Positioned right of the center (half of the center + half of the right)
        glm::vec3(1.0f, 0.0f, 0.0f),    // Rotation axis (no rotation needed)
        glm::vec3(inchesToMeters(2.0f), inchesToMeters(0.5f), inchesToMeters(0.7f)), // Scale (2 inches wide)
        glm::vec3(0.0f),                // No rotation
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)  // Color (Black)
    );
    sensorBar.push_back(sensorBarRight);
    // ----------------------------------------------------------------------------

    // Draw the towel -------------------------------------------------------------

    Towel towel(
        glm::vec3(-0.27f, topShelfYPos - inchesToMeters(0.5f), 0.04f), // Position on the shelf
        glm::vec3(0.1f),               // Default scale (already folded towel)
        glm::vec3(0.5f, 90.0f, 0.7f),
        glm::vec4(0.868f, 0.96f, 0.596f, 1.0f),  // Towel color (Greenish)
        "./towel.obj"
    );

    // ----------------------------------------------------------------------------

    // Draw the wii ---------------------------------------------------------------
    std::vector<Trapezoid> wii;

    Trapezoid base(
        glm::vec3( 0.56f,  0.432f,  0.14f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.13f, 0.05f, 0.06f),  // Scale
        glm::vec3(0.0f, -85.0f, 0.0f),      // Angle
        glm::vec4(0.44f, 0.42f, 0.42f, 1.0f)    // Color
    );
    wii.push_back(base);

    Trapezoid console(
        glm::vec3( 0.559f,  0.527f,  0.09f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.12f, 0.2f, 0.05f),  // Scale
        glm::vec3(-10.0f, -85.0f, 0.0f),      // Angle
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)    // Color
    );
    wii.push_back(console);

    std::vector<Cube> wiiDetails;

    Cube discSlot(
        glm::vec3( 0.574f,  0.545f,  0.13f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.006f, 0.16f, 0.01f),  // Scale
        glm::vec3(-18.0f, 0.0f, 1.0f),      // Angle
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)    // Color
    );
    wiiDetails.push_back(discSlot);
    Cube powerButton(
        glm::vec3( 0.545f,  0.61f,  0.115f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.02f, 0.01f, 0.01f),  // Scale
        glm::vec3(-18.0f, 0.0f, 1.0f),      // Angle
        glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)    // Color
    );
    wiiDetails.push_back(powerButton);
    Cube buttonLight(
        glm::vec3( 0.5425f,  0.6125f,  0.121f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.004f, 0.004f, 0.002f),  // Scale
        glm::vec3(-18.0f, 0.0f, 1.0f),      // Angle
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)    // Color
    );
    wiiDetails.push_back(buttonLight);
    Cube resetButton(
        glm::vec3( 0.545f,  0.595f,  0.12f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.02f, 0.006f, 0.01f),  // Scale
        glm::vec3(-18.0f, 0.0f, 1.0f),      // Angle
        glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)    // Color
    );
    wiiDetails.push_back(resetButton);
    Cube hdmiPort(
        glm::vec3( 0.545f,  0.545f,  0.138f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.02f, 0.08f, 0.01f),  // Scale
        glm::vec3(-18.0f, 0.0f, 1.0f),      // Angle
        glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)    // Color
    );
    wiiDetails.push_back(hdmiPort);
        Cube ejectButton(
        glm::vec3( 0.548f,  0.48f,  0.157f),
        glm::vec3(1.0f, 0.3f, 0.5f),    // Rotation
        glm::vec3(0.02f, 0.01f, 0.01f),  // Scale
        glm::vec3(-18.0f, 0.0f, 1.0f),      // Angle
        glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)    // Color
    );
    wiiDetails.push_back(ejectButton);
    // ----------------------------------------------------------------------------

    // Draw the stacks of wii games -----------------------------------------------
    std::vector<wiiGame> wiiGames;

    glm::vec3 gamePositions[] = {
        glm::vec3( 0.13f,  0.417f,  0.05f),
        glm::vec3( 0.13f,  0.437f,  0.055f),
        glm::vec3( 0.13f,  0.457f,  0.04f),
        glm::vec3( 0.13f,  0.477f,  0.07f),
        glm::vec3( 0.13f,  0.497f,  0.05f),
        glm::vec3( 0.13f,  0.517f,  0.05f),
        glm::vec3( 0.13f,  0.537f,  0.05f),
        glm::vec3( 0.13f,  0.557f,  0.09f),


        glm::vec3( 0.34f,  0.417f,  0.05f),
        glm::vec3( 0.32f,  0.437f,  0.07f),
        glm::vec3( 0.33f,  0.457f,  0.05f),
        glm::vec3( 0.35f,  0.477f,  0.02f),
        glm::vec3( 0.34f,  0.497f,  0.05f),
        glm::vec3( 0.33f,  0.517f,  0.06f),
        glm::vec3( 0.325f,  0.537f,  0.05f),
        glm::vec3( 0.34f,  0.557f,  0.03f),
        glm::vec3( 0.36f,  0.577f,  0.06f)
    };

    float gameRotations[] = {
	    0.0f,
	   -1.0f,
        3.5f,
        3.5f,
	    0.0f,
        2.0f,
        5.0f,
       -5.0f,


    	0.0f,
       -2.5f,
       -0.5f,
        0.0f,
	    1.0f,
        3.0f,
        3.0f,
        0.0f,
       -6.5f
    };    

    const char* gameTextures[] = {
        "./game1.jpg",
        "./game3.jpg",
        "./game4.jpg",
        "./game5.jpg",
        "./game6.jpg",
        "./game7.jpg",
        "./game8.jpg",
        "./game2.jpg",

        "./game9.jpg",
        "./game10.jpg",
        "./game11.jpg",
        "./game12.jpg",
        "./game13.jpg",
        "./game14.jpg",
        "./game12.jpg",
        "./game13.jpg",
        "./game3.jpg",
    };

    glm::vec4 colors[] = {
        glm::vec4(0.904f, 0.88f, 0.832f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
        glm::vec4(0.904f, 0.88f, 0.832f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.982f, 0.984f, 0.96f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),

        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.928f, 0.94f, 0.9f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.928f, 0.94f, 0.9f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
        glm::vec4(0.982f, 0.964f, 0.932f, 1.0f),
    };

    i = 0;
    for (auto& pos : gamePositions) {
        wiiGame game(
            pos,
            glm::vec3(1.0f, 0.3f, 0.5f),
            glm::vec3(0.17f, 0.3f, 0.02f),
            glm::vec3(90.0f, 0.0f, gameRotations[i]),
            colors[i],
            gameTextures[i]
        );
        i++;
        wiiGames.push_back(game);
    }
    // ----------------------------------------------------------------------------

    // Draw the Television --------------------------------------------------------
    std::vector<Cube> TelevisionParts;

    // Create TV
    Cube tvCase(
        glm::vec3(0.0f, 1.11f, 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(3.5f), inchesToMeters(23.0f), feetToMeters(0.1f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.352f, 0.352f, 0.352f, 1.0f)
    );
    TelevisionParts.push_back(tvCase);
    // Create TV
    Cube tvScreen(
        glm::vec3(0.0f, 1.11f, 0.001f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(3.45f), inchesToMeters(22.5f), feetToMeters(0.1f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.168f, 0.168f, 0.168f, 0.9f),                    // Color (wooden)
        "./tv.jpg"
    );
    TelevisionParts.push_back(tvScreen);

    Cube bottomBar(
        glm::vec3(0.0f, 0.81f, 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(3.52f), inchesToMeters(0.8f), feetToMeters(0.125f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.352f, 0.352f, 0.352f, 1.0f)
    );
    TelevisionParts.push_back(bottomBar);

    Cube redLight(
        glm::vec3(-0.05f, 0.795f, 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.02f), inchesToMeters(0.25f), feetToMeters(0.05f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
    );
    TelevisionParts.push_back(redLight);

    Cube redCase(
        glm::vec3(-0.05f, 0.795f, 0.0f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.08f), inchesToMeters(0.35f), feetToMeters(0.05f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.776f, 0.268f, 0.276f, 0.7f)
    );
    TelevisionParts.push_back(redCase);

    Cube sticker1(
        glm::vec3(-0.4965f, 1.30f, 0.01f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.18f), inchesToMeters(6.5f), feetToMeters(0.05f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.848f, 0.756f, 0.092f, 1.0f)
    );
    TelevisionParts.push_back(sticker1);

    Cube sticker2(
        glm::vec3(0.4965f, 1.365f, 0.01f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.18f), inchesToMeters(2.20f), feetToMeters(0.05f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.848f, 0.756f, 0.092f, 1.0f)
    );
    TelevisionParts.push_back(sticker2);

    Cube sticker3(
        glm::vec3(0.471f, 0.875f, 0.01f), // Position
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.18f), inchesToMeters(1.10f), feetToMeters(0.05f)), // Scale
        glm::vec3(0.0f),                                                    // Angle
        glm::vec4(0.996f, 0.98f, 0.972f, 1.0f)
    );
    TelevisionParts.push_back(sticker3);
    Cube tvStandConnect1(
        glm::vec3(0.431f, 0.79f, 0.0f),
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.058f), inchesToMeters(1.60f), feetToMeters(0.075f)),
        glm::vec3(0.0f, 0.0f, 0.0f),                                                    // Angle
        glm::vec4(0.18f, 0.188f, 0.184f, 1.0f)
    );
    TelevisionParts.push_back(tvStandConnect1);

    Cube tvStandConnect2(
        glm::vec3(-0.431f, 0.79f, 0.0f),
        glm::vec3(1.0f, 0.3f, 0.5f),                                        // Rotation axis
        glm::vec3(feetToMeters(0.058f), inchesToMeters(1.60f), feetToMeters(0.075f)),
        glm::vec3(0.0f, 0.0f, 0.0f),                                                    // Angle
        glm::vec4(0.18f, 0.188f, 0.184f, 1.0f)
    );
    TelevisionParts.push_back(tvStandConnect2);

    std::vector<Pyramid> tvStands;
    Pyramid stand1;
    stand1.position = glm::vec3(0.44f, 0.76f, 0.05f);
    stand1.angle = glm::vec3(115.0f, 0.0f, -10.0f);
    stand1.scale = glm::vec3(feetToMeters(0.06f), inchesToMeters(5.10f), feetToMeters(0.075f));
    stand1.color = glm::vec4(0.18f, 0.188f, 0.184f, 1.0f);
    tvStands.push_back(stand1);
    
    Pyramid stand2;
    stand2.position = glm::vec3(0.44f, 0.76f, -0.055f);
    stand2.angle = glm::vec3(70.0f, 0.0f, -170.0f);
    stand2.scale = glm::vec3(feetToMeters(0.06f), inchesToMeters(5.10f), feetToMeters(0.075f));
    stand2.color = glm::vec4(0.18f, 0.188f, 0.184f, 1.0f);
    tvStands.push_back(stand2);

    Pyramid stand3;
    stand3.position = glm::vec3(-0.44f, 0.76f, 0.05f);
    stand3.angle = glm::vec3(115.0f, 0.0f, 10.0f);
    stand3.scale = glm::vec3(feetToMeters(0.06f), inchesToMeters(5.10f), feetToMeters(0.075f));
    stand3.color = glm::vec4(0.18f, 0.188f, 0.184f, 1.0f);
    tvStands.push_back(stand3);
    
    Pyramid stand4;
    stand4.position = glm::vec3(-0.44f, 0.76f, -0.055f);
    stand4.angle = glm::vec3(70.0f, 0.0f, -190.0f);
    stand4.scale = glm::vec3(feetToMeters(0.06f), inchesToMeters(5.10f), feetToMeters(0.075f));
    stand4.color = glm::vec4(0.18f, 0.188f, 0.184f, 1.0f);
    tvStands.push_back(stand4);
    // ----------------------------------------------------------------------------


    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();
        // Calculate deltatime
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Handle Input
        do_movement();

        // Set up the OpenGL state and return the objectColorLoc and modelLoc
        SetupOpenGLState(ourShader, objectColorLoc, modelLoc);

        // Bind the objects VAO
        glBindVertexArray(wii[0].VAO); 
        // Draw the wii
        for (const auto& piece : wii)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), piece.color.w);
            piece.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Bind the objects VAO
        glBindVertexArray(wiiDetails[0].VAO); 
        // Draw the wii game stacks
        for (const auto& detail : wiiDetails)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), detail.color.w);
            detail.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Bind the objects VAO
        glBindVertexArray(wiiGames[0].VAO); 
        // Draw the wii game stacks
        for (const auto& game : wiiGames)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), game.color.w);
            game.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);
        
        // Bind the objects VAO
        glBindVertexArray(TelevisionParts[0].VAO); 
        // Draw the wii game stacks
        for (const auto& part : TelevisionParts)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), part.color.w);
            part.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Bind the objects VAO
        glBindVertexArray(sensorBar[0].VAO); 
        // Draw the wii sensor bar
        for (const auto& part : sensorBar)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), part.color.w);
            part.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Bind the objects VAO
        glBindVertexArray(towel.VAO); 
        // Set the object color alpha value to the part's alpha
        glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), towel.color.w);
        towel.draw(ourShader, objectColorLoc, modelLoc);
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Bind the objects VAO
        glBindVertexArray(Wall.VAO); 
        // Set the object color alpha value to the part's alpha
        glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), Wall.color.w);
        Wall.draw(ourShader, objectColorLoc, modelLoc);
        // Unbind the objects VAO
        glBindVertexArray(0); 

        // Bind the objects VAO
        glBindVertexArray(Trim.VAO); 
        // Set the object color alpha value to the part's alpha
        glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), Trim.color.w);
        Trim.draw(ourShader, objectColorLoc, modelLoc);
        // Unbind the objects VAO
        glBindVertexArray(0); 

        // Bind the objects VAO
        glBindVertexArray(Floor.VAO); 
        // Set the object color alpha value to the part's alpha
        glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), Floor.color.w);
        Floor.draw(ourShader, objectColorLoc, modelLoc);
        // Unbind the objects VAO
        glBindVertexArray(0); 

        // Bind the objects VAO
        glBindVertexArray(tvStandParts[0].VAO);    
        // Draw the TV stand parts
        for (const auto& part : tvStandParts)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), part.color.w);
            part.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Bind the objects VAO
        glBindVertexArray(tvStands[0].VAO); 
        for (const auto& part : tvStands)
        {
            // Set the object color alpha value to the part's alpha
            glUniform1f(glGetUniformLocation(ourShader.Program, "objectAlpha"), part.color.w);
            part.draw(ourShader, objectColorLoc, modelLoc);
        }
        // Unbind the objects VAO
        glBindVertexArray(0);

        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    // Cleanup: Delete the VAOs and VBOs for each object to avoid memory leaks.
    
    // Background cubes
    glDeleteVertexArrays(1, &Wall.VAO);
    glDeleteBuffers(1, &Wall.VBO);
    glDeleteVertexArrays(1, &Trim.VAO);
    glDeleteBuffers(1, &Trim.VBO);
    glDeleteVertexArrays(1, &Floor.VAO);
    glDeleteBuffers(1, &Floor.VBO);

    // TV Stand Parts
    for (auto& part : tvStandParts) {
        glDeleteVertexArrays(1, &part.VAO);
        glDeleteBuffers(1, &part.VBO);
    }

    // Wii and Wii details
    for (auto& piece : wii) {
        glDeleteVertexArrays(1, &piece.VAO);
        glDeleteBuffers(1, &piece.VBO);
    }
    for (auto& detail : wiiDetails) {
        glDeleteVertexArrays(1, &detail.VAO);
        glDeleteBuffers(1, &detail.VBO);
    }

    // Wii Games
    for (auto& game : wiiGames) {
        glDeleteVertexArrays(1, &game.VAO);
        glDeleteBuffers(1, &game.VBO);
    }

    // TV
    for (auto& part : TelevisionParts) {
        glDeleteVertexArrays(1, &part.VAO);
        glDeleteBuffers(1, &part.VBO);
    }

    // Sensor Bar
    for (auto& part : sensorBar) {
        glDeleteVertexArrays(1, &part.VAO);
        glDeleteBuffers(1, &part.VBO);
    }

    // Towel
    glDeleteVertexArrays(1, &towel.VAO);
    glDeleteBuffers(1, &towel.VBO);

    // TV Stand legs
    for (auto& stand : tvStands) {
        glDeleteVertexArrays(1, &stand.VAO);
        glDeleteBuffers(1, &stand.VBO);
    }

    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}

// GLFW window initialization function
GLFWwindow* windowInit()
{
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "CST-310 Project 5", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // GLFW Options
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    glewInit();

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);
    // Camera/View transformation
    glm::mat4 view;
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

// Initialization Function for OpenGL state
void SetupOpenGLState(Shader& ourShader, GLint& objectColorLoc, GLint& modelLoc) {
    // Clear the color buffer
    glClearColor(0.894f, 0.824f, 0.980f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use corresponding shader when setting uniforms/drawing objects
    ourShader.Use();
    // Grab lighting variables
    objectColorLoc = glGetUniformLocation(ourShader.Program, "objectColor");
    GLint lightColorLoc  = glGetUniformLocation(ourShader.Program, "lightColor");
    GLint lightPosLoc    = glGetUniformLocation(ourShader.Program, "lightPos");
    GLint viewPosLoc     = glGetUniformLocation(ourShader.Program, "viewPos");

    // Set variables
    glUniform3f(lightColorLoc,  1.0f, 1.0f, 1.0f);
    glUniform3f(lightPosLoc,    lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(viewPosLoc,     camera.Position.x, camera.Position.y, camera.Position.z);

    // Create camera transformations
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);

    // Get the uniform locations for model, view, and projection
    modelLoc = glGetUniformLocation(ourShader.Program, "model");
    GLint viewLoc  = glGetUniformLocation(ourShader.Program, "view");
    GLint projLoc  = glGetUniformLocation(ourShader.Program, "projection");

    // Pass the matrices to the shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        mouseMovementEnabled = !mouseMovementEnabled; // Toggle mouse movement
        if (mouseMovementEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Re-enable mouse cursor control
            std::cout << "Yaw: " << camera.Yaw << "\nPitch: " << camera.Pitch << std::endl;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show mouse cursor
        }
    }
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}


void do_movement()
{
    // Camera controls
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

bool firstMouse = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!mouseMovementEnabled) return; // Ignore mouse movement if it's disabled
    if(firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left
    

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset/3, yoffset/3);
}	
