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

#include <vector>

#define MAX_BULLETS 30
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

void log(const std::string &message) {
    std::cout << message << std::endl;
}

// Global Data

enum GameMode { 
    MAIN_MENU,
    GAME_LEVEL,
    GAME_OVER,
};

GameMode gameMode = GAME_OVER;

const float PI = 3.14159265358979323846f;
const float unit = 0.0625f;

GLuint fontTexture, spriteTexture, borderTexture;

SDL_Window* displayWindow;

ShaderProgram program;
ShaderProgram untextured_program;

glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;

SDL_Event event;

bool done = false;

const Uint8 *keys;

// End Global Data

class Ticks {
    public:

        float playerLastTicks = 0.0f;
        float enemyLastTicks = 0.0f;
};

class SpriteData {
    public:
        SpriteData(){};
        SpriteData(float u, float v, float width, float height):
        u(u), v(v), width(width), height(height){};

        float u, v;
        float width, height;
};

class SheetSprite {
    public:
        SheetSprite(){};
        SheetSprite(GLuint textureID, float u, float v, float width, 
            float height, float size):
            textureID(textureID), u(u),v(v), width(width), height(height),
            size(size){};
        SheetSprite(GLuint textureID, SpriteData data, float size):
            textureID(textureID), u(data.u), v(data.v), width(data.width),
            height(data.height), size(size){};

        float size;
        GLuint textureID;
        float u;
        float v;
        float width;
        float height;

        void Draw(ShaderProgram &program);
};

void SheetSprite::Draw(ShaderProgram &p) {
    glBindTexture(GL_TEXTURE_2D, this->textureID);

    GLfloat texCoords[] = {
        u, v + height,
        u + width, v,
        u, v,
        u + width, v,
        u, v + height,
        u + width, v + height
    };

    float aspect = width / height;
    float vertices[] = {
        -0.5f * size * aspect, -0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, 0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, -0.5f * size,
        0.5f * size * aspect, -0.5f * size,
    };

    glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(p.positionAttribute);

    glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(p.texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(p.positionAttribute);
    glDisableVertexAttribArray(p.texCoordAttribute);
}

class Entity {
    public:

        float x, y;
        float z = 0.0f;
        float rotation = 0.0f;

        float r, g, b; // color

        float width;
        float height;

        float velocity_x = 1.0f;
        float velocity_y = 1.0f;
        float velocity_z = 0.0f;

        float direction_x = 0.0f;
        float direction_y = 0.0f;
        float direction_z = 0.0f;

        SheetSprite sprite;

        void Draw(ShaderProgram &p);
        void SetColor(float r, float g, float b);
        void Move(float elapsed);
};

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

class TextEntity {
    public:
        std::string entity_text;
        GLuint fontTexture;
        float size;
        float spacing;
        float x, y;

        void DrawText(ShaderProgram &p);
};

void TextEntity::DrawText(ShaderProgram &p) {
    float char_size = 1.0/16.0f;

    std::vector<float> vertexData;
    std::vector<float> textCoordData;

    for(int i=0; i < entity_text.size(); i++) {
        int spriteIndex = (int)entity_text[i];

        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });

        textCoordData.insert(textCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + char_size,
            texture_x + char_size, texture_y,
            texture_x + char_size, texture_y + char_size,
            texture_x + char_size, texture_y,
            texture_x, texture_y + char_size,
        });
    }

    glBindTexture(GL_TEXTURE_2D, fontTexture);

    glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(p.positionAttribute);

    glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, textCoordData.data());
    glEnableVertexAttribArray(p.texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6 * (int)entity_text.size());

    glDisableVertexAttribArray(p.positionAttribute);
    glDisableVertexAttribArray(p.texCoordAttribute);
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
    createSquare(x, y, width, height, squareVerticies);
    float triangleVerticies[12];
    triangularizeSquare(squareVerticies, triangleVerticies);

    glUseProgram(p.programID);
    glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, triangleVerticies);
    glEnableVertexAttribArray(p.positionAttribute);
    p.SetColor(r, g, b, 1.0f);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    p.SetModelMatrix(modelMatrix);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(p.positionAttribute);
}

