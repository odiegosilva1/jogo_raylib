#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_PARTICLES 100
#define MAX_WATER_DROPS 80
#define TILE_SIZE 32

#define TILE_WATER 0
#define TILE_SAND  1
#define TILE_EARTH 2
#define TILE_GRASS 3

typedef struct {
    float x, y;
    float size;
    float alpha;
    float vx, vy;
    float life;
} Particle;

typedef struct {
    int width;
    int height;
    int *tiles;
    float cameraX;
    float cameraY;
} Map;

typedef enum { STATE_SPLASH, STATE_MENU, STATE_GAME, STATE_DROWNED } GameState;

void GenerateMap(Map *map, int screenWidth, int screenHeight);
void UpdateMapCamera(Map *map, float playerX, float playerY, int screenWidth, int screenHeight);
int GetTile(Map *map, int x, int y);

static unsigned int seed = 12345;
static float noise2D(int x, int y) {
    seed = seed * 1103515245 + 12345;
    float n = sinf((float)(seed % 10000) / 10000.0f * 6.28318f) * 0.5f;
    seed = seed * 1103515245 + 12345;
    n += sinf((float)((seed + x * 17 + y * 31) % 10000) / 10000.0f * 6.28318f) * 0.3f;
    seed = seed * 1103515245 + 12345;
    n += sinf((float)((seed + x * 43 + y * 67) % 10000) / 10000.0f * 6.28318f) * 0.2f;
    return n;
}

int GetTileType(int x, int y, int mapWidth, int mapHeight) {
    float nx = (float)x / mapWidth;
    float ny = (float)y / mapHeight;
    
    float dist = sqrtf((nx - 0.5f) * (nx - 0.5f) + (ny - 0.5f) * (ny - 0.5f));
    
    float base = 0.55f - dist * 1.5f;
    
    float noiseScale = dist * 0.3f;
    float n = noise2D(x, y) * noiseScale;
    float h = base + n;
    
    if (h > 0.12f) return TILE_GRASS;
    if (h > 0.04f) return TILE_EARTH;
    if (h > -0.04f) return TILE_SAND;
    
    return TILE_WATER;
}

int GetTile(Map *map, int x, int y) {
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) return TILE_WATER;
    return map->tiles[y * map->width + x];
}

void GenerateMap(Map *map, int screenWidth, int screenHeight) {
    map->width = 80;
    map->height = 70;
    map->tiles = malloc(map->width * map->height * sizeof(int));
    map->cameraX = 0;
    map->cameraY = 0;
    
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            map->tiles[y * map->width + x] = GetTileType(x, y, map->width, map->height);
        }
    }
}

void UpdateMapCamera(Map *map, float playerX, float playerY, int screenWidth, int screenHeight) {
    map->cameraX = playerX - screenWidth / 2.0f;
    map->cameraY = playerY - screenHeight / 2.0f;
    
    float maxCamX = map->width * TILE_SIZE - screenWidth;
    float maxCamY = map->height * TILE_SIZE - screenHeight;
    if (maxCamX > 0) map->cameraX = fmaxf(0, fminf(maxCamX, map->cameraX));
    if (maxCamY > 0) map->cameraY = fmaxf(0, fminf(maxCamY, map->cameraY));
}

