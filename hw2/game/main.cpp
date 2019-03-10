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

class Ticks {
    public:

        float paddleLastTicks = 0.0f;
        float ballLastTicks = 0.0f;
};

class Entity {
    public:

        void Draw(ShaderProgram &p);
        void SetColor(float r, float g, float b);
        void Move(float elapsed);

        float x;
        float y;
        float rotation = 0.0f;

        float r, g, b;

        float width;
        float height;

        float velocity = 1.0f; // unit; // unit per second
        float direction_x = 0.0f;
        float direction_y = 0.0f;
};

void Entity::SetColor(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

void createSquare(float x, float y, float width, float height, float(&squareArray)[8]) {
    squareArray[0] = x; squareArray[1] = y;
    squareArray[2] = x + width; squareArray[3] = y;
    squareArray[4] = x + width; squareArray[5] = y - height;
    squareArray[6] = x; squareArray[7] = y - height;
}

void triangularizeSquare(float vertices[8], float (&trianglesArray)[12]) {
    trianglesArray[0] = vertices[2]; trianglesArray[1] = vertices[3];
    trianglesArray[2] = vertices[0]; trianglesArray[3] = vertices[1];
    trianglesArray[4] = vertices[6]; trianglesArray[5] = vertices[7];

    trianglesArray[6] = vertices[6]; trianglesArray[7] = vertices[7];
    trianglesArray[8] = vertices[4]; trianglesArray[9] = vertices[5];
    trianglesArray[10] = vertices[2]; trianglesArray[11] = vertices[3];
}

void Entity::Draw(ShaderProgram &p) {
    float squareVerticies[8];
    createSquare(this->x, this->y, this->width, this->height, squareVerticies);
    float triangleVerticies[12];
    triangularizeSquare(squareVerticies, triangleVerticies);

    glUseProgram(p.programID);
    glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, triangleVerticies);
    glEnableVertexAttribArray(p.positionAttribute);
    p.SetColor(r, g, b, 1.0f);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    p.SetModelMatrix(modelMatrix);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Entity::Move(float elapsed) {
    this->x += this->direction_x * elapsed * this->velocity;
    this->y += this->direction_y * elapsed * this->velocity;
}

bool CheckEntityCollision(const Entity left, const Entity right) {
    float left_x = left.x + (left.width / 2);
    float left_y = left.y - (left.height / 2);

    float right_x = right.x + (right.width / 2);
    float right_y = right.y - (right.height / 2);

    float p_x = glm::abs(left_x - right_x) - ((left.width + right.width) / 2);
    float p_y = glm::abs(left_y - right_y) - ((left.height + right.height) / 2);

    if ((p_x < 0.0f) && (p_y < 0.0f)) {
        return true;
    }

    return false;
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
const float unit = 0.0625f;
const bool RED_WIN = true;
const bool BLUE_WIN = false;

// Game Data
SDL_Window* displayWindow;

ShaderProgram program;
glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;

Entity leftPaddle, rightPaddle, topBoundary, bottomBoundary, middleBar, ball, leftBar, rightBar;

SDL_Event event;
bool done = false;

const Uint8 *keys;

Ticks gameTicks;

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
    // Setup
    glViewport(0,0,1280,720);

    program.Load("vertex.glsl", "fragment.glsl");  

    projectionMatrix = glm::mat4(1.0f);
    viewMatrix = glm::mat4(1.0f);

    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    leftPaddle.x = -1.5f;
    leftPaddle.y = 0.125f;
    leftPaddle.width = unit;
    leftPaddle.height = unit * 4;
    leftPaddle.SetColor(1.0f, 0.0f, 0.0f);

    rightPaddle = leftPaddle;
    rightPaddle.x = 1.5f;
    rightPaddle.SetColor(0.0f, 0.0f, 1.0f);

    topBoundary.x = -1.777f;
    topBoundary.y = 1.0f;
    topBoundary.width = 1.777f * 2;
    topBoundary.height = unit;
    topBoundary.SetColor(1.0f, 1.0f, 1.0f);

    bottomBoundary = topBoundary;
    bottomBoundary.y = -1.0f + unit;

    middleBar.x = 0.0f - (unit / 2);
    middleBar.y = 1.0f;
    middleBar.width = unit;
    middleBar.height = 2.0f;
    middleBar.SetColor(0.8627f, 0.8627f, 0.8627f);

    leftBar = middleBar;
    leftBar.x = -1.777f;
    leftBar.SetColor(0.0f, 1.0f, 0.0f);

    rightBar = leftBar;
    rightBar.x = 1.777f - unit;

    ball.x = 0.0f - (unit / 2);
    ball.y = 0.0f + (unit / 2);
    ball.width = unit;
    ball.height = unit;
    ball.direction_x = 1.0f;
    ball.direction_y = -1.0f;
    ball.SetColor(1.0f, 1.0f, 0.0f);

    keys = SDL_GetKeyboardState(NULL);
}

void ProcessEvents() {
    // Is player blue moving in the y direction?
    if(keys[SDL_SCANCODE_UP]) {
        rightPaddle.direction_y = 1.0f;
    } else if (keys[SDL_SCANCODE_DOWN]) {
        rightPaddle.direction_y = -1.0f;
    } else {
        rightPaddle.direction_y = 0.0f;
    }

    // Is player red moving in the y direction?
    if(keys[SDL_SCANCODE_W]) {
        leftPaddle.direction_y = 1.0f;
    } else if(keys[SDL_SCANCODE_S]) {
        leftPaddle.direction_y = -1.0f;
    } else {
        leftPaddle.direction_y = 0.0f;
    }

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        } else if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                done = true;
            }
        }
    }
}

