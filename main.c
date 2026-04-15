#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_PARTICLES 100
#define MAX_WATER_DROPS 80
#define TILE_SIZE 64
#define MAX_DROPS 50
#define MAX_INVENTORY 20
#define MAX_TREE_HP 3
#define MAX_GROWING_TREES 50
#define GROW_TIME 1200.0f
#define TREE_HIT_COOLDOWN 0.3f

typedef enum { ITEM_LOG, ITEM_SEED, ITEM_STONE, ITEM_LOG_DRY, ITEM_LOG_ORANGE } ItemType;

typedef struct {
    float x, y;
    float vx, vy;
    ItemType type;
    float lifetime;
    float alpha;
    int collected;
} ItemDrop;

typedef struct {
    int x, y;
    int hp;
    int maxHp;
} TreeCutting;

typedef struct {
    int x, y;
    float plantTime;
    int grows;
} TreeGrowth;

typedef struct {
    ItemType type;
    int quantity;
} InventoryItem;

static InventoryItem inventory[MAX_INVENTORY];
static int inventoryCount = 0;
static ItemDrop drops[MAX_DROPS];
static int dropCount = 0;
static float camOffsetX = 0;
static float camOffsetY = 0;
static TreeCutting cuttingTree = {-1, -1, 0, 0};
static TreeGrowth growingTrees[MAX_GROWING_TREES];
static int growingCount = 0;
static float totalTime = 0.0f;
static float hitCooldown = 0.0f;

#define TILE_WATER 0
#define TILE_SAND  1
#define TILE_EARTH 2
#define TILE_GRASS 3
#define TILE_TREE  4

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

typedef enum { STATE_SPLASH, STATE_MENU, STATE_GAME, STATE_PAUSE, STATE_DROWNED } GameState;

void GenerateMap(Map *map, int w, int h);
void UpdateMapCamera(Map *map, float px, float py, int sw, int sh);
int GetTile(Map *map, int x, int y);