int main() {
    const int minWidth = 640;
    const int minHeight = 480;

    InitWindow(900, 800, "Island Exploration");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(minWidth, minHeight);

    GameState state = STATE_MENU;
    GameState targetState = STATE_MENU;
    float fadeAlpha = 0.0f;
    float fadeSpeed = 2.0f;
    float splashTimer = 0.0f;

    Map gameMap = {0};
    GenerateMap(&gameMap, 900, 800);

    int spawnTileX = gameMap.width / 2;
    int spawnTileY = gameMap.height / 2;
    float playerX = spawnTileX * TILE_SIZE + TILE_SIZE / 2.0f;
    float playerY = spawnTileY * TILE_SIZE + TILE_SIZE / 2.0f;
    float playerSize = 20.0f;
    float landSpeed = 280.0f;
    float waterSpeed = 80.0f;

    float health = 10.0f;
    float maxHealth = 10.0f;
    float healthLossInterval = 1.5f;
    float healthLossAmount = 3.0f;
    float healthLossTimer = 0.0f;
    int currentTile = TILE_WATER;
    float bobTimer = 0.0f;
    float damageFlash = 0.0f;
    float speedModifier = 1.0f;

    Particle particles[MAX_PARTICLES];
    int particleCount = 0;
    float particleTimer = 0.0f;

    Particle waterDrops[MAX_WATER_DROPS];
    int waterDropCount = 0;
    float waterDropTimer = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        if (fadeAlpha > 0.0f || targetState != state) {
            if (targetState != state) {
                fadeAlpha += fadeSpeed * delta;
                if (fadeAlpha >= 1.0f) {
                    state = targetState;
                    fadeAlpha = 1.0f;
                }
            } else {
                fadeAlpha -= fadeSpeed * delta;
                if (fadeAlpha <= 0.0f) {
                    fadeAlpha = 0.0f;
                }
            }
        }

        if (state == STATE_SPLASH) {
            splashTimer += delta;
            if (splashTimer > 1.5f && fadeAlpha == 0.0f) {
                spawnTileX = gameMap.width / 2;
                spawnTileY = gameMap.height / 2;
                playerX = spawnTileX * TILE_SIZE + TILE_SIZE / 2.0f;
                playerY = spawnTileY * TILE_SIZE + TILE_SIZE / 2.0f;
                health = maxHealth;
                currentTile = GetTile(&gameMap, spawnTileX, spawnTileY);
                bobTimer = 0.0f;
                damageFlash = 0.0f;
                healthLossTimer = 0.0f;
                particleCount = 0;
                waterDropCount = 0;
                targetState = STATE_GAME;
                fadeAlpha = 1.0f;
                targetState = STATE_GAME;
            }
            BeginDrawing();
            ClearBackground((Color){20, 30, 50, 255});
            DrawText("ISLAND EXPLORATION", screenWidth / 2 - MeasureText("ISLAND EXPLORATION", 40) / 2, screenHeight / 2 - 40, 40, WHITE);
            DrawText("Press ENTER or Click to Start", screenWidth / 2 - MeasureText("Press ENTER or Click to Start", 20) / 2, screenHeight / 2 + 20, 20, (Color){150, 150, 150, 255});
            EndDrawing();
            continue;
        }

        if (state == STATE_MENU) {
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                targetState = STATE_SPLASH;
            }
            BeginDrawing();
            ClearBackground((Color){20, 30, 50, 255});
            int btnW = 200;
            int btnH = 50;
            int btnX = screenWidth / 2 - btnW / 2;
            int btnY = screenHeight / 2 - 30;
            Rectangle playBtn = {btnX, btnY, btnW, btnH};
            Color btnColor = CheckCollisionPointRec(GetMousePosition(), playBtn) ? (Color){80, 120, 80, 255} : (Color){40, 80, 40, 255};
            DrawRectangleRec(playBtn, btnColor);
            DrawText("PLAY", btnX + btnW / 2 - MeasureText("PLAY", 30) / 2, btnY + 10, 30, WHITE);
            DrawText("ISLAND EXPLORATION", screenWidth / 2 - MeasureText("ISLAND EXPLORATION", 40) / 2, screenHeight / 4, 40, WHITE);
            if (fadeAlpha > 0.0f) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, (unsigned char)(fadeAlpha * 255)});
            }
            EndDrawing();
            continue;
        }

        if (state == STATE_DROWNED) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                targetState = STATE_MENU;
            }
            BeginDrawing();
            ClearBackground((Color){20, 40, 60, 255});
            DrawText("YOU DROWNED", screenWidth / 2 - MeasureText("YOU DROWNED", 40) / 2, screenHeight / 2 - 40, 40, (Color){150, 180, 220, 255});
            DrawText("Press ESC, ENTER or Click to return", screenWidth / 2 - MeasureText("Press ESC, ENTER or Click to return", 20) / 2, screenHeight / 2 + 20, 20, WHITE);
            if (fadeAlpha > 0.0f) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, (unsigned char)(fadeAlpha * 255)});
            }
            EndDrawing();
            continue;
        }

        float moveX = 0.0f;
        float moveY = 0.0f;

        if (IsKeyDown(KEY_W)) moveY -= 1.0f;
        if (IsKeyDown(KEY_S)) moveY += 1.0f;
        if (IsKeyDown(KEY_A)) moveX -= 1.0f;
        if (IsKeyDown(KEY_D)) moveX += 1.0f;

        int isMoving = (moveX != 0.0f || moveY != 0.0f);

        if (isMoving) {
            float length = sqrtf(moveX * moveX + moveY * moveY);
            moveX /= length;
            moveY /= length;

            float baseSpeed = (currentTile == TILE_WATER) ? waterSpeed : landSpeed;
            playerX += moveX * baseSpeed * speedModifier * delta;
            playerY += moveY * baseSpeed * speedModifier * delta;
        }

        float maxMapX = gameMap.width * TILE_SIZE - playerSize / 2;
        float maxMapY = gameMap.height * TILE_SIZE - playerSize / 2;
        playerX = fmaxf(playerSize / 2, fminf(playerX, maxMapX));
        playerY = fmaxf(playerSize / 2, fminf(playerY, maxMapY));

        int tileX = (int)(playerX / TILE_SIZE);
        int tileY = (int)(playerY / TILE_SIZE);

        currentTile = GetTile(&gameMap, tileX, tileY);

        int isInWater = (currentTile == TILE_WATER);

        if (isInWater) {
            bobTimer += delta * 4.0f;
            speedModifier = 0.35f;

            healthLossTimer += delta;
            if (healthLossTimer >= healthLossInterval) {
                health -= healthLossAmount;
                healthLossTimer = 0.0f;
                damageFlash = 1.0f;
            }

            if (isMoving && waterDropCount < MAX_WATER_DROPS) {
                waterDropTimer += delta;
                if (waterDropTimer > 0.04f) {
                    Particle *wd = &waterDrops[waterDropCount++];
                    wd->x = playerX + GetRandomValue(-12, 12);
                    wd->y = playerY + GetRandomValue(-8, 5);
                    wd->size = GetRandomValue(2, 5);
                    wd->alpha = 0.9f;
                    wd->vx = (GetRandomValue(-1, 1) * 40.0f);
                    wd->vy = -GetRandomValue(40, 90);
                    wd->life = 0.7f;
                    waterDropTimer = 0.0f;
                }
            }
        } else {
            speedModifier = 1.0f;
            bobTimer = 0.0f;
            damageFlash = fmaxf(0.0f, damageFlash - delta * 2.0f);
            healthLossTimer = 0.0f;

            health = fminf(maxHealth, health + 0.8f * delta);

            if (isMoving && particleCount < MAX_PARTICLES) {
                particleTimer += delta;
                if (particleTimer > 0.05f) {
                    Particle *p = &particles[particleCount++];
                    p->x = playerX - moveX * playerSize * 0.6f + GetRandomValue(-5, 5);
                    p->y = playerY - moveY * playerSize * 0.6f + GetRandomValue(-3, 3);
                    p->size = GetRandomValue(3, 6);
                    p->alpha = 0.6f;
                    p->vx = -moveX * 30.0f + GetRandomValue(-10, 10);
                    p->vy = -moveY * 30.0f + GetRandomValue(-10, 10);
                    p->life = 0.5f;
                    particleTimer = 0.0f;
                }
            }
        }

        for (int i = 0; i < waterDropCount; i++) {
            waterDrops[i].x += waterDrops[i].vx * delta;
            waterDrops[i].y += waterDrops[i].vy * delta;
            waterDrops[i].vy += 200.0f * delta;
            waterDrops[i].alpha -= delta * 1.5f;
        }
        int writeIdx = 0;
        for (int i = 0; i < waterDropCount; i++) {
            if (waterDrops[i].alpha > 0) {
                waterDrops[writeIdx++] = waterDrops[i];
            }
        }
        waterDropCount = writeIdx;

        for (int i = 0; i < particleCount; i++) {
            particles[i].x += particles[i].vx * delta;
            particles[i].y += particles[i].vy * delta;
            particles[i].alpha -= delta * 2.0f;
        }
        writeIdx = 0;
        for (int i = 0; i < particleCount; i++) {
            if (particles[i].alpha > 0) {
                particles[writeIdx++] = particles[i];
            }
        }
        particleCount = writeIdx;

        if (health <= 0.0f) {
            targetState = STATE_DROWNED;
        }

        float time = GetTime();

        BeginDrawing();
        ClearBackground((Color){10, 20, 40, 255});

        UpdateMapCamera(&gameMap, playerX, playerY, screenWidth, screenHeight);
        float camOffsetX = gameMap.cameraX;
        float camOffsetY = gameMap.cameraY;

        for (int ty = 0; ty < gameMap.height; ty++) {
            for (int tx = 0; tx < gameMap.width; tx++) {
                int px = tx * TILE_SIZE - (int)camOffsetX;
                int py = ty * TILE_SIZE - (int)camOffsetY;
                if (px < -TILE_SIZE || px > screenWidth || py < -TILE_SIZE || py > screenHeight) continue;

                int tileType = GetTile(&gameMap, tx, ty);

                if (tileType == TILE_GRASS) {
                    Color grassColor = {40, 100, 40, 255};
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, grassColor);
                }
                else if (tileType == TILE_EARTH) {
                    Color earthColor = {80, 60, 40, 255};
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, earthColor);
                }
                else if (tileType == TILE_SAND) {
                    Color sandColor = {210, 190, 140, 255};
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, sandColor);
                }
                else {
                    float wave = sinf(time * 2.0f + tx * 0.5f + ty * 0.3f);
                    unsigned char waterBase = (unsigned char)(20 + wave * 8);
                    unsigned char waterBlue = (unsigned char)(80 + wave * 20);
                    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, (Color){waterBase, waterBase + 20, waterBlue, 255});
                }
            }
        }

        for (int i = 0; i < waterDropCount; i++) {
            unsigned char alpha = (unsigned char)(waterDrops[i].alpha * 255);
            Color c = {200, 230, 255, alpha};
            DrawCircle((int)waterDrops[i].x - (int)camOffsetX, (int)waterDrops[i].y - (int)camOffsetY, waterDrops[i].size, c);
        }

        for (int i = 0; i < particleCount; i++) {
            unsigned char alpha = (unsigned char)(particles[i].alpha * 255);
            Color c = {120, 160, 80, alpha};
            DrawCircle((int)particles[i].x - (int)camOffsetX, (int)particles[i].y - (int)camOffsetY, particles[i].size, c);
        }

        float bobOffset = isInWater ? sinf(bobTimer) * 3.0f : 0.0f;
        float displayY = playerY + bobOffset;

        Color playerColor = WHITE;

        if (health <= 0.0f && isInWater) {
            playerColor = (Color){120, 120, 120, 230};
        } else if (isInWater) {
            if (damageFlash > 0.1f) {
                unsigned char flash = (unsigned char)(damageFlash * 255);
                playerColor = (Color){255, 255 - flash, 255 - flash, 255};
            } else {
                playerColor = (Color){180, 200, 255, 255};
            }
        }

        DrawRectangleRec((Rectangle){playerX - playerSize / 2 - camOffsetX, displayY - playerSize / 2 - camOffsetY, playerSize, playerSize}, playerColor);

        if (isInWater && health < maxHealth) {
            char dmgText[16];
            sprintf(dmgText, "-%.1f", healthLossAmount);
            int textW = MeasureText(dmgText, 12);
            DrawText(dmgText, (int)playerX - textW / 2 - (int)camOffsetX, (int)playerY - 30 - (int)camOffsetY, 12, (Color){255, 100, 100, 255});
        }

        if (!isInWater && health < maxHealth) {
            char regenText[16];
            sprintf(regenText, "+%.1f", 0.8f);
            int textW = MeasureText(regenText, 12);
            DrawText(regenText, (int)playerX - textW / 2 - (int)camOffsetX, (int)playerY - 30 - (int)camOffsetY, 12, (Color){100, 255, 100, 255});
        }

        int hudMargin = 10;
        int barWidth = 150;
        int barHeight = 15;
        
        DrawRectangle(hudMargin, screenHeight - hudMargin - barHeight, barWidth, barHeight, (Color){50, 50, 50, 200});
        float healthRatio = health / maxHealth;
        Color healthColor = healthRatio > 0.5f ? (Color){50, 200, 50, 255} : (healthRatio > 0.25f ? (Color){200, 200, 50, 255} : (Color){200, 50, 50, 255});
        DrawRectangle(hudMargin, screenHeight - hudMargin - barHeight, barWidth * healthRatio, barHeight, healthColor);
        DrawRectangleLines(hudMargin, screenHeight - hudMargin - barHeight, barWidth, barHeight, WHITE);

        char hpText[32];
        sprintf(hpText, "HP: %.1f/%.1f", health, maxHealth);
        DrawText(hpText, hudMargin + 5, screenHeight - hudMargin - barHeight + 2, 10, WHITE);

        char coordsText[32];
        sprintf(coordsText, "X: %d Y: %d", tileX, tileY);
        DrawText(coordsText, hudMargin, screenHeight - hudMargin - barHeight - 18, 12, WHITE);

        int miniMapSize = 100;
        int miniMapX = screenWidth - miniMapSize - hudMargin;
        int miniMapY = screenHeight - miniMapSize - hudMargin;
        DrawRectangle(miniMapX, miniMapY, miniMapSize, miniMapSize, (Color){0, 0, 0, 150});
        
        float scaleX = (float)miniMapSize / gameMap.width;
        float scaleY = (float)miniMapSize / gameMap.height;
        
        for (int my = 0; my < gameMap.height; my++) {
            for (int mx = 0; mx < gameMap.width; mx++) {
                int mt = GetTile(&gameMap, mx, my);
                Color mc;
                if (mt == TILE_GRASS) mc = (Color){40, 100, 40, 255};
                else if (mt == TILE_EARTH) mc = (Color){80, 60, 40, 255};
                else if (mt == TILE_SAND) mc = (Color){210, 190, 140, 255};
                else mc = (Color){20, 40, 80, 255};
                DrawRectangle(miniMapX + mx * scaleX, miniMapY + my * scaleY, scaleX + 1, scaleY + 1, mc);
            }
        }
        
        int playerMiniX = miniMapX + tileX * scaleX;
        int playerMiniY = miniMapY + tileY * scaleY;
        DrawRectangle(playerMiniX, playerMiniY, scaleX + 1, scaleY + 1, RED);

        if (fadeAlpha > 0.0f) {
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, (unsigned char)(fadeAlpha * 255)});
        }
        EndDrawing();
    }

    free(gameMap.tiles);
    CloseWindow();
    return 0;
}