#ifdef _WINDOWS
// #include <GL/glew.h>
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

#define MAX_BULLETS 16
#define MAX_ENEMIES 12

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

#define LEVEL_HEIGHT 12
#define LEVEL_WIDTH 40

#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
#define TILE_SIZE 1.0f/6.0f

#define WORLD_SHIFT_X -1.8f
#define WORLD_SHIFT_Y 1.0f

#define NUM_SOLIDS 1

#define TINY_VALUE 1.0f/4096.0f

// Generic print function for debugging found at: https://stackoverflow.com/a/22483243
template <typename T>
void print_container(const T&);

template <typename T>
void print(const std::vector<T>& v) { print_container(v); }

template <typename T>
void print(const T& e) { std::cout << e << std::endl; }

template <typename T>
void print_container(const T& c) {
    std::cout << "[";
    bool isFirst = true;
    for (const auto& e : c) {
        if (isFirst) isFirst = false;
        else std::cout << ",";
        print(e);
    }
    std::cout << "]" << std::endl;
}

// Global Data

unsigned int levelOne[LEVEL_HEIGHT][LEVEL_WIDTH] =
{
    {2,2,2,2,0, 0,0,0,0,0, 0,0,0,0,0, 0,2,2,2,2, 2,2,2,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,2,2,2,2},
    {2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2},
    {2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2},
    {2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2},

    {0,0,0,0,0, 0,0,0,0,0, 2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0},
    {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0},
    {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0},
    {0,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0},

    {2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2},
    {2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2},
    {2,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,2},
    {2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2, 2,2,2,2,2}
};

int solids[NUM_SOLIDS] = {2};

const float PI = 3.14159265358979323846f;
const float unit = 0.0625f;

GLuint fontTexture, arneSprites, spaceSprite;

SDL_Window* displayWindow;

ShaderProgram program;

glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;

SDL_Event event;

bool done = false;

const Uint8 *keys;

enum EntityType { PLAYER, ENEMY };

// End Global Data

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

