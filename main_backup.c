#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
} GameState;

typedef struct {
    Vector2 pos;
    Vector2 velocity;
    float speed;
} Player;

typedef struct {
    Rectangle rect;
    bool active;
} Wall;

typedef struct {
    Rectangle rect;
    bool active;
    float animY;
} Coin;

typedef struct {
    Vector2 pos;
    Rectangle rect;
    bool active;
    Vector2 velocity;
} Dart;

typedef struct {
    Rectangle rect;
    bool active;
} Bow;

typedef struct {
    Vector2 pos;
    bool active;
} DropItem;

typedef struct {
    Rectangle rect;
    const char *text;
    bool selected;
} Button;

typedef enum {
    TILE_WATER,
    TILE_SAND,
    TILE_GRASS,
    TILE_EARTH
} TileType;

#define MAP_WIDTH 100
#define MAP_HEIGHT 100
#define TILE_SIZE 32
#define MAX_DARTS 30

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Jogo Ilha");
    SetTargetFPS(60);
    srand(time(NULL));

    GameState state = STATE_MENU;

    Camera2D camera = {0};
    camera.target = (Vector2){0, 0};
    camera.offset = (Vector2){screenWidth/2, screenHeight/2};
    camera.rotation = 0;
    camera.zoom = 1.5f;

    TileType map[MAP_WIDTH][MAP_HEIGHT];
    
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            float dx = x - MAP_WIDTH/2.0f;
            float dy = y - MAP_HEIGHT/2.0f;
            float dist = sqrt(dx*dx + dy*dy);
            float islandSize = 35;
            
            if (dist < islandSize - 5) {
                if (dist < islandSize - 15) {
                    float r = (float)rand() / RAND_MAX;
                    if (r < 0.7f) map[x][y] = TILE_GRASS;
                    else map[x][y] = TILE_EARTH;
                } else {
                    map[x][y] = TILE_SAND;
                }
            } else {
                map[x][y] = TILE_WATER;
            }
        }
    }

    Player player = {
        .pos = {MAP_WIDTH/2.0f * TILE_SIZE, MAP_HEIGHT/2.0f * TILE_SIZE},
        .velocity = {0, 0},
        .speed = 180
    };

    Vector2 aimDir = {0, -1};

    int coinCount = 0;
    int coinScore = 0;

    Bow bow = {
        .rect = {0, 0, 24, 24},
        .active = false
    };

    Dart darts[MAX_DARTS];
    for (int i = 0; i < MAX_DARTS; i++) darts[i].active = false;

    DropItem drops[30];
    for (int i = 0; i < 30; i++) drops[i].active = false;
    int dropCount = 0;

    int highScore = 0;
    bool gameOver = false;
    float gameTime = 0;
    float shootCooldown = 0;
    int ammo = 0;

    Button buttons[2] = {
        {{screenWidth/2 - 60, 250, 120, 50}, "PLAY", true},
        {{screenWidth/2 - 60, 320, 120, 50}, "EXIT", false}
    };

    int selectedButton = 0;

    float prevTileColor[MAP_WIDTH][MAP_HEIGHT][3] = {0};

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            prevTileColor[x][y][0] = 60;
            prevTileColor[x][y][1] = 130;
            prevTileColor[x][y][2] = 60;
        }
    }

    while (!WindowShouldClose()) {
        if (state == STATE_MENU) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) selectedButton = (selectedButton + 1) % 2;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selectedButton = (selectedButton + 1) % 2;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                if (selectedButton == 0) {
                    state = STATE_PLAYING;
                    player.pos.x = MAP_WIDTH/2.0f * TILE_SIZE;
                    player.pos.y = MAP_HEIGHT/2.0f * TILE_SIZE;
                    aimDir = (Vector2){0, -1};
                    coinScore = ammo = 0;
                    bow.active = false;
                    gameOver = false;
                    gameTime = 0;
                    camera.target.x = player.pos.x;
                    camera.target.y = player.pos.y;
                    
                    for (int i = 0; i < MAX_DARTS; i++) darts[i].active = false;
                    for (int i = 0; i < 30; i++) drops[i].active = false;
                    dropCount = 0;
                } else {
                    CloseWindow();
                    return 0;
                }
            }
            for (int i = 0; i < 2; i++) buttons[i].selected = (i == selectedButton);
        }
        else if (state == STATE_PLAYING) {
            float dt = GetFrameTime();
            gameTime += dt;

            if (shootCooldown > 0) shootCooldown -= dt;

            float vx = 0, vy = 0;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) vy -= 1;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) vy += 1;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) vx -= 1;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) vx += 1;

            if (vx != 0 || vy != 0) {
                float len = sqrt(vx*vx + vy*vy);
                vx /= len;
                vy /= len;
                aimDir.x = vx;
                aimDir.y = vy;
            }

            if (IsKeyPressed(KEY_SPACE) && bow.active && ammo > 0 && shootCooldown <= 0) {
                for (int i = 0; i < MAX_DARTS; i++) {
                    if (!darts[i].active) {
                        darts[i].active = true;
                        darts[i].pos.x = player.pos.x;
                        darts[i].pos.y = player.pos.y;
                        darts[i].rect.x = player.pos.x;
                        darts[i].rect.y = player.pos.y;
                        darts[i].rect.width = 12;
                        darts[i].rect.height = 4;
                        darts[i].velocity.x = aimDir.x * 400;
                        darts[i].velocity.y = aimDir.y * 400;
                        ammo--;
                        shootCooldown = 0.3f;
                        break;
                    }
                }
            }

            Vector2 newPos;
            newPos.x = player.pos.x + vx * player.speed * dt;
            newPos.y = player.pos.y + vy * player.speed * dt;

            int tileX = (int)(newPos.x / TILE_SIZE);
            int tileY = (int)(newPos.y / TILE_SIZE);

            bool canMove = true;
            if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT || map[tileX][tileY] == TILE_WATER) {
                canMove = false;
            }

            if (canMove) {
                player.pos.x = newPos.x;
                player.pos.y = newPos.y;
            }

            for (int i = 0; i < dropCount; i++) {
                if (drops[i].active) {
                    float dx = player.pos.x - drops[i].pos.x;
                    float dy = player.pos.y - drops[i].pos.y;
                    if (sqrt(dx*dx + dy*dy) < 25) {
                        drops[i].active = false;
                        ammo += 3;
                    }
                }
            }

            camera.target.x = player.pos.x;
            camera.target.y = player.pos.y;
        }

        BeginDrawing();
        ClearBackground((Color){20, 60, 100, 255});

        if (state == STATE_MENU) {
            DrawText("ILHA PERDIDA", screenWidth/2 - MeasureText("ILHA PERDIDA", 40)/2, 130, 40, WHITE);

            for (int i = 0; i < 2; i++) {
                Color bgColor = buttons[i].selected ? (Color){240, 240, 240, 255} : (Color){80, 80, 80, 255};
                Color textColor = buttons[i].selected ? (Color){20, 20, 20, 255} : (Color){200, 200, 200, 255};
                DrawRectangleRec(buttons[i].rect, bgColor);
                DrawRectangleLinesEx(buttons[i].rect, 2, textColor);
                DrawText(buttons[i].text, buttons[i].rect.x + buttons[i].rect.width/2 - MeasureText(buttons[i].text, 20)/2, buttons[i].rect.y + 15, 20, textColor);
            }

            DrawText("WASD to move | SPACE shoot", screenWidth/2 - 130, 400, 16, GRAY);
        }
        else if (state == STATE_PLAYING) {
            BeginMode2D(camera);

            int startX = (int)(camera.target.x / TILE_SIZE) - 15;
            int startY = (int)(camera.target.y / TILE_SIZE) - 12;
            int endX = startX + 30;
            int endY = startY + 25;

            for (int x = startX; x <= endX && x < MAP_WIDTH; x++) {
                for (int y = startY; y <= endY && y < MAP_HEIGHT; y++) {
                    if (x < 0 || y < 0) continue;
                    
                    int px = x * TILE_SIZE;
                    int py = y * TILE_SIZE;
                    TileType tile = map[x][y];
                    
                    float targetR, targetG, targetB;
                    
                    if (tile == TILE_WATER) {
                        float wave = sinf(GetTime() * 2.0f + x * 0.3f + y * 0.2f);
                        targetR = 30 + wave * 20;
                        targetG = 80 + wave * 30;
                        targetB = 140 + wave * 40;
                    } else if (tile == TILE_SAND) {
                        targetR = 210; targetG = 190; targetB = 140;
                    } else if (tile == TILE_GRASS) {
                        float wind = sinf(GetTime() * 3.0f + x * 0.5f + y * 0.3f);
                        targetR = 60 + wind * 8;
                        targetG = 130 + wind * 15;
                        targetB = 60 + wind * 8;
                    } else if (tile == TILE_EARTH) {
                        targetR = 120; targetG = 90; targetB = 60;
                    } else {
                        targetR = 60; targetG = 130; targetB = 60;
                    }
                    
                    float smooth = 0.15f;
                    prevTileColor[x][y][0] += (targetR - prevTileColor[x][y][0]) * smooth;
                    prevTileColor[x][y][1] += (targetG - prevTileColor[x][y][1]) * smooth;
                    prevTileColor[x][y][2] += (targetB - prevTileColor[x][y][2]) * smooth;
                    
DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, 
                        (Color){(int)prevTileColor[x][y][0], (int)prevTileColor[x][y][1], (int)prevTileColor[x][y][2], 255});
                    
                    if (tile == TILE_GRASS) {
                        float wind = sinf(GetTime() * 3.0f + x * 0.5f + y * 0.3f);
                        int sway = (int)(wind * 3);
                        
                        if ((x + y) % 2 == 0) {
                            DrawLineEx((Vector2){px + 10 + sway, py + 28}, (Vector2){px + 10 + sway * 2, py + 20}, 2, (Color){50, 120, 50, 200});
                            DrawLineEx((Vector2){px + 14 + sway, py + 28}, (Vector2){px + 14 + sway * 2, py + 18}, 2, (Color){70, 140, 70, 180});
                        }
                        if ((x + y) % 3 == 1) {
                            DrawLineEx((Vector2){px + 22 + sway, py + 28}, (Vector2){px + 22 + sway * 2, py + 22}, 2, (Color){55, 125, 55, 200});
                        }
                    }
                    
                }
            }
            }

            if (bow.active) {
                DrawRectangleRec(bow.rect, (Color){139, 90, 43, 255});
            }

            for (int i = 0; i < dropCount; i++) {
                if (drops[i].active) {
                    float bounce = sinf(GetTime() * 6 + i) * 2;
                    DrawCircle((int)drops[i].pos.x + 12, (int)(drops[i].pos.y + 12 + bounce), 8, (Color){255, 100, 100, 255});
                    DrawCircle((int)drops[i].pos.x + 12, (int)(drops[i].pos.y + 12 + bounce), 4, (Color){255, 200, 200, 255});
                }
            }

            for (int i = 0; i < MAX_DARTS; i++) {
                if (darts[i].active) {
                    DrawCircle((int)darts[i].rect.x, (int)darts[i].rect.y, 5, (Color){100, 100, 100, 255});
                }
            }

            DrawCircle((int)player.pos.x, (int)player.pos.y, 14, (Color){240, 240, 240, 255});
            DrawCircle((int)player.pos.x, (int)player.pos.y, 10, (Color){255, 255, 255, 255});

            if (bow.active) {
                DrawLineEx(player.pos, (Vector2){player.pos.x + aimDir.x * 25, player.pos.y + aimDir.y * 25}, 2, (Color){255, 255, 255, 150});
            }

            EndMode2D();

            if (bow.active) {
                DrawText(TextFormat("Flechas: %i", ammo), 20, 20, 20, (Color){255, 100, 100, 255});
            }
            
            DrawText(TextFormat("Tempo: %.1f", gameTime), 20, 50, 16, WHITE);
            DrawText("WASD move | SPACE shoot", screenWidth - 180, 20, 14, GRAY);
            DrawText("ESC for menu", screenWidth - 100, screenHeight - 30, 14, GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}