void Entity::SetColor(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

void Entity::Move(float elapsed) {
    this->x += this->direction_x * elapsed * this->velocity_x;
    this->y += this->direction_y * elapsed * this->velocity_y;
    this->z += this->direction_z * elapsed * this->velocity_z;
}

class MenuState {
    public:
        TextEntity titleText;
        TextEntity subtitleText;
        TextEntity startText;

        void Setup();
        void Render();
        void ProcessEvents();
};

class GameState {
    public:
        Entity player;
        Entity enemies[36];
        Entity bullets[30];

        int score = 0;
        int final_score = 0;
        int enemiesLeft = -1;
        int bulletIndex = 0;
        Ticks ticks;

        void Setup();
        void Render();
        void Update(float elapsed);
        void ProcessEvents();
};

GameState gameState;

class OverState {
    public:
        TextEntity overText;
        TextEntity scoreText;
        TextEntity restartText;

        void Setup();
        void Render();
        void ProcessEvents();
};

OverState overState;

void MenuState::Setup() {
    // Setup menu state entities
    this->titleText.fontTexture = fontTexture;
    this->titleText.entity_text = "Space Invaders";
    this->titleText.size = 0.125f;
    this->titleText.spacing = 0.0f;
    this->titleText.x = -0.8f;
    this->titleText.y = 1.0f;

    this->subtitleText.fontTexture = fontTexture;
    this->subtitleText.entity_text = "by Graeme Ferguson";
    this->subtitleText.size = unit;
    this->subtitleText.spacing = 0.0f;
    this->subtitleText.x = -1.0f + (unit * 7);
    this->subtitleText.y = 1.0f - (unit * 3);

    this->startText.fontTexture = fontTexture;
    this->startText.entity_text = "PRESS ENTER";
    this->startText.size = unit*2;
    this->startText.spacing = 0.0f;
    this->startText.x = -1.0f + (unit * 6);
    this->startText.y = 0.0f;
}

SpriteData playerSprite = SpriteData(237.0f/1024.0f, 377.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f);
SpriteData bulletSprite = SpriteData(143.0f/1024.0f, 377.0f/1024.0f, 43.0f/1024.0f, 31.0f/1024.0f);
SpriteData enemySprite1 = SpriteData(423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f);
SpriteData enemySprite2 = SpriteData(425.0f/1024.0f, 468.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f);
SpriteData enemySprite3 = SpriteData(144.0f/1024.0f, 156.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f);
SpriteData enemySprite4 = SpriteData(224.0f/1024.0f, 496.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f);
SpriteData enemySprite5 = SpriteData(346.0f/1024.0f, 150.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f);
SpriteData enemySprite6 = SpriteData(423.0f/1024.0f, 644.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f);
SpriteData borderSprite = SpriteData(0.0f, 0.0f, 256.0f, 256.0f);

void GameState::Setup() {
    // Setup game state entities
    // Player
    this->player.x = -1 * (unit/2);
    this->player.y = -1.777f + (8*unit);
    this->player.width = unit;
    this->player.height = unit;
    this->player.sprite = SheetSprite(spriteTexture, playerSprite, unit);

    // Bullets
    for(int i = 0; i < MAX_BULLETS; i++) {
        this->bullets[i].sprite = SheetSprite(spriteTexture, bulletSprite, unit/4);
        this->bullets[i].x = 1000;
        this->bullets[i].width = unit/4;
        this->bullets[i].height = unit/4;
        this->bullets[i].direction_y = 1.0f;
    }

    // Enemies
    enemiesLeft = 36;
    
    float stride_x = unit * 3;
    float stride_y = unit * 3;
    float start_x = -1 * unit * 8;
    float start_y = unit * 2;

    for(int i = 0; i < 6; i++) {
        for(int j = 0; j < 6; j++) {
            switch(i) {
                case 0:
                    this->enemies[(i*6)+j].sprite = SheetSprite(spriteTexture, enemySprite1, unit);
                    break;
                case 1:
                    this->enemies[(i*6)+j].sprite = SheetSprite(spriteTexture, enemySprite2, unit);
                    break;
                case 2:
                    this->enemies[(i*6)+j].sprite = SheetSprite(spriteTexture, enemySprite3, unit);
                    break;
                case 3:
                    this->enemies[(i*6)+j].sprite = SheetSprite(spriteTexture, enemySprite4, unit);
                    break;
                case 4:
                    this->enemies[(i*6)+j].sprite = SheetSprite(spriteTexture, enemySprite5, unit);
                    break;
                case 5:
                    this->enemies[(i*6)+j].sprite = SheetSprite(spriteTexture, enemySprite6, unit);
                    break;
            }
            this->enemies[(i*6)+j].x = start_x + (stride_x * j);
            this->enemies[(i*6)+j].y = start_y + (stride_y * i);
            this->enemies[(i*6)+j].width = unit;
            this->enemies[(i*6)+j].height = unit;
            this->enemies[(i*6)+j].direction_x = 1.0f;
        }
    }
}

void OverState::Setup() {
    // Setup game over state entities
    this->overText.fontTexture = fontTexture;
    this->overText.entity_text = "LEVEL OVER";
    this->overText.size = 0.125f;
    this->overText.spacing = 0.0f;
    this->overText.x = -1.0f + (unit * 7);
    this->overText.y = 1.0f;

    std::string score_text = "Final score: " + std::to_string(gameState.final_score);

    this->scoreText.fontTexture = fontTexture;
    this->scoreText.entity_text = score_text;
    this->scoreText.size = unit;
    this->scoreText.spacing = 0.0f;
    this->scoreText.x = -1.0f + (unit * 9);
    this->scoreText.y = 1.0f - (unit * 3);

    this->restartText.fontTexture = fontTexture;

    std::string restart_text = "PRESS ENTER TO ";
    if (gameState.score == 0) {
        restart_text += "RESTART";
    } else {
        restart_text += "CONTINUE";
    }

    this->restartText.entity_text = restart_text;
    this->restartText.size = unit;
    this->restartText.spacing = 0.0f;
    this->restartText.x = -1.0f + (unit * 5);
    this->restartText.y = 0.0f;
}

void MenuState::Render() {
    // Draw menu state entities
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(titleText.x, titleText.y, 1.0f));
    program.SetModelMatrix(modelMatrix);
    titleText.DrawText(program);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(subtitleText.x, subtitleText.y, 1.0f));
    program.SetModelMatrix(modelMatrix);
    subtitleText.DrawText(program);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(startText.x, startText.y, 1.0f));
    program.SetModelMatrix(modelMatrix);
    startText.DrawText(program);
}

