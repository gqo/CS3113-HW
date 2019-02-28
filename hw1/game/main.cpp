#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

/*
    [
        Top 0, left 1,
        Top 2, right 3,
        Bottom 4, right 5,
        Bottom 6, left 7,
    ]
*/

void createSquare(float vertices[8], float (&trianglesArray)[12]) {
    trianglesArray[0] = vertices[2]; trianglesArray[1] = vertices[3];
    trianglesArray[2] = vertices[0]; trianglesArray[3] = vertices[1];
    trianglesArray[4] = vertices[6]; trianglesArray[5] = vertices[7];

    trianglesArray[6] = vertices[6]; trianglesArray[7] = vertices[7];
    trianglesArray[8] = vertices[4]; trianglesArray[9] = vertices[5];
    trianglesArray[10] = vertices[2]; trianglesArray[11] = vertices[3];
}

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

    if(image == NULL) {
        std::cout << "Unable to load image @ path: " << filePath << ". Make sure the path is correct\n";
        assert(false);
    }

    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return retTexture;
}

const float PI = 3.14159265358979323846f;

SDL_Window* displayWindow;

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
    // Setup
    glViewport(0,0,1280,720);

    ShaderProgram program;
    program.Load("vertex.glsl", "fragment.glsl");

    ShaderProgram texturedProgram;
    texturedProgram.Load("vertex_textured.glsl","fragment_textured.glsl");

    GLuint sandTexture = LoadTexture("../assets/sand.png");
    GLuint lizardTexture = LoadTexture("../assets/lizard.png");
    GLuint cactusTexture = LoadTexture("../assets/cactus_sq.png");    

    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);

    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    texturedProgram.SetProjectionMatrix(projectionMatrix);
    texturedProgram.SetViewMatrix(viewMatrix);

    glClearColor(0.5294f, 0.8078f, 0.9216f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float angle, gameAngle;
    float angle45 = 45 * (PI / 180.0f);
    float angle22point5 = 22.5 * (PI / 180.0f);
    float lastFrameTicks = 0.0f;

    // Main loop
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }

        float ticks = (float)SDL_GetTicks()/2500.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;

        gameAngle += elapsed;

        glClear(GL_COLOR_BUFFER_BIT);
        // Make a square (two triangles)
        float squareVertices[] = {
            -0.5f, 0.5f,
            0.5f, 0.5f,
            0.5f, -0.5f,
            -0.5f, -0.5f,
        };

        float squareTriangles[12]; 
        createSquare(squareVertices, squareTriangles);

        // Untextured
        glUseProgram(program.programID);
        // Draw orange sun layer 1
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, squareTriangles);
        glEnableVertexAttribArray(program.positionAttribute);
        program.SetColor(0.9686f, 0.3529f, 0.627f, 1.0f);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1.8f, 1.0f, 0.0f));
        angle = angle22point5 + gameAngle;
        modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.75f, 1.0f));
        program.SetModelMatrix(modelMatrix);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw orange sun layer 2
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, squareTriangles);
        glEnableVertexAttribArray(program.positionAttribute);
        program.SetColor(0.9686f, 0.3529f, 0.627f, 1.0f);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1.8f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.75f, 1.0f));
        angle = angle45 + angle22point5 + gameAngle;
        modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        program.SetModelMatrix(modelMatrix);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw yellow sun layer 1
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, squareTriangles);
        glEnableVertexAttribArray(program.positionAttribute);
        program.SetColor(0.9765f, 0.8431f, 0.1098f, 1.0f);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1.8f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.75f, 1.0f));
        angle = gameAngle;
        modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        program.SetModelMatrix(modelMatrix);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw yellow sun layer 2
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, squareTriangles);
        glEnableVertexAttribArray(program.positionAttribute);
        program.SetColor(0.9765f, 0.8431f, 0.1098f, 1.0f);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1.8f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.75f, 1.0f));
        angle = angle45 + gameAngle;
        modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        program.SetModelMatrix(modelMatrix);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Textured
        // Align textures
        glUseProgram(texturedProgram.programID);

        glVertexAttribPointer(texturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, squareTriangles);
        glEnableVertexAttribArray(texturedProgram.positionAttribute);

        float texCoords[] = {
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0,
            1.0, 1.0,
            0.0, 1.0,
            0.0, 0.0,
        };
        glVertexAttribPointer(texturedProgram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(texturedProgram.texCoordAttribute);

        // Draw sand
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.75f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(3.75f, 1.0f, 1.0f));
        texturedProgram.SetModelMatrix(modelMatrix);

        glBindTexture(GL_TEXTURE_2D, sandTexture);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw cactus
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.80f, 0.05f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.80f, 0.80f, 1.0f));
        texturedProgram.SetModelMatrix(modelMatrix);

        glBindTexture(GL_TEXTURE_2D, cactusTexture);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw lizard
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.70f, -0.55f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.40f, 0.40f, 1.0f));
        texturedProgram.SetModelMatrix(modelMatrix);

        glBindTexture(GL_TEXTURE_2D, lizardTexture);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Clean up
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(texturedProgram.positionAttribute);
        glDisableVertexAttribArray(texturedProgram.texCoordAttribute);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}