void DrawSpriteSheetSprite(ShaderProgram &p, int index) {
    
    float u = (float)(((int)index) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
    float v = (float)(((int)index) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;

    float spriteWidth = 1.0/(float)SPRITE_COUNT_X;
    float spriteHeight = 1.0/(float)SPRITE_COUNT_Y;

    float texCoords[] = {
        u, v + spriteHeight,
        u + spriteWidth, v,
        u, v,

        u + spriteWidth, v,
        u, v + spriteHeight,
        u + spriteWidth, v + spriteHeight
    };

    float vertices[] = {
        -0.5f, -0.5f,
        0.5f, 0.5f,
        -0.5f, 0.5f,

        0.5f, 0.5f,
        -0.5f, -0.5f,
        0.5f, -0.5f
    };

    glBindTexture(GL_TEXTURE_2D, arneSprites);

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
        float rotation = 0.0f;

        float width;
        float height;

        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

        int spriteIndex;

        bool collidedTop = false;
        bool collidedBotoom = false;
        bool collidedLeft = false;
        bool collidedRight = false;

        void MoveX(float elapsed);
        void MoveY(float elapsed);
};

class Player {
    public:

        int headIndex;
        Entity body;
        int legIndex;

        void Draw();
};

void Player::Draw() {
    // Draw body
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(body.position.x, body.position.y, body.position.z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(TILE_SIZE,TILE_SIZE,0.0f));
    program.SetModelMatrix(modelMatrix);
    DrawSpriteSheetSprite(program, body.spriteIndex);

    // Draw head
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(body.position.x, body.position.y+TILE_SIZE, body.position.z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(TILE_SIZE,TILE_SIZE,0.0f));
    program.SetModelMatrix(modelMatrix);
    DrawSpriteSheetSprite(program, headIndex);

    // Draw legs
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(body.position.x, body.position.y-(0.5f * TILE_SIZE), body.position.z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(TILE_SIZE,TILE_SIZE,0.0f));
    program.SetModelMatrix(modelMatrix);
    DrawSpriteSheetSprite(program, legIndex);
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

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

float pen(float p0, float p1, float measurement1, float measurement2) {
    float distance = abs(p0 - p1);
    return fabs(distance - (measurement1 * 0.5f) - (measurement2 * 0.5f));
}

void Entity::MoveX(float elapsed) {
    this->velocity.x = lerp(velocity.x, 0.0f, elapsed * 2.0f);
    if (abs(velocity.x) < 0.016f) {
        this->velocity.x = 0.0f;
    }
    this->velocity.x += acceleration.x * elapsed;
    this->position.x += velocity.x * elapsed;
}

void Entity::MoveY(float elapsed) {
    this->velocity.y = lerp(velocity.y, 0.0f, elapsed * 2.0f);
    if (abs(velocity.y) < 0.016f) {
        this->velocity.y = 0.0f;
    }
    this->velocity.y += acceleration.y * elapsed;
    this->position.y += velocity.y * elapsed;
}

void DrawLevel(ShaderProgram &p) {
    std::vector<float> vertexData;
    std::vector<float> texCoordData;

    for(int y = 0; y < LEVEL_HEIGHT; y++) {
        for(int x = 0; x < LEVEL_WIDTH; x++) {
            if(levelOne[y][x] != 0) {
                float u = (float)(((int)levelOne[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int)levelOne[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;

                float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
                float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;

                vertexData.insert(vertexData.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
                    (TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,

                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
                    (TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
                });

                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v + (spriteHeight),
                    u + spriteWidth, v + (spriteHeight),
                    
                    u, v,
                    u + spriteWidth, v + (spriteHeight),
                    u + spriteWidth, v
                });
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, arneSprites);

    glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(p.positionAttribute);

    glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(p.texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);

    glDisableVertexAttribArray(p.positionAttribute);
    glDisableVertexAttribArray(p.texCoordAttribute);
}

class GameState {
    public:
        Player player;
        Entity enemies[12];
        Entity bullets[16];

        int bulletIndex = 0;

        void Setup();
        void Render();
        void Update(float elapsed);
        void ProcessEvents();
};

GameState gameState;

// SpriteData playerSprite = SpriteData(237.0f/1024.0f, 377.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f);
// SpriteData bulletSprite = SpriteData(143.0f/1024.0f, 377.0f/1024.0f, 43.0f/1024.0f, 31.0f/1024.0f);
// SpriteData enemySprite1 = SpriteData(423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f);
// SpriteData enemySprite2 = SpriteData(425.0f/1024.0f, 468.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f);
// SpriteData enemySprite3 = SpriteData(144.0f/1024.0f, 156.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f);
// SpriteData enemySprite4 = SpriteData(224.0f/1024.0f, 496.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f);
// SpriteData enemySprite5 = SpriteData(346.0f/1024.0f, 150.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f);
// SpriteData enemySprite6 = SpriteData(423.0f/1024.0f, 644.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f);
// SpriteData borderSprite = SpriteData(0.0f, 0.0f, 256.0f, 256.0f);

void GameState::Setup() {
    // Setup game state entities
    // Player
    this->player.body.position.x = 0.0f;
    this->player.body.position.y = 0.0f - (TILE_SIZE * 1);
    this->player.body.width = TILE_SIZE;
    this->player.body.height = 2.0f * TILE_SIZE;
    this->player.body.spriteIndex = 98;

    this->player.headIndex = 82;
    this->player.legIndex = 66;
    // this->player.sprite = SheetSprite(spaceSprite, playerSprite, unit);

    // // Bullets
    // for(int i = 0; i < MAX_BULLETS; i++) {
    //     this->bullets[i].sprite = SheetSprite(arneSprites, bulletSprite, unit/4);
    //     this->bullets[i].x = 1000;
    //     this->bullets[i].width = unit/4;
    //     this->bullets[i].height = unit/4;
    //     this->bullets[i].direction_y = 1.0f;
    // }

    // Enemies
    // for(int i = 0; i < 6; i++) {
    //     for(int j = 0; j < 6; j++) {
    //         switch(i) {
    //             case 0:
    //                 this->enemies[(i*6)+j].sprite = SheetSprite(arneSprites, enemySprite1, unit);
    //                 break;
    //             case 1:
    //                 this->enemies[(i*6)+j].sprite = SheetSprite(arneSprites, enemySprite2, unit);
    //                 break;
    //             case 2:
    //                 this->enemies[(i*6)+j].sprite = SheetSprite(arneSprites, enemySprite3, unit);
    //                 break;
    //             case 3:
    //                 this->enemies[(i*6)+j].sprite = SheetSprite(arneSprites, enemySprite4, unit);
    //                 break;
    //             case 4:
    //                 this->enemies[(i*6)+j].sprite = SheetSprite(arneSprites, enemySprite5, unit);
    //                 break;
    //             case 5:
    //                 this->enemies[(i*6)+j].sprite = SheetSprite(arneSprites, enemySprite6, unit);
    //                 break;
    //         }
    //         this->enemies[(i*6)+j].x = start_x + (stride_x * j);
    //         this->enemies[(i*6)+j].y = start_y + (stride_y * i);
    //         this->enemies[(i*6)+j].width = unit;
    //         this->enemies[(i*6)+j].height = unit;
    //         this->enemies[(i*6)+j].direction_x = 1.0f;
    //     }
    // }
}

void GameState::Render() {
    // Set scroll on player x movement (and z movement even though it doesn't matter)
    viewMatrix = glm::mat4(1.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.body.position.x, 0.0f,-player.body.position.z));
    program.SetViewMatrix(viewMatrix);

    // Draw game state entities
    // Player
    player.Draw();
    // glm::mat4 modelMatrix = glm::mat4(1.0f);
    // modelMatrix = glm::translate(modelMatrix, glm::vec3(player.x, player.y, player.z));
    // modelMatrix = glm::scale(modelMatrix, glm::vec3(TILE_SIZE,TILE_SIZE,0.0f));
    // program.SetModelMatrix(modelMatrix);
    // DrawSpriteSheetSprite(program, player.spriteIndex);

    // // Enemies
    // for(int i = 0; i < 36; i++) {
    //     glm::mat4 modelMatrix = glm::mat4(1.0f);
    //     modelMatrix = glm::translate(modelMatrix, glm::vec3(enemies[i].x, enemies[i].y, enemies[i].z));
    //     program.SetModelMatrix(modelMatrix);
    //     enemies[i].sprite.Draw(program);
    // }

    // float angle = 90 * (PI / 180.0f);

    // // Bullets
    // for(int i = 0; i < MAX_BULLETS; i++) {
    //     glm::mat4 modelMatrix = glm::mat4(1.0f);
    //     modelMatrix = glm::translate(modelMatrix, glm::vec3(bullets[i].x, bullets[i].y, bullets[i].z));
    //     modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
    //     program.SetModelMatrix(modelMatrix);
    //     bullets[i].sprite.Draw(program);
    // }
    
    // Draw the game level
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(WORLD_SHIFT_X, WORLD_SHIFT_Y, 0.0f));
    program.SetModelMatrix(modelMatrix);
    DrawLevel(program);
}

void getTileCoords(float worldX, float worldY, int &gridX, int &gridY) {
    // print("Grid tile X and Y:");
    gridX = (int)worldX;
    gridY = (int)worldY;
    // print(gridX);
    // print(gridY);
}

void getAdjustedWorldCoords(float x, float y, float &worldX, float &worldY) {
    // print("Tile Size:");
    // print(TILE_SIZE);
    // print("World X and Y:");
    // print(x);
    // print(y);
    worldX = (x - (WORLD_SHIFT_X)) / (TILE_SIZE);
    worldY = (y - (WORLD_SHIFT_Y)) / (-TILE_SIZE);
    // print("Float tile X and Y:");
    // print(worldX);
    // print(worldY);
}

bool checkSolid(const int x, const int y) {
    int blockType = levelOne[y][x];
    // print("Block Type:");
    // print(blockType);

    for (int i = 0; i < NUM_SOLIDS; i++) {
        if(blockType == solids[i]) {
            return true;
        }
    }

    return false;
}

// bool CheckEntityCollision(const Entity left, const Entity right) {
//     // Separate into two functions for x and y
//     float left_x = left.x + (left.width / 2);
//     float left_y = left.y - (left.height / 2);

//     float right_x = right.x + (right.width / 2);
//     float right_y = right.y - (right.height / 2);

//     float p_x = glm::abs(left_x - right_x) - ((left.width + right.width) / 2);
//     float p_y = glm::abs(left_y - right_y) - ((left.height + right.height) / 2);

//     if ((p_x < 0.0f) && (p_y < 0.0f)) {
//         return true;
//     }

//     return false;
// }

// bool CheckLeftWorldCollide(const Entity right) {
    
// }

// bool CheckRightWorldCollide(const Entity left) {
    
// }

bool CheckTopWorldCollide(const Entity top) {
    int gridX, gridY;
    float worldX, worldY;

    getAdjustedWorldCoords(top.position.x, top.position.y + (0.5f * top.height), worldX, worldY);
    getTileCoords(worldX, worldY, gridX, gridY);

    // pen_y = gridY - worldY;
    // print("Pen:");
    // print(pen_y);

    return checkSolid(gridX, gridY);
}

bool CheckBottomWorldCollide(const Entity top) {
    int gridX, gridY;
    float worldX, worldY;
    
    getAdjustedWorldCoords(top.position.x, top.position.y - (0.5f * top.height), worldX, worldY);
    getTileCoords(worldX, worldY, gridX, gridY);

    // pen_y = gridY - worldY;
    // print("Pen:");
    // print(pen_y);

    return checkSolid(gridX, gridY);
}

// bool CheckEnemyCollision(const GameState *state, bool (*f)(const Entity)) {
//     bool check = false;
//     for(int i = 0; i < 36; i++) {
//         if ((*f)(state->enemies[i])) {
//             check = true;
//             break;
//         }
//     }

//     return check;
// }

// void CheckBulletCollision(GameState *state) {
//     for(int i = 0; i < MAX_BULLETS; i++) {
//         for(int j = 0; j < 36; j++) {
//             if (CheckEntityCollision(state->bullets[i], state->enemies[j])) {

//                 state->bullets[i].x = 1000;
//                 state->enemies[j].y = 2000;
//             }
//         }
//     }
// }

void GameState::Update(float elapsed) {
    // if (!(CheckLeftCollide(player) || CheckRightCollide(player))) {
    // print("test");
    print("Velocity:");
    print(player.body.velocity.y);
    print("Accleration:");
    print(player.body.acceleration.y);

    float pen_x, pen_y;

    // if (!(CheckBottomWorldCollide(player.body, pen_y))) {
    //     this->player.body.MoveY(elapsed);
    // } else {
    //     this->player.body.acceleration.y = 0.0f;
    //     this->player.body.velocity.y = 0.0f;
    //     this->player.body.position.y -= pen_y + (TINY_VALUE);
    // }

    // if (CheckTopWorldCollide(player.body)) {
    //     this->player.body.MoveY(elapsed);
    // }
    this->player.body.MoveY(elapsed);
    if (CheckBottomWorldCollide(player.body) || CheckTopWorldCollide(player.body)) {
        float inverse = -1.0f;
        float inverse_elapsed = inverse * elapsed;
        this->player.body.MoveY(inverse);
        // this->player.body.acceleration.y = 0.0f;
        // this->player.body.velocity.y = 0.0f;
    }

    this->player.body.MoveX(elapsed);
    // }

    // for(int i = 0; i < 36; i++) {
    //     this->enemies[i].Move(elapsed);
    // }

    // if (CheckEnemyCollision(this, CheckLeftCollide)) {
    //     for(int i = 0; i < 36; i++) {
    //         this->enemies[i].direction_x = 1.0f;
    //         this->enemies[i].y = enemies[i].y - unit;
    //     }
    // } else if (CheckEnemyCollision(this, CheckRightCollide)) {
    //     for(int i = 0; i < 36; i++) {
    //         this->enemies[i].direction_x = -1.0f;
    //         this->enemies[i].y = enemies[i].y - unit;
    //     }
    // } else if (CheckEnemyCollision(this, CheckBottomCollide)) {
    //     final_score = score;
    //     score = 0;
    //     overState.Setup();
    //     gameMode = GAME_OVER;
    // }

    // CheckBulletCollision(this);

    // for(int i = 0; i < MAX_BULLETS; i++) {
    //     this->bullets[i].Move(elapsed);
    // }

}

// void Fire(GameState *state) {
//     state->bullets[state->bulletIndex].x = state->player.body.x;
//     state->bullets[state->bulletIndex].y = state->player.body.y;
//     state->bullets[state->bulletIndex].velocity_y = 2.0f;

//     state->bulletIndex++;

//     if(state->bulletIndex > (MAX_BULLETS - 1)) {
//         state->bulletIndex = 0;
//     }
// }

void GameState::ProcessEvents() {
    if(keys[SDL_SCANCODE_LEFT]) {
        this->player.body.acceleration.x = -5.0f;
    } else if (keys[SDL_SCANCODE_RIGHT]) {
        this->player.body.acceleration.x = 5.0f;
    } else {
        this->player.body.acceleration.x = 0.0f;
    }

    if(keys[SDL_SCANCODE_UP]) {
        this->player.body.acceleration.y = 1.0f;
    } else if (keys[SDL_SCANCODE_DOWN]) {
        this->player.body.acceleration.y = -1.0f;
    } else {
        this->player.body.acceleration.y = 0.0f;
    }

    // Process events for game
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        } else if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                done = true;
            } else if(event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                this->player.body.velocity.y = 1.0f;
            }
        }
    }
}

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Platformer HW4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

    #ifdef _WINDOWS
    glewInit();
    #endif

    glViewport(0,0,1280,720);

    program.Load("vertex_textured.glsl", "fragment_textured.glsl");

    projectionMatrix = glm::mat4(1.0f);
    viewMatrix = glm::mat4(1.0f);

    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.529f, 0.808f, 0.922f, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    keys = SDL_GetKeyboardState(NULL);

    fontTexture = LoadTexture("../assets/pixel_font.png");
    arneSprites = LoadTexture("../assets/arne_sprites.png");
    spaceSprite = LoadTexture("../assets/space_sheet.png");

    gameState.Setup();
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    gameState.Render();

    SDL_GL_SwapWindow(displayWindow);
}

void Update(float elapsed) {
    gameState.Update(elapsed);
}

void ProcessEvents() {
    gameState.ProcessEvents();
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

    SDL_Quit();
    return 0;
}