void GameState::Render() {
    // Draw game state entities
    // Player
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(player.x, player.y, player.z));
    program.SetModelMatrix(modelMatrix);
    player.sprite.Draw(program);

    // Enemies
    for(int i = 0; i < 36; i++) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(enemies[i].x, enemies[i].y, enemies[i].z));
        program.SetModelMatrix(modelMatrix);
        enemies[i].sprite.Draw(program);
    }

    float angle = 90 * (PI / 180.0f);

    // Bullets
    for(int i = 0; i < MAX_BULLETS; i++) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(bullets[i].x, bullets[i].y, bullets[i].z));
        modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        program.SetModelMatrix(modelMatrix);
        bullets[i].sprite.Draw(program);
    }
}

void OverState::Render() {
    // Draw game over state entities
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(overText.x, overText.y, 1.0f));
    program.SetModelMatrix(modelMatrix);
    overText.DrawText(program);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(scoreText.x, scoreText.y, 1.0f));
    program.SetModelMatrix(modelMatrix);
    scoreText.DrawText(program);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(restartText.x, restartText.y, 1.0f));
    program.SetModelMatrix(modelMatrix);
    restartText.DrawText(program);
}

bool CheckEntityCollision(const Entity left, const Entity right) {
    // Separate into two functions for x and y
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

bool CheckLeftCollide(const Entity right) {
    if ((right.x - (right.width/2)) < -1.0f) {
        return true;
    }

    return false;
}

bool CheckRightCollide(const Entity left) {
    if ((left.x + (left.width/2)) > 1.0f) {
        return true;
    }

    return false;
}

bool CheckBottomCollide(const Entity top) {
    if ((top.y - (top.height/2)) < (-1.777f + 9*unit)) {
        return true;
    }

    return false;
}

bool CheckEnemyCollision(const GameState *state, bool (*f)(const Entity)) {
    bool check = false;
    for(int i = 0; i < 36; i++) {
        if ((*f)(state->enemies[i])) {
            check = true;
            break;
        }
    }

    return check;
}

void CheckBulletCollision(GameState *state) {
    for(int i = 0; i < MAX_BULLETS; i++) {
        for(int j = 0; j < 36; j++) {
            if (CheckEntityCollision(state->bullets[i], state->enemies[j])) {
                state->score++;
                state->enemiesLeft--;

                state->bullets[i].x = 1000;
                state->enemies[j].y = 2000;
            }
        }
    }
}

void GameState::Update(float elapsed) {
    this->player.Move(elapsed);

    if (CheckLeftCollide(player) || CheckRightCollide(player)) {
        this->player.Move(-1.0f * elapsed);
    }

    for(int i = 0; i < 36; i++) {
        this->enemies[i].Move(elapsed);
    }

    if (CheckEnemyCollision(this, CheckLeftCollide)) {
        for(int i = 0; i < 36; i++) {
            this->enemies[i].direction_x = 1.0f;
            this->enemies[i].y = enemies[i].y - unit;
        }
    } else if (CheckEnemyCollision(this, CheckRightCollide)) {
        for(int i = 0; i < 36; i++) {
            this->enemies[i].direction_x = -1.0f;
            this->enemies[i].y = enemies[i].y - unit;
        }
    } else if (CheckEnemyCollision(this, CheckBottomCollide)) {
        final_score = score;
        score = 0;
        overState.Setup();
        gameMode = GAME_OVER;
    }

    CheckBulletCollision(this);

    for(int i = 0; i < MAX_BULLETS; i++) {
        this->bullets[i].Move(elapsed);
    }

    if(enemiesLeft == 0) {
        final_score = score;
        overState.Setup();
        gameMode = GAME_OVER;
    }
}

void MenuState::ProcessEvents() {
    // Process events for menu
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        } else if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                done = true;
            } else if(event.key.keysym.scancode = SDL_SCANCODE_KP_ENTER) {
                gameState.Setup();
                gameMode = GAME_LEVEL;
            }
        }
    }
}

void Fire(GameState *state) {
    state->bullets[state->bulletIndex].x = state->player.x;
    state->bullets[state->bulletIndex].y = state->player.y;
    state->bullets[state->bulletIndex].velocity_y = 2.0f;

    state->bulletIndex++;

    if(state->bulletIndex > (MAX_BULLETS - 1)) {
        state->bulletIndex = 0;
    }
}

void GameState::ProcessEvents() {
    if(keys[SDL_SCANCODE_LEFT]) {
        this->player.direction_x = -1.0f;
    } else if (keys[SDL_SCANCODE_RIGHT]) {
        this->player.direction_x = 1.0f;
    } else {
        this->player.direction_x = 0.0f;
    }

    // Process events for game
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        } else if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                gameMode = MAIN_MENU;
            } else if(event.key.keysym.scancode == SDL_SCANCODE_UP) {
                Fire(this);
            } else if(event.key.keysym.scancode = SDL_SCANCODE_KP_ENTER) {
                // overState.Setup();
                // gameMode = GAME_OVER;
            }
        }
    }
}

void OverState::ProcessEvents() {
    // Process events for game over screen
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        } else if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                gameMode = MAIN_MENU;
            } else if(event.key.keysym.scancode = SDL_SCANCODE_KP_ENTER) {
                gameState.Setup();
                gameMode = GAME_LEVEL;
            }
        }
    }
}

// Game Data
MenuState menuState;

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 1080, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

    #ifdef _WINDOWS
    glewInit();
    #endif

    glViewport(0,0,720,1080);

    program.Load("vertex_textured.glsl", "fragment_textured.glsl");

    projectionMatrix = glm::mat4(1.0f);
    viewMatrix = glm::mat4(1.0f);

    projectionMatrix = glm::ortho(-1.0f, 1.0f, -1.777f, 1.777f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    keys = SDL_GetKeyboardState(NULL);

    fontTexture = LoadTexture("../assets/pixel_font.png");
    spriteTexture = LoadTexture("../assets/space_sheet.png");

    menuState.Setup();
    overState.Setup();
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    switch(gameMode) {
        case MAIN_MENU:
            menuState.Render();
            break;
        case GAME_LEVEL:
            gameState.Render();
            break;
        case GAME_OVER:
            overState.Render();
            break;
    }

    SDL_GL_SwapWindow(displayWindow);
}

void Update(float elapsed) {
    switch(gameMode) {
        case GAME_LEVEL:
            gameState.Update(elapsed);
            break;
    }
}

void ProcessEvents() {
    switch(gameMode) {
        case MAIN_MENU:
            menuState.ProcessEvents();
            break;
        case GAME_LEVEL:
            gameState.ProcessEvents();
            break;
        case GAME_OVER:
            overState.ProcessEvents();
            break;
    }
}

void Cleanup() {
    // Does nothing currently
}

float lastTicks = 0.0f;

int main(int argc, char *argv[])
{
    Setup();

    float acc = 0.0f;

    while (!done) {
        ProcessEvents();

        float ticks = (float)SDL_GetTicks()/1500.0f;
        float elapsed = ticks - lastTicks;
        lastTicks = ticks;

        elapsed += acc;
        if(elapsed < FIXED_TIMESTEP) {
            acc = elapsed;
            continue;
        }

        int count = 0;

        while(elapsed >= FIXED_TIMESTEP && (count != MAX_TIMESTEPS)) {
            Update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
            count++;
        }

        acc = elapsed;

        Render();
    }

    Cleanup();
    SDL_Quit();
    return 0;
}