int main() {
    const int minWidth = 640;
    const int minHeight = 480;

    InitWindow(900, 800, "Colonialismo Experience");
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
    float playerSize = 30.0f;
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
        totalTime += delta;
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
            }
            BeginDrawing();
            ClearBackground((Color){0, 0, 0, 255});
            DrawText("Colonialismo Experience", screenWidth / 2 - MeasureText("Colonialismo Experience", 50) / 2, screenHeight / 2 - 60, 50, WHITE);
            DrawText("A journey through the seas", screenWidth / 2 - MeasureText("A journey through the seas", 20) / 2, screenHeight / 2, 20, (Color){150, 150, 150, 255});
            DrawText("Click to Start", screenWidth / 2 - MeasureText("Click to Start", 20) / 2, screenHeight / 2 + 60, 20, WHITE);
            DrawText("A game by @unopllayer", screenWidth / 2 - MeasureText("A game by @unopllayer", 16) / 2, screenHeight - 40, 16, (Color){100, 100, 100, 255});
            EndDrawing();
            continue;
        }

        if (state == STATE_MENU) {
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                targetState = STATE_SPLASH;
            }
            BeginDrawing();
            ClearBackground((Color){0, 0, 0, 255});
            DrawText("Colonialismo Experience", screenWidth / 2 - MeasureText("Colonialismo Experience", 40) / 2, screenHeight / 4, 40, WHITE);
            DrawText("A journey through the seas", screenWidth / 2 - MeasureText("A journey through the seas", 20) / 2, screenHeight / 4 + 50, 20, (Color){150, 150, 150, 255});
            int btnW = 200;
            int btnH = 50;
            int btnX = screenWidth / 2 - btnW / 2;
            int btnY = screenHeight / 2 - 30;
            Rectangle playBtn = {btnX, btnY, btnW, btnH};
            Color btnColor = CheckCollisionPointRec(GetMousePosition(), playBtn) ? (Color){80, 80, 80, 255} : (Color){40, 40, 40, 255};
            DrawRectangleRec(playBtn, btnColor);
            DrawText("PLAY", btnX + btnW / 2 - MeasureText("PLAY", 30) / 2, btnY + 10, 30, WHITE);
            
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
        
        if (state == STATE_PAUSE) {
            BeginDrawing();
            ClearBackground((Color){0, 0, 0, 180});
            DrawText("PAUSED", screenWidth / 2 - MeasureText("PAUSED", 40) / 2, screenHeight / 4, 40, WHITE);
            
            int btnW = 200;
            int btnH = 50;
            int btnSpacing = 60;
            int btnStartY = screenHeight / 2 - 30;
            
            Rectangle resumeBtn = {screenWidth / 2 - btnW / 2, btnStartY, btnW, btnH};
            Rectangle saveBtn = {screenWidth / 2 - btnW / 2, btnStartY + btnSpacing, btnW, btnH};
            Rectangle quitBtn = {screenWidth / 2 - btnW / 2, btnStartY + btnSpacing * 2, btnW, btnH};
            
            Vector2 mouse = GetMousePosition();
            Color resumeColor = CheckCollisionPointRec(mouse, resumeBtn) ? (Color){80, 80, 80, 255} : (Color){40, 40, 40, 255};
            Color saveColor = CheckCollisionPointRec(mouse, saveBtn) ? (Color){80, 80, 80, 255} : (Color){40, 40, 40, 255};
            Color quitColor = CheckCollisionPointRec(mouse, quitBtn) ? (Color){80, 80, 80, 255} : (Color){40, 40, 40, 255};
            
            DrawRectangleRec(resumeBtn, resumeColor);
            DrawRectangleRec(saveBtn, saveColor);
            DrawRectangleRec(quitBtn, quitColor);
            
            DrawText("RESUME", btnStartY + btnW / 2 - MeasureText("RESUME", 30) / 2, btnStartY + 10, 30, WHITE);
            DrawText("SAVE", btnStartY + btnSpacing + btnW / 2 - MeasureText("SAVE", 30) / 2, btnStartY + btnSpacing + 10, 30, WHITE);
            DrawText("QUIT", btnStartY + btnSpacing * 2 + btnW / 2 - MeasureText("QUIT", 30) / 2, btnStartY + btnSpacing * 2 + 10, 30, WHITE);
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, resumeBtn)) {
                    targetState = STATE_GAME;
                } else if (CheckCollisionPointRec(mouse, saveBtn)) {
                } else if (CheckCollisionPointRec(mouse, quitBtn)) {
                    targetState = STATE_MENU;
                }
            }
            
            if (IsKeyPressed(KEY_ESCAPE)) {
                targetState = STATE_GAME;
            }
            
            EndDrawing();
            continue;
        }

        float moveX = 0.0f;
        float moveY = 0.0f;

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (state == STATE_GAME) targetState = STATE_PAUSE;
            else if (state == STATE_PAUSE) targetState = STATE_GAME;
        }

        if (state == STATE_GAME) {
            Vector2 mouse = GetMousePosition();
            float worldMouseX = mouse.x + camOffsetX;
            float worldMouseY = mouse.y + camOffsetY;
            
            if (hitCooldown > 0.0f) hitCooldown -= delta;
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hitCooldown <= 0.0f) {
                int clickTileX = (int)(worldMouseX / TILE_SIZE);
                int clickTileY = (int)(worldMouseY / TILE_SIZE);
                if (clickTileX >= 0 && clickTileX < gameMap.width && clickTileY >= 0 && clickTileY < gameMap.height) {
                    int clickedTile = GetTile(&gameMap, clickTileX, clickTileY);
                    if (clickedTile == TILE_TREE) {
                        if (cuttingTree.x != clickTileX || cuttingTree.y != clickTileY) {
                            cuttingTree.x = clickTileX;
                            cuttingTree.y = clickTileY;
                            cuttingTree.hp = MAX_TREE_HP - 1;
                            cuttingTree.maxHp = MAX_TREE_HP;
                        } else {
                            cuttingTree.hp--;
                        }
                        hitCooldown = TREE_HIT_COOLDOWN;
                        if (cuttingTree.hp <= 0) {
                            gameMap.tiles[clickTileY * gameMap.width + clickTileX] = TILE_GRASS;
                            int logAmount = GetRandomValue(2, 3);
                            for (int d = 0; d < logAmount; d++) {
                                if (dropCount < MAX_DROPS) {
                                    ItemDrop *item = &drops[dropCount];
                                    item->x = clickTileX * TILE_SIZE + TILE_SIZE / 2.0f + GetRandomValue(-20, 20);
                                    item->y = clickTileY * TILE_SIZE + TILE_SIZE / 2.0f + GetRandomValue(-20, 20);
                                    item->vx = GetRandomValue(-30, 30);
                                    item->vy = GetRandomValue(-80, -40);
                                    item->type = ITEM_LOG;
                                    item->lifetime = 10.0f;
                                    item->alpha = 1.0f;
                                    item->collected = 0;
                                    dropCount++;
                                }
                            }
                            int seedAmount = GetRandomValue(1, 2);
                            for (int d = 0; d < seedAmount; d++) {
                                if (dropCount < MAX_DROPS) {
                                    ItemDrop *item = &drops[dropCount];
                                    item->x = clickTileX * TILE_SIZE + TILE_SIZE / 2.0f + GetRandomValue(-20, 20);
                                    item->y = clickTileY * TILE_SIZE + TILE_SIZE / 2.0f + GetRandomValue(-20, 20);
                                    item->vx = GetRandomValue(-30, 30);
                                    item->vy = GetRandomValue(-80, -40);
                                    item->type = ITEM_SEED;
                                    item->lifetime = 10.0f;
                                    item->alpha = 1.0f;
                                    item->collected = 0;
                                    dropCount++;
                                }
                            }
                            cuttingTree.x = -1;
                            cuttingTree.y = -1;
                        }
                    }
                }
            }
            
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                int clickTileX = (int)(worldMouseX / TILE_SIZE);
                int clickTileY = (int)(worldMouseY / TILE_SIZE);
                if (clickTileX >= 0 && clickTileX < gameMap.width && clickTileY >= 0 && clickTileY < gameMap.height) {
                    int clickedTile = GetTile(&gameMap, clickTileX, clickTileY);
                    for (int j = 0; j < inventoryCount; j++) {
                        if (inventory[j].type == ITEM_SEED && inventory[j].quantity > 0) {
                            if (clickedTile == TILE_GRASS || clickedTile == TILE_EARTH) {
                                gameMap.tiles[clickTileY * gameMap.width + clickTileX] = TILE_TREE;
                                if (growingCount < MAX_GROWING_TREES) {
                                    growingTrees[growingCount].x = clickTileX;
                                    growingTrees[growingCount].y = clickTileY;
                                    growingTrees[growingCount].plantTime = totalTime;
                                    growingTrees[growingCount].grows = 1;
                                    growingCount++;
                                }
inventory[j].quantity--;
                                if (inventory[j].quantity <= 0) {
                                    for (int rem = j; rem < inventoryCount - 1; rem++) {
                                        inventory[rem].quantity = inventory[rem + 1].quantity;
                                        inventory[rem].type = inventory[rem + 1].type;
                                    }
                                    inventoryCount--;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            
            totalTime += delta;
            
            for (int i = 0; i < dropCount; i++) {
                if (!drops[i].collected) {
                    float playerScreenX = playerX - camOffsetX;
                    float playerScreenY = playerY - camOffsetY;
                    float dx = playerScreenX - drops[i].x;
                    float dy = playerScreenY - drops[i].y;
                    float distToPlayer = sqrtf(dx * dx + dy * dy);
                    if (distToPlayer < 150.0f && distToPlayer > 5.0f) {
                        float speed = 300.0f * delta;
                        drops[i].x += (dx / distToPlayer) * speed;
                        drops[i].y += (dy / distToPlayer) * speed;
                    }
                    float screenX = drops[i].x - camOffsetX;
                    float screenY = drops[i].y - camOffsetY;
                    float dist = sqrtf((mouse.x - screenX) * (mouse.x - screenX) + (mouse.y - screenY) * (mouse.y - screenY));
                    if (dist < 40.0f || distToPlayer < 15.0f) {
                        drops[i].collected = 1;
                        for (int j = 0; j < inventoryCount; j++) {
                            if (inventory[j].type == drops[i].type && inventory[j].quantity < 64) {
                                inventory[j].quantity += 1;
                                break;
                            }
                        }
                        if (inventoryCount < MAX_INVENTORY) {
                            int created = 0;
                            for (int j = 0; j < inventoryCount; j++) {
                                if (inventory[j].type == drops[i].type && inventory[j].quantity < 64) {
                                    created = 1;
                                    break;
                                }
                            }
                            if (!created) {
                                inventory[inventoryCount].type = drops[i].type;
                                inventory[inventoryCount].quantity = 1;
                                inventoryCount++;
                            }
                        }
                    }
                }
}
            
            if (IsKeyDown(KEY_W)) moveY -= 1.0f;
            if (IsKeyDown(KEY_S)) moveY += 1.0f;
            if (IsKeyDown(KEY_A)) moveX -= 1.0f;
            if (IsKeyDown(KEY_D)) moveX += 1.0f;
        }

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
        int isTree = (currentTile == TILE_TREE);
        
        if (isTree) {
            float treeCenterX = tileX * TILE_SIZE + TILE_SIZE / 2.0f;
            float treeCenterY = tileY * TILE_SIZE + TILE_SIZE / 2.0f;
            float distX = playerX - treeCenterX;
            float distY = playerY - treeCenterY;
            float dist = sqrtf(distX * distX + distY * distY);
            if (dist < 35.0f) {
                float pushDist = 35.0f - dist;
                playerX += (distX / dist) * pushDist;
                playerY += (distY / dist) * pushDist;
            }
        }
        else if (isInWater) {
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
        camOffsetX = gameMap.cameraX;
        camOffsetY = gameMap.cameraY;

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
                else if (tileType == TILE_TREE) {
                    float growScale = 1.0f;
                    for (int g = 0; g < growingCount; g++) {
                        if (growingTrees[g].x == tx && growingTrees[g].y == ty && growingTrees[g].grows) {
                            float elapsed = totalTime - growingTrees[g].plantTime;
                            growScale = 0.24f + (0.76f * (elapsed / GROW_TIME));
                            if (elapsed >= GROW_TIME) {
                                growingTrees[g].grows = 0;
                            }
                            break;
                        }
                    }
                    int groundR = 40, groundG = 100, groundB = 40;
                    int groundTile = GetTile(&gameMap, tx, ty);
                    if (tx >= 0 && tx < gameMap.width && ty >= 0 && ty < gameMap.height) {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (dx == 0 && dy == 0) continue;
                                int gx = tx + dx, gy = ty + dy;
                                if (gx >= 0 && gx < gameMap.width && gy >= 0 && gy < gameMap.height) {
                                    int gt = GetTile(&gameMap, gx, gy);
                                    if (gt != TILE_TREE && gt != TILE_WATER && gt != -1) {
                                        groundTile = gt;
                                        break;
                                    }
                                }
                            }
                            if (groundTile != -1 && groundTile != GetTile(&gameMap, tx, ty)) break;
                        }
                        if (groundTile == TILE_EARTH) { groundR = 80; groundG = 60; groundB = 40; }
                        else if (groundTile == TILE_SAND) { groundR = 210; groundG = 190; groundB = 140; }
                        else if (groundTile == TILE_GRASS) { groundR = 40; groundG = 100; groundB = 40; }
                        else if (groundTile == TILE_WATER) { groundR = 40; groundG = 100; groundB = 40; }
                        else { groundR = 40; groundG = 100; groundB = 40; }
                    }
                    int fullSize = TILE_SIZE;
                    int smallSize = (int)(fullSize * growScale);
                    int offset = (fullSize - smallSize) / 2;
                    DrawRectangle(px + offset, py + offset, smallSize, smallSize, (Color){groundR, groundG, groundB, 255});
                    int triSize = smallSize;
                    int triOffset = offset;
                    DrawTriangle((Vector2){px + TILE_SIZE/2, py + triOffset}, (Vector2){px + triOffset, py + TILE_SIZE - triOffset}, (Vector2){px + TILE_SIZE - triOffset, py + TILE_SIZE - triOffset}, (Color){20, 60, 20, 255});
                    DrawTriangle((Vector2){px + TILE_SIZE/2, py + triOffset + triSize/6}, (Vector2){px + triOffset + triSize/8, py + TILE_SIZE - triOffset - triSize/6}, (Vector2){px + TILE_SIZE - triOffset - triSize/8, py + TILE_SIZE - triOffset - triSize/6}, (Color){25, 75, 25, 255});
                    int trunkW = 4 + (int)(4 * growScale);
                    int trunkH = TILE_SIZE/2 + (int)(smallSize/4);
                    DrawRectangle(px + TILE_SIZE/2 - trunkW/2, py + TILE_SIZE/2, trunkW, trunkH, (Color){60, 40, 20, 255});
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

        for (int i = 0; i < dropCount; i++) {
            if (!drops[i].collected && drops[i].alpha > 0) {
                int screenX = (int)(drops[i].x - camOffsetX);
                int screenY = (int)(drops[i].y - camOffsetY);
                if (drops[i].type == ITEM_LOG) {
                    DrawRectangle(screenX - 12, screenY - 4, 24, 8, (Color){139, 90, 43, 255});
                    DrawRectangle(screenX - 10, screenY - 2, 20, 4, (Color){160, 110, 60, 255});
                } else if (drops[i].type == ITEM_STONE) {
                    DrawCircle(screenX, screenY, 10, (Color){100, 100, 100, 255});
                    DrawCircle(screenX - 3, screenY - 3, 4, (Color){130, 130, 130, 255});
                } else {
                    DrawCircle(screenX, screenY, 4, (Color){100, 200, 100, 255});
                    DrawCircle(screenX + 3, screenY + 2, 2, (Color){120, 220, 120, 255});
                }
            }
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

        if (cuttingTree.x >= 0 && cuttingTree.y >= 0) {
            float treeScreenX = cuttingTree.x * TILE_SIZE + TILE_SIZE / 2.0f - camOffsetX;
            float treeScreenY = cuttingTree.y * TILE_SIZE - camOffsetY - 10;
            int barW = 40;
            int barH = 6;
            DrawRectangle((int)treeScreenX - barW/2, (int)treeScreenY, barW, barH, (Color){50, 50, 50, 200});
            float hpPercent = (float)cuttingTree.hp / (float)cuttingTree.maxHp;
            DrawRectangle((int)treeScreenX - barW/2, (int)treeScreenY, (int)(barW * hpPercent), barH, (Color){255, 80, 80, 255});
        }

        if (isInWater && health < maxHealth) {
            char dmgText[16];
            sprintf(dmgText, "-%.1f", healthLossAmount);
            int textW = MeasureText(dmgText, 12);
            DrawText(dmgText, (int)playerX - textW / 2 - (int)camOffsetX, (int)playerY - 30 - (int)camOffsetY, 12, (Color){255, 100, 100, 255});
        }
        
        int hudY = screenHeight - 70;
        int slotSize = 50;
        int slotSpacing = 55;
        int maxSlots = 6;
        int slotStartX = screenWidth / 2 - (maxSlots * slotSpacing) / 2;
        for (int i = 0; i < maxSlots; i++) {
            int sx = slotStartX + i * slotSpacing;
            int idx = (i < inventoryCount) ? i : -1;
            DrawRectangle(sx, hudY, slotSize, slotSize, (Color){50, 50, 50, 200});
            DrawRectangle(sx + 2, hudY + 2, slotSize - 4, slotSize - 4, (Color){80, 80, 80, 255});
            int qty = 0;
            ItemType showType = -1;
            if (idx >= 0 && inventory[idx].quantity > 0) {
                qty = inventory[idx].quantity;
                showType = inventory[idx].type;
            }
            if (showType == ITEM_LOG) {
                DrawRectangle(sx + 12, hudY + 15, 26, 10, (Color){139, 90, 43, 255});
            } else if (showType == ITEM_SEED) {
                DrawCircle(sx + slotSize/2, hudY + slotSize/2, 8, (Color){100, 200, 100, 255});
            } else if (showType == ITEM_STONE) {
                DrawCircle(sx + slotSize/2, hudY + slotSize/2, 10, (Color){100, 100, 100, 255});
                DrawCircle(sx + slotSize/2 - 3, hudY + slotSize/2 - 3, 4, (Color){130, 130, 130, 255});
            }
            if (qty > 0) {
                char qtyText[8];
                sprintf(qtyText, "%d", qty);
                DrawText(qtyText, sx + slotSize - 20, hudY + slotSize - 18, 16, WHITE);
            }
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
        DrawText("HP", hudMargin, hudMargin, 16, WHITE);

        int miniMapSize = 120;
        int miniMapX = screenWidth - miniMapSize - hudMargin;
        int miniMapY = hudMargin;
        DrawRectangle(miniMapX - 2, miniMapY - 2, miniMapSize + 4, miniMapSize + 4, (Color){50, 50, 50, 255});
        DrawRectangle(miniMapX, miniMapY, miniMapSize, miniMapSize, (Color){0, 0, 0, 180});
        
        float scaleX = (float)miniMapSize / gameMap.width;
        float scaleY = (float)miniMapSize / gameMap.height;
        
        for (int my = 0; my < gameMap.height; my++) {
            for (int mx = 0; mx < gameMap.width; mx++) {
                int mt = GetTile(&gameMap, mx, my);
                Color mc;
                if (mt == TILE_GRASS) mc = (Color){40, 100, 40, 255};
                else if (mt == TILE_EARTH) mc = (Color){80, 60, 40, 255};
                else if (mt == TILE_SAND) mc = (Color){210, 190, 140, 255};
                else if (mt == TILE_TREE) mc = (Color){15, 50, 15, 255};
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
    float n = noise2D(x, y) * 0.1f;
    float h = base + n;
    
    if (h > 0.12f) return TILE_GRASS;
    if (h > 0.04f) return TILE_EARTH;
    if (h > 0.0f) return TILE_SAND;
    
    return TILE_WATER;
}

int GetTile(Map *map, int x, int y) {
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) return TILE_WATER;
    return map->tiles[y * map->width + x];
}

typedef struct {
    float x, y;
    float radiusX, radiusY;
    float rotation;
    int type;
} Island;

static Island islands[1];
static int islandCount = 1;
static unsigned int worldSeed = 12345;

static float pseudoRand() {
    worldSeed = worldSeed * 1103515245 + 12345;
    return (float)(worldSeed % 10000) / 10000.0f;
}

static float noise1D(float x) {
    return sinf(x * 12.9898f) * 43758.5453f;
}

void GenerateMap(Map *map, int w, int h) {
    map->width = 200;
    map->height = 160;
    map->tiles = malloc(map->width * map->height * sizeof(int));
    map->cameraX = 0;
    map->cameraY = 0;
    
    worldSeed = 12345;
    
    islands[0].x = 0.5f;
    islands[0].y = 0.5f;
    islands[0].radiusX = 0.30f;
    islands[0].radiusY = 0.24f;
    islands[0].rotation = 0.0f;
    islands[0].type = 0;
    
    float minDist = 0.35f;
    for (int i = 1; i < islandCount; i++) {
        float bestX = 0, bestY = 0;
        float bestDist = 0;
        for (int attempts = 0; attempts < 150; attempts++) {
            float rx = 0.1f + (pseudoRand() * 0.8f);
            float ry = 0.1f + (pseudoRand() * 0.8f);
            float distToMain = sqrtf((rx - islands[0].x) * (rx - islands[0].x) + (ry - islands[0].y) * (ry - islands[0].y));
            if (distToMain < 0.35f) continue;
            float minIslandDist = distToMain;
            for (int j = 1; j < i; j++) {
                float d = sqrtf((rx - islands[j].x) * (rx - islands[j].x) + (ry - islands[j].y) * (ry - islands[j].y));
                if (d < minIslandDist) minIslandDist = d;
            }
            if (minIslandDist > bestDist) {
                bestDist = minIslandDist;
                bestX = rx;
                bestY = ry;
            }
            if (minIslandDist >= minDist) break;
        }
        islands[i].x = bestX;
        islands[i].y = bestY;
        islands[i].radiusX = 0.04f + (pseudoRand() * 0.12f);
        islands[i].radiusY = 0.04f + (pseudoRand() * 0.08f);
        islands[i].rotation = pseudoRand() * 3.14159f * 2.0f;
        islands[i].type = (int)(pseudoRand() * 5.0f);
    }
    
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            map->tiles[y * map->width + x] = TILE_WATER;
        }
    }
    
    for (int i = 0; i < islandCount; i++) {
        Island *isl = &islands[i];
        int cx = (int)(isl->x * map->width);
        int cy = (int)(isl->y * map->height);
        int rx = (int)(isl->radiusX * map->width);
        int ry = (int)(isl->radiusY * map->height);
        float rot = isl->rotation;
        
        for (int dy = -ry - 4; dy <= ry + 4; dy++) {
            for (int dx = -rx - 4; dx <= rx + 4; dx++) {
                int tx = cx + dx;
                int ty = cy + dy;
                if (tx < 0 || tx >= map->width || ty < 0 || ty >= map->height) continue;
                
                float angle = atan2f((float)dy, (float)dx);
                float noise = sinf(angle * 2.0f) * 0.25f + sinf(angle * 5.0f) * 0.15f + sinf(angle * 9.0f) * 0.08f + sinf(angle * 15.0f) * 0.04f;
                float dist = sqrtf((float)(dx * dx) / (rx * rx) + (float)(dy * dy) / (ry * ry)) - noise;
                
                if (dist < 0.6f && map->tiles[ty * map->width + tx] == TILE_WATER) {
                    map->tiles[ty * map->width + tx] = TILE_GRASS;
                } else if (dist >= 0.6f && dist < 0.85f && map->tiles[ty * map->width + tx] == TILE_WATER) {
                    map->tiles[ty * map->width + tx] = TILE_EARTH;
                } else if (dist >= 0.85f && dist < 1.2f && map->tiles[ty * map->width + tx] == TILE_WATER) {
                    map->tiles[ty * map->width + tx] = TILE_SAND;
                }
            }
        }
    }
    
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            if (map->tiles[y * map->width + x] == TILE_GRASS || map->tiles[y * map->width + x] == TILE_EARTH) {
                float h = noise2D(x, y);
                if (h > 0.76f) {
                    map->tiles[y * map->width + x] = TILE_TREE;
                }
            }
        }
    }
    
    int stoneCount = (map->width * map->height) / 400;
    for (int s = 0; s < stoneCount; s++) {
        int sx = (int)(pseudoRand() * map->width);
        int sy = (int)(pseudoRand() * map->height);
        int tileAt = map->tiles[sy * map->width + sx];
        if (tileAt == TILE_GRASS || tileAt == TILE_EARTH) {
            if (dropCount < MAX_DROPS) {
                ItemDrop *d = &drops[dropCount];
                d->x = sx * TILE_SIZE + TILE_SIZE / 2.0f;
                d->y = sy * TILE_SIZE + TILE_SIZE / 2.0f;
                d->vx = 0;
                d->vy = 0;
                d->type = ITEM_STONE;
                d->lifetime = -1.0f;
                d->alpha = 1.0f;
                d->collected = 0;
                dropCount++;
            }
        }
    }
}

void UpdateMapCamera(Map *map, float px, float py, int sw, int sh) {
    map->cameraX = px - sw / 2.0f;
    map->cameraY = py - sh / 2.0f;
    
    float maxCamX = map->width * TILE_SIZE - sw;
    float maxCamY = map->height * TILE_SIZE - sh;
    if (maxCamX > 0) map->cameraX = fmaxf(0, fminf(maxCamX, map->cameraX));
    if (maxCamY > 0) map->cameraY = fmaxf(0, fminf(maxCamY, map->cameraY));
}