// winner?
// red = true
// blue = false
void WinReset(bool winner) {
    // Change wall color based on win
    if (winner == RED_WIN) {
        topBoundary.SetColor(1.0f, 0.0f, 0.0f);
        bottomBoundary.SetColor(1.0f, 0.0f, 0.0f);
    } else {
        topBoundary.SetColor(0.0f, 0.0f, 1.0f);
        bottomBoundary.SetColor(0.0f, 0.0f, 1.0f);
    }

    // Reset paddles
    leftPaddle.y = 0.125f;
    rightPaddle.y = 0.125f;

    // Reset ball
    ball.x = 0.0f - (unit / 2);
    ball.y = 0.0f + (unit / 2);
    ball.velocity = 1.0f;
}

void Update() {
    float paddleTicks = (float)SDL_GetTicks()/1500.0f;
    float paddleElapsed = paddleTicks - gameTicks.paddleLastTicks;
    gameTicks.paddleLastTicks = paddleTicks;

    float ballTicks = (float)SDL_GetTicks()/2000.0f;
    float ballElapsed = ballTicks - gameTicks.ballLastTicks;
    gameTicks.ballLastTicks = ballTicks;

    // Ball increases in speed over time
    ball.velocity += (ballElapsed / 10);

    leftPaddle.Move(paddleElapsed);
    rightPaddle.Move(paddleElapsed);
    ball.Move(ballElapsed);

    if (CheckEntityCollision(ball, leftPaddle)) {
        ball.direction_x = 1.0f;
    } else if (CheckEntityCollision(ball, rightPaddle)) {
        ball.direction_x = -1.0f;
    }

    if (CheckEntityCollision(ball, topBoundary)) {
        ball.direction_y = -1.0f;
    } else if (CheckEntityCollision(ball, bottomBoundary)) {
        ball.direction_y = 1.0f;
    }

    if (CheckEntityCollision(ball, leftBar)) {
        WinReset(BLUE_WIN);
    } else if (CheckEntityCollision(ball, rightBar)) {
        WinReset(RED_WIN);
    }

    if (CheckEntityCollision(leftPaddle, topBoundary) || CheckEntityCollision(leftPaddle, bottomBoundary)) {
        leftPaddle.Move(-1.0f * paddleElapsed);
    }

    if (CheckEntityCollision(rightPaddle, topBoundary) || CheckEntityCollision(rightPaddle, bottomBoundary)) {
        rightPaddle.Move(-1.0f * paddleElapsed);
    }
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    leftPaddle.Draw(program);
    rightPaddle.Draw(program);

    middleBar.Draw(program);
    leftBar.Draw(program);
    rightBar.Draw(program);

    topBoundary.Draw(program);
    bottomBoundary.Draw(program);

    ball.Draw(program);

    glDisableVertexAttribArray(program.positionAttribute);
    SDL_GL_SwapWindow(displayWindow);
}

void Cleanup() {
    // Does nothing currently
}

int main(int argc, char *argv[])
{
    Setup();

    while (!done) {
        ProcessEvents();
        Update();
        Render();
    }

    Cleanup();
    SDL_Quit();
    return 0;
}