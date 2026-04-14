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
    int type;
    float health;
    float attackCooldown;
    float deathTimer;
} Enemy;

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
    TILE_EARTH,
    TILE_FOREST
} TileType;

typedef enum {
    ENEMY_NORMAL,
    ENEMY_RANGED,
    ENEMY_AGGRESSIVE
} EnemyType;

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
    
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (map[x][y] == TILE_GRASS && rand() % 100 < 15) {
                int nx = x + rand() % 3 - 1;
                int ny = y + rand() % 3 - 1;
                if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                    if (map[nx][ny] == TILE_GRASS) map[nx][ny] = TILE_FOREST;
                }
            }
        }
    }

    int wallCount = 0;
    Wall *walls = malloc(500 * sizeof(Wall));
    
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (map[x][y] == TILE_FOREST && wallCount < 500) {
                walls[wallCount].rect.x = x * TILE_SIZE;
                walls[wallCount].rect.y = y * TILE_SIZE;
                walls[wallCount].rect.width = TILE_SIZE;
                walls[wallCount].rect.height = TILE_SIZE;
                walls[wallCount].active = true;
                wallCount++;
            }
        }
    }

    Player player = {
        .pos = {MAP_WIDTH/2.0f * TILE_SIZE, MAP_HEIGHT/2.0f * TILE_SIZE},
        .velocity = {0, 0},
        .speed = 180
    };

    Vector2 aimDir = {0, -1};

    Coin coins[50];
    for (int i = 0; i < 50; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (map[x][y] == TILE_WATER || map[x][y] == TILE_FOREST || 
               (abs(x - MAP_WIDTH/2) < 3 && abs(y - MAP_HEIGHT/2) < 3));
        
        coins[i].rect.x = x * TILE_SIZE + TILE_SIZE/2;
        coins[i].rect.y = y * TILE_SIZE + TILE_SIZE/2;
        coins[i].rect.width = 16;
        coins[i].rect.height = 16;
        coins[i].active = true;
    }
    int coinCount = 50;

    Bow bow = {
        .rect = {0, 0, 24, 24},
        .active = false
    };

    Dart darts[MAX_DARTS];
    for (int i = 0; i < MAX_DARTS; i++) darts[i].active = false;

    DropItem drops[30];
    for (int i = 0; i < 30; i++) drops[i].active = false;
    int dropCount = 0;

    Enemy enemies[25];
    for (int i = 0; i < 25; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (map[x][y] == TILE_WATER || map[x][y] == TILE_FOREST ||
               (abs(x - MAP_WIDTH/2) < 10 && abs(y - MAP_HEIGHT/2) < 10));
        
        enemies[i].pos.x = x * TILE_SIZE;
        enemies[i].pos.y = y * TILE_SIZE;
        enemies[i].rect.x = enemies[i].pos.x;
        enemies[i].rect.y = enemies[i].pos.y;
        enemies[i].rect.width = 24;
        enemies[i].rect.height = 24;
        enemies[i].type = rand() % 3;
        enemies[i].health = 1;
        enemies[i].attackCooldown = 0;
        enemies[i].deathTimer = 0;
        enemies[i].active = true;
    }
    int enemyCount = 25;

    int coinScore = 0;
    int highScore = 0;
    int killCount = 0;
    bool gameOver = false;
    float gameTime = 0;
    float shootCooldown = 0;
    int ammo = 0;

    Button buttons[2] = {
        {{screenWidth/2 - 60, 250, 120, 50}, "PLAY", true},
        {{screenWidth/2 - 60, 320, 120, 50}, "EXIT", false}
    };

    int selectedButton = 0;

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
                    coinScore = killCount = ammo = 0;
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
            if (gameOver) {
                if (IsKeyPressed(KEY_SPACE)) {
                    player.pos.x = MAP_WIDTH/2.0f * TILE_SIZE;
                    player.pos.y = MAP_HEIGHT/2.0f * TILE_SIZE;
                    aimDir = (Vector2){0, -1};
                    coinScore = killCount = ammo = 0;
                    bow.active = false;
                    gameOver = false;
                    gameTime = 0;
                    camera.target.x = player.pos.x;
                    camera.target.y = player.pos.y;
                    
                    for (int i = 0; i < MAX_DARTS; i++) darts[i].active = false;
                    for (int i = 0; i < 30; i++) drops[i].active = false;
                    dropCount = 0;
                    
                    for (int i = 0; i < enemyCount; i++) {
                        int x, y;
                        do {
                            x = rand() % MAP_WIDTH;
                            y = rand() % MAP_HEIGHT;
                        } while (map[x][y] == TILE_WATER || map[x][y] == TILE_FOREST ||
                               (abs(x - MAP_WIDTH/2) < 10 && abs(y - MAP_HEIGHT/2) < 10));
                        
                        enemies[i].pos.x = x * TILE_SIZE;
                        enemies[i].pos.y = y * TILE_SIZE;
                        enemies[i].health = 1;
                        enemies[i].attackCooldown = 0;
                        enemies[i].deathTimer = 0;
                        enemies[i].active = true;
                    }
                }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    state = STATE_MENU;
                    gameOver = false;
                }
            } else {
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

                for (int i = 0; i < wallCount && canMove; i++) {
                    if (walls[i].active) {
                        Rectangle playerRect = {newPos.x - 12, newPos.y - 12, 24, 24};
                        if (CheckCollisionRecs(playerRect, walls[i].rect)) {
                            canMove = false;
                            break;
                        }
                    }
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

                Rectangle playerRect = {player.pos.x - 12, player.pos.y - 12, 24, 24};

                for (int i = 0; i < coinCount; i++) {
                    if (coins[i].active) {
                        float dx = player.pos.x - coins[i].rect.x;
                        float dy = player.pos.y - coins[i].rect.y;
                        if (sqrt(dx*dx + dy*dy) < 20) {
                            coins[i].active = false;
                            coinScore++;
                        }
                    }
                }

                for (int i = 0; i < MAX_DARTS; i++) {
                    if (darts[i].active) {
                        darts[i].pos.x += darts[i].velocity.x * dt;
                        darts[i].pos.y += darts[i].velocity.y * dt;
                        darts[i].rect.x = darts[i].pos.x;
                        darts[i].rect.y = darts[i].pos.y;
                        
                        int dx = (int)(darts[i].pos.x / TILE_SIZE);
                        int dy = (int)(darts[i].pos.y / TILE_SIZE);
                        if (dx < 0 || dx >= MAP_WIDTH || dy < 0 || dy >= MAP_HEIGHT || map[dx][dy] == TILE_WATER) {
                            darts[i].active = false;
                            continue;
                        }
                        
                        for (int j = 0; j < wallCount; j++) {
                            if (walls[j].active && CheckCollisionRecs(darts[i].rect, walls[j].rect)) {
                                darts[i].active = false;
                                break;
                            }
                        }
                        if (!darts[i].active) continue;
                        
                        for (int j = 0; j < enemyCount; j++) {
                            if (enemies[j].active && CheckCollisionRecs(darts[i].rect, enemies[j].rect)) {
                                enemies[j].health -= 1;
                                darts[i].active = false;
                                if (enemies[j].health <= 0) {
                                    enemies[j].active = false;
                                    enemies[j].deathTimer = 0.5f;
                                    if (dropCount < 30) {
                                        drops[dropCount].pos.x = enemies[j].pos.x;
                                        drops[dropCount].pos.y = enemies[j].pos.y;
                                        drops[dropCount].active = true;
                                        dropCount++;
                                    }
                                    killCount++;
                                }
                                break;
                            }
                        }
                    }
                }

                for (int i = 0; i < enemyCount; i++) {
                    if (!enemies[i].active) {
                        if (enemies[i].deathTimer > 0) {
                            enemies[i].deathTimer -= dt;
                        }
                        continue;
                    }

                    if (enemies[i].deathTimer > 0) continue;

                    float dx = player.pos.x - enemies[i].pos.x;
                    float dy = player.pos.y - enemies[i].pos.y;
                    float dist = sqrt(dx*dx + dy*dy);

                    if (dist < 150 && enemies[i].type == ENEMY_AGGRESSIVE) {
                        float speed = 80;
                        enemies[i].pos.x += (dx / dist) * speed * dt;
                        enemies[i].pos.y += (dy / dist) * speed * dt;
                    } else if (dist < 200 && enemies[i].type == ENEMY_RANGED) {
                        if (enemies[i].attackCooldown <= 0) {
                            for (int j = 0; j < MAX_DARTS; j++) {
                                if (!darts[j].active && dist < 180) {
                                    darts[j].active = true;
                                    darts[j].pos.x = enemies[i].pos.x + 12;
                                    darts[j].pos.y = enemies[i].pos.y + 12;
                                    darts[j].rect.x = enemies[i].pos.x + 12;
                                    darts[j].rect.y = enemies[i].pos.y + 12;
                                    darts[j].rect.width = 10;
                                    darts[j].rect.height = 10;
                                    float dLen = sqrt(dx*dx + dy*dy);
                                    darts[j].velocity.x = (dx / dLen) * 200;
                                    darts[j].velocity.y = (dy / dLen) * 200;
                                    enemies[i].attackCooldown = 2.0f;
                                    break;
                                }
                            }
                        }
                    }

                    if (enemies[i].attackCooldown > 0) enemies[i].attackCooldown -= dt;

                    int ex = (int)(enemies[i].pos.x / TILE_SIZE);
                    int ey = (int)(enemies[i].pos.y / TILE_SIZE);
                    if (ex >= 0 && ex < MAP_WIDTH && ey >= 0 && ey < MAP_HEIGHT) {
                        if (map[ex][ey] == TILE_WATER || (rand() % 100 < 2)) {
                            enemies[i].velocity.x *= -1;
                            enemies[i].velocity.y *= -1;
                        }
                    }
                    enemies[i].pos.x += enemies[i].velocity.x * dt;
                    enemies[i].pos.y += enemies[i].velocity.y * dt;
                    enemies[i].rect.x = enemies[i].pos.x;
                    enemies[i].rect.y = enemies[i].pos.y;

                    if (dist < 25) {
                        gameOver = true;
                        if (coinScore + killCount * 2 > highScore) 
                            highScore = coinScore + killCount * 2;
                    }
                }

                for (int i = 0; i < MAX_DARTS; i++) {
                    if (darts[i].active) {
                        Dart d = darts[i];
                        Rectangle pr = {player.pos.x - 12, player.pos.y - 12, 24, 24};
                        if (CheckCollisionRecs(d.rect, pr)) {
                            gameOver = true;
                            if (coinScore + killCount * 2 > highScore) 
                                highScore = coinScore + killCount * 2;
                        }
                    }
                }

                camera.target.x = player.pos.x;
                camera.target.y = player.pos.y;
            }
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

            DrawText("VERSOES:", screenWidth/2 - 50, 380, 18, WHITE);
            DrawText(" VERMELHO = Normal", screenWidth/2 - 80, 410, 16, (Color){200, 80, 80, 255});
            DrawText(" AZUL = Ranged (atira)", screenWidth/2 - 80, 435, 16, (Color){80, 80, 200, 255});
            DrawText(" ROXO = Agressivo (persegue)", screenWidth/2 - 80, 460, 16, (Color){150, 50, 200, 255});
            
            DrawText("WASD move | SPACE shoot | E get bow", screenWidth/2 - 150, 520, 16, GRAY);
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
                    
                    switch (map[x][y]) {
                        case TILE_WATER:
                            DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){30, 80, 140, 255});
                            break;
                        case TILE_SAND:
                            DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){210, 190, 140, 255});
                            break;
                        case TILE_GRASS:
                            DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){60, 130, 60, 255});
                            break;
                        case TILE_EARTH:
                            DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){120, 90, 60, 255});
                            break;
                        case TILE_FOREST:
                            DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){40, 90, 40, 255});
                            DrawCircle(px + 16, py + 10, 10, (Color){30, 70, 30, 255});
                            break;
                    }
                }
            }

            for (int i = 0; i < coinCount; i++) {
                if (coins[i].active) {
                    float bounce = sinf(GetTime() * 4 + i * 0.5f) * 3;
                    DrawCircle((int)coins[i].rect.x, (int)(coins[i].rect.y + bounce), 10, (Color){255, 215, 0, 255});
                    DrawCircle((int)coins[i].rect.x, (int)(coins[i].rect.y + bounce), 5, (Color){255, 255, 200, 255});
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
                    float angle = atan2(darts[i].velocity.y, darts[i].velocity.x);
                    DrawCircle((int)darts[i].rect.x, (int)darts[i].rect.y, 5, (Color){100, 100, 100, 255});
                    if (darts[i].velocity.x < 0 || darts[i].velocity.y < 0) {
                        DrawCircle((int)darts[i].rect.x - darts[i].velocity.x * 0.02f, 
                                  (int)darts[i].rect.y - darts[i].velocity.y * 0.02f, 3, (Color){200, 50, 50, 255});
                    }
                }
            }

            for (int i = 0; i < enemyCount; i++) {
                if (!enemies[i].active) {
                    if (enemies[i].deathTimer > 0) {
                        DrawCircle((int)enemies[i].pos.x + 12, (int)enemies[i].pos.y + 12, 
                                15 * (enemies[i].deathTimer / 0.5f), (Color){255, 50, 50, 150});
                    }
                    continue;
                }

                Color color;
                if (enemies[i].type == ENEMY_NORMAL) color = (Color){200, 60, 60, 255};
                else if (enemies[i].type == ENEMY_RANGED) color = (Color){60, 60, 200, 255};
                else color = (Color){150, 50, 180, 255};

                DrawRectangleRec(enemies[i].rect, color);
                DrawRectangleLinesEx(enemies[i].rect, 2, (Color){255, 255, 255, 150});
                
                DrawCircle((int)(enemies[i].pos.x + 7), (int)(enemies[i].pos.y + 8), 3, (Color){255, 200, 200, 255});
                DrawCircle((int)(enemies[i].pos.x + 17), (int)(enemies[i].pos.y + 8), 3, (Color){255, 200, 200, 255});
            }

            DrawRectangle((int)player.pos.x - 14, (int)player.pos.y - 14, 28, 28, (Color){80, 140, 220, 255});
            DrawRectangle((int)player.pos.x - 10, (int)player.pos.y - 10, 20, 20, (Color){100, 170, 255, 255});
            DrawCircle((int)player.pos.x, (int)player.pos.y, 4, (Color){255, 255, 255, 255});

            if (bow.active) {
                DrawLineEx(player.pos, (Vector2){player.pos.x + aimDir.x * 25, player.pos.y + aimDir.y * 25}, 2, (Color){255, 255, 255, 150});
            }

            EndMode2D();

            if (gameOver) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 150});
                DrawText("GAME OVER", screenWidth/2 - 100, screenHeight/2 - 40, 40, (Color){255, 100, 100, 255});
                DrawText(TextFormat("Moedas: %i", coinScore), screenWidth/2 - 60, screenHeight/2 + 10, 30, (Color){255, 215, 0, 255});
                DrawText(TextFormat("Kills: %i", killCount), screenWidth/2 - 50, screenHeight/2 + 45, 25, (Color){255, 100, 100, 255});
                int total = coinScore + killCount * 2;
                DrawText(TextFormat("Total: %i", total), screenWidth/2 - 40, screenHeight/2 + 80, 30, (Color){100, 255, 100, 255});
                DrawText(TextFormat("Recorde: %i", highScore), screenWidth/2 - 50, screenHeight/2 + 120, 20, GRAY);
                DrawText("Press SPACE to restart", screenWidth/2 - 100, screenHeight/2 + 150, 18, GRAY);
            } else {
                DrawText(TextFormat("Moedas: %i", coinScore), 20, 20, 20, (Color){255, 215, 0, 255});
                
                if (bow.active) {
                    DrawText(TextFormat("Flechas: %i", ammo), 20, 50, 18, (Color){255, 100, 100, 255});
                    DrawText(TextFormat("Kills: %i", killCount), 20, 75, 18, (Color){255, 100, 100, 255});
                } else {
                    DrawText("Colete o arco!", 20, 50, 18, (Color){200, 100, 50, 255});
                }
                
                DrawText(TextFormat("Inimigos: %i", enemyCount - killCount), 20, 100, 16, WHITE);
                DrawText("WASD move | SPACE shoot | E get bow", screenWidth - 250, 20, 14, GRAY);
                DrawText("ESC for menu", screenWidth - 100, screenHeight - 30, 14, GRAY);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    free(walls);
    return 0;
}