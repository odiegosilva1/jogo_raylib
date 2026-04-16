#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_PARTICLES 50
#define MAX_ENEMIES 8
#define MAX_PROJECTILES 20
#define MAX_FLOORS 6

#define TILE_SIZE 32
#define FLOOR_WIDTH 20
#define FLOOR_HEIGHT 12
#define FLOOR_TILE_SIZE 40

typedef enum { STATE_HELICOPTER, STATE_ELEVATOR, STATE_FLOOR, STATE_ROOFTOP } GameState;

typedef struct {
    float x, y;
    float vx, vy;
    float size;
    float alpha;
} Particle;

typedef struct {
    float x, y;
    float vx, vy;
    float damage;
} Projectile;

typedef struct {
    float x, y;
    float angle;
    float patrolPoints[4][2];
    int patrolIndex;
    float patrolTimer;
    float shootCooldown;
} Enemy;

typedef struct {
    int floorNum;
    int hasKey;
    int enemyCount;
    Enemy enemies[4];
    int exitX;
    int exitY;
} Floor;

int floorMap[MAX_FLOORS][FLOOR_HEIGHT][FLOOR_WIDTH] = {
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,1,0,1,1,1,1,0,0,0,1,1,1,1,0,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,0,0,1,1,1,1,1,0,0,1,1,1,0,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,0,0,1,1,1,1,1,0,0,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,0,0,1,1,1,1,1,0,0,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,1,0,1,1,1,1,0,0,0,1,1,1,1,0,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    }
};

int main() {
    InitWindow(900, 700, "Elevator Mission");
    SetTargetFPS(60);

    GameState state = STATE_HELICOPTER;
    int currentFloor = 5;
    int hasBriefcase = 0;
    int gameWon = 0;
    int gameOver = 0;

    float playerX = FLOOR_WIDTH * FLOOR_TILE_SIZE / 2;
    float playerY = FLOOR_HEIGHT * FLOOR_TILE_SIZE - 60;
    float playerSize = 20;
    float playerSpeed = 150.0f;

    float health = 10.0f;
    float maxHealth = 10.0f;

    Projectile projectiles[MAX_PROJECTILES];
    int projectileCount = 0;

    Particle particles[MAX_PARTICLES];
    int particleCount = 0;
    float particleTimer = 0.0f;

    float elevatorY = 0;
    float elevatorTarget = 0;
    float elevatorSpeed = 100.0f;

    int heliTimer = 0;
    float heliAngle = 0;

    for (int f = 0; f < MAX_FLOORS; f++) {
        for (int i = 0; i < 4; i++) {
            int ex = 2 + (i % 2) * 14;
            int ey = 2 + i * 2;
            floorMap[f][ey][ex] = 0;
            floorMap[f][ey+1][ex] = 0;
        }
    }

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        if (state == STATE_HELICOPTER) {
            heliTimer += delta;
            heliAngle += delta * 2.0f;
            
            if (heliTimer > 3.0f) {
                state = STATE_FLOOR;
                playerX = FLOOR_WIDTH * FLOOR_TILE_SIZE / 2;
                playerY = FLOOR_HEIGHT * FLOOR_TILE_SIZE - 60;
            }
        }
        else if (state == STATE_ELEVATOR) {
            if (elevatorTarget > elevatorY) {
                elevatorY += elevatorSpeed * delta;
                if (elevatorY >= elevatorTarget) {
                    elevatorY = elevatorTarget;
                    state = STATE_FLOOR;
                }
            } else {
                elevatorY -= elevatorSpeed * delta;
                if (elevatorY <= elevatorTarget) {
                    elevatorY = elevatorTarget;
                    state = STATE_FLOOR;
                }
            }
            
            if (IsKeyPressed(KEY_SPACE)) {
                int targetFloor = currentFloor;
                if (IsKeyDown(KEY_UP) && currentFloor < MAX_FLOORS - 1) targetFloor = currentFloor + 1;
                else if (IsKeyDown(KEY_DOWN) && currentFloor > 0) targetFloor = currentFloor - 1;
                
                if (targetFloor != currentFloor) {
                    elevatorTarget = (MAX_FLOORS - 1 - targetFloor) * 200.0f;
                    currentFloor = targetFloor;
                    state = STATE_ELEVATOR;
                } else {
                    state = STATE_FLOOR;
                }
            }
        }
        else if (state == STATE_FLOOR) {
            float moveX = 0.0f;
            float moveY = 0.0f;
            if (IsKeyDown(KEY_W)) moveY -= 1.0f;
            if (IsKeyDown(KEY_S)) moveY += 1.0f;
            if (IsKeyDown(KEY_A)) moveX -= 1.0f;
            if (IsKeyDown(KEY_D)) moveX += 1.0f;

            if (IsKeyPressed(KEY_SPACE) && projectileCount < MAX_PROJECTILES) {
                float shootAngle = 0;
                if (moveX != 0 || moveY != 0) {
                    shootAngle = atan2f(moveY, moveX);
                }
                Projectile *p = &projectiles[projectileCount++];
                p->x = playerX;
                p->y = playerY;
                p->vx = cosf(shootAngle) * 400.0f;
                p->vy = sinf(shootAngle) * 400.0f;
                p->damage = 2.0f;
            }

            if (moveX != 0.0f || moveY != 0.0f) {
                float length = sqrtf(moveX * moveX + moveY * moveY);
                moveX /= length;
                moveY /= length;
                
                float newX = playerX + moveX * playerSpeed * delta;
                float newY = playerY + moveY * playerSpeed * delta;
                
                int tileX = (int)(newX / FLOOR_TILE_SIZE);
                int tileY = (int)(playerY / FLOOR_TILE_SIZE);
                if (tileX >= 0 && tileX < FLOOR_WIDTH && tileY >= 0 && tileY < FLOOR_HEIGHT && floorMap[currentFloor][tileY][tileX] == 0) {
                    playerX = newX;
                }
                
                tileX = (int)(playerX / FLOOR_TILE_SIZE);
                tileY = (int)(newY / FLOOR_TILE_SIZE);
                if (tileX >= 0 && tileX < FLOOR_WIDTH && tileY >= 0 && tileY < FLOOR_HEIGHT && floorMap[currentFloor][tileY][tileX] == 0) {
                    playerY = newY;
                }
            }

            int elevatorAreaX = FLOOR_WIDTH * FLOOR_TILE_SIZE - 60;
            if (playerX > elevatorAreaX && playerY > FLOOR_HEIGHT * FLOOR_TILE_SIZE - 100) {
                state = STATE_ELEVATOR;
            }

            if (currentFloor == 2 && !hasBriefcase && playerX > FLOOR_WIDTH * FLOOR_TILE_SIZE / 2 - 50 && 
                playerX < FLOOR_WIDTH * FLOOR_TILE_SIZE / 2 + 50 && playerY > 100 && playerY < 200) {
                hasBriefcase = 1;
            }

            if (currentFloor == 0 && playerY < 80) {
                if (hasBriefcase) {
                    gameWon = 1;
                } else {
                    state = STATE_ELEVATOR;
                    elevatorTarget = (MAX_FLOORS - 1) * 200.0f;
                    currentFloor = MAX_FLOORS - 1;
                    state = STATE_ELEVATOR;
                }
            }

            int enemySpawnCount = 3 + currentFloor;
            for (int e = 0; e < enemySpawnCount; e++) {
                float ex = 100 + (e * 150) % (FLOOR_WIDTH * FLOOR_TILE_SIZE - 200);
                float ey = 100 + (e * 80) % (FLOOR_HEIGHT * FLOOR_TILE_SIZE - 200);
                
                if (e < 4) {
                    float dx = playerX - ex;
                    float dy = playerY - ey;
                    float dist = sqrtf(dx * dx + dy * dy);
                    
                    if (dist < 150.0f && projectiles[e].x <= 0) {
                        if (projectileCount < MAX_PROJECTILES) {
                            float angle = atan2f(dy, dx);
                            Projectile *p = &projectiles[projectileCount++];
                            p->x = ex;
                            p->y = ey;
                            p->vx = cosf(angle) * 150.0f;
                            p->vy = sinf(angle) * 150.0f;
                            p->damage = 1.0f;
                        }
                    }
                }
                
                for (int p = 0; p < projectileCount; p++) {
                    if (projectiles[p].x > 0) {
                        float dx = projectiles[p].x - ex;
                        float dy = projectiles[p].y - ey;
                        if (sqrtf(dx*dx + dy*dy) < 20) {
                            projectiles[p].x = -100;
                        }
                    }
                }
            }

            for (int i = 0; i < projectileCount; i++) {
                if (projectiles[i].x > 0) {
                    projectiles[i].x += projectiles[i].vx * delta;
                    projectiles[i].y += projectiles[i].vy * delta;
                    
                    int tx = (int)(projectiles[i].x / FLOOR_TILE_SIZE);
                    int ty = (int)(projectiles[i].y / FLOOR_TILE_SIZE);
                    if (tx < 0 || tx >= FLOOR_WIDTH || ty < 0 || ty >= FLOOR_HEIGHT || floorMap[currentFloor][ty][tx] == 1) {
                        projectiles[i].x = -100;
                    }
                }
            }

            float dx = playerX - (FLOOR_WIDTH * FLOOR_TILE_SIZE / 2);
            float dy = playerY - (FLOOR_HEIGHT * FLOOR_TILE_SIZE - 60);
            if (sqrtf(dx*dx + dy*dy) < 800) {
                health = fminf(maxHealth, health + 2.0f * delta);
            }

            for (int p = 0; p < projectileCount; p++) {
                if (projectiles[p].x > 0) {
                    float pdx = playerX - projectiles[p].x;
                    float pdy = playerY - projectiles[p].y;
                    if (sqrtf(pdx*pdx + pdy*pdy) < playerSize + 5) {
                        health -= projectiles[p].damage;
                        projectiles[p].x = -100;
                        if (health <= 0) gameOver = 1;
                    }
                }
            }

            particleTimer += delta;
            if (particleTimer > 0.05f && particleCount < MAX_PARTICLES && (moveX != 0 || moveY != 0)) {
                Particle *p = &particles[particleCount++];
                p->x = playerX + (GetRandomValue(-5, 5));
                p->y = playerY + (GetRandomValue(-5, 5));
                p->vx = -moveX * 30 + GetRandomValue(-10, 10);
                p->vy = -moveY * 30 + GetRandomValue(-10, 10);
                p->size = GetRandomValue(2, 4);
                p->alpha = 0.7f;
                particleTimer = 0;
            }
        }

        for (int i = 0; i < particleCount; i++) {
            particles[i].x += particles[i].vx * delta;
            particles[i].y += particles[i].vy * delta;
            particles[i].alpha -= delta * 2.0f;
        }
        
        int writeIdx = 0;
        for (int i = 0; i < particleCount; i++) {
            if (particles[i].alpha > 0) particles[writeIdx++] = particles[i];
        }
        particleCount = writeIdx;

        BeginDrawing();
        ClearBackground((Color){15, 15, 20, 255});

        if (state == STATE_HELICOPTER) {
            DrawText("MISSÃO: Encontre a maleta no 3o andar e escape pelo terreo!", 
                      screenWidth/2 - 250, 50, 20, (Color){200, 200, 200, 255});
            DrawText("Controles: WASD = Mover | ESPACO = Atirar", 
                      screenWidth/2 - 150, 80, 16, (Color){150, 150, 150, 255});
            
            float heliX = screenWidth/2 + sinf(heliAngle) * 50;
            float heliY = 200;
            DrawRectangle(heliX - 40, heliY - 15, 80, 30, (Color){80, 80, 90, 255});
            DrawRectangle(heliX - 5, heliY - 25, 10, 15, (Color){60, 60, 70, 255});
            DrawCircle(heliX - 30, heliY, 15, (Color){100, 100, 110, 255});
            DrawCircle(heliX + 30, heliY, 15, (Color){100, 100, 110, 255});
            
            float ropeLen = (heliTimer * 50);
            if (ropeLen < 150) {
                DrawLine(heliX, heliY + 15, heliX, heliY + 15 + ropeLen, (Color){100, 100, 100, 255});
                
                if (ropeLen > 100) {
                    DrawRectangle(screenWidth/2 - 10, screenHeight/2, 20, 20, (Color){255, 255, 255, 255});
                    DrawText("PRONTO!", screenWidth/2 - 30, screenHeight/2 - 30, 20, (Color){200, 200, 200, 255});
                }
            }
            
            DrawText("Helicoptero chegando ao teto do edificio...", 
                      screenWidth/2 - 150, 350, 18, (Color){100, 100, 100, 255});
        }
        else if (state == STATE_ELEVATOR) {
            int elevWidth = 50;
            int elevHeight = 600;
            int elevX = FLOOR_WIDTH * FLOOR_TILE_SIZE + 20;
            int elevY = 50;
            
            DrawRectangle(elevX, elevY, elevWidth, elevHeight, (Color){50, 50, 60, 255});
            DrawRectangle(elevX + 5, elevY + (int)elevatorY, elevWidth - 10, 180, (Color){80, 80, 90, 255});
            
            for (int f = 0; f < MAX_FLOORS; f++) {
                int fy = elevY + f * 100 + 10;
                char floorText[20];
                sprintf(floorText, "%d", MAX_FLOORS - f);
                DrawText(floorText, elevX + 20, fy, 16, (Color){150, 150, 150, 255});
                
                if (currentFloor == f) {
                    DrawRectangle(elevX, fy - 2, 5, 20, (Color){100, 200, 100, 255});
                }
            }
            
            DrawText("PRESSIONE ESPACO PARA ESCOLHER ANDAR", screenWidth/2 - 160, 50, 16, (Color){150, 150, 150, 255});
            DrawText("USE SETAS CIMA/BAIXO PARA SELECIONAR", screenWidth/2 - 160, 75, 14, (Color){120, 120, 120, 255});
            
            char currentText[30];
            sprintf(currentText, "Andar atual: %d", currentFloor + 1);
            DrawText(currentText, 20, 20, 18, (Color){200, 200, 200, 255});
            
            int barWidth = 150;
            DrawRectangle(20, 50, barWidth, 15, (Color){40, 40, 40, 255});
            float hpRatio = health / maxHealth;
            unsigned char hpColor = (unsigned char)(200 * hpRatio + 55);
            DrawRectangle(20, 50, (int)(barWidth * hpRatio), 15, (Color){hpColor, hpColor, hpColor, 255});
            DrawRectangleLines(20, 50, barWidth, 15, (Color){100, 100, 100, 255});
            
            char hpText[20];
            sprintf(hpText, "HP: %.0f", health);
            DrawText(hpText, 25, 52, 10, (Color){0, 0, 0, 255});
        }
        else if (state == STATE_FLOOR) {
            for (int y = 0; y < FLOOR_HEIGHT; y++) {
                for (int x = 0; x < FLOOR_WIDTH; x++) {
                    int px = x * FLOOR_TILE_SIZE;
                    int py = y * FLOOR_TILE_SIZE;
                    
                    if (floorMap[currentFloor][y][x] == 1) {
                        DrawRectangle(px, py, FLOOR_TILE_SIZE, FLOOR_TILE_SIZE, (Color){60, 60, 70, 255});
                        DrawRectangle(px + 2, py + 2, FLOOR_TILE_SIZE - 4, FLOOR_TILE_SIZE - 4, (Color){70, 70, 80, 255});
                    } else {
                        DrawRectangle(px, py, FLOOR_TILE_SIZE, FLOOR_TILE_SIZE, (Color){20, 20, 25, 255});
                    }
                }
            }
            
            int elevAreaX = FLOOR_WIDTH * FLOOR_TILE_SIZE - 60;
            DrawRectangle(elevAreaX, FLOOR_HEIGHT * FLOOR_TILE_SIZE - 80, 60, 80, (Color){50, 50, 60, 255});
            DrawText("ELEV", elevAreaX + 10, FLOOR_HEIGHT * FLOOR_TILE_SIZE - 60, 12, (Color){150, 150, 150, 255});
            
            if (currentFloor == 2 && !hasBriefcase) {
                int bx = FLOOR_WIDTH * FLOOR_TILE_SIZE / 2;
                int by = 150;
                DrawRectangle(bx - 15, by - 10, 30, 20, (Color){80, 60, 30, 255});
                DrawRectangle(bx - 15, by - 10, 30, 5, (Color){100, 80, 50, 255});
                DrawText("MALETA!", bx - 25, by + 15, 10, (Color){180, 150, 100, 255});
            }
            
            if (hasBriefcase) {
                DrawRectangle(20, 90, 25, 15, (Color){80, 60, 30, 255});
            }
            
            for (int i = 0; i < particleCount; i++) {
                unsigned char alpha = (unsigned char)(particles[i].alpha * 255);
                DrawRectangle((int)particles[i].x, (int)particles[i].y, (int)particles[i].size, (int)particles[i].size, (Color){150, 150, 150, alpha});
            }
            
            for (int i = 0; i < projectileCount; i++) {
                if (projectiles[i].x > 0) {
                    DrawCircle((int)projectiles[i].x, (int)projectiles[i].y, 5, (Color){255, 100, 100, 255});
                }
            }
            
            int enemyCount = 3 + currentFloor;
            for (int e = 0; e < enemyCount; e++) {
                float ex = 100 + (e * 150) % (FLOOR_WIDTH * FLOOR_TILE_SIZE - 200);
                float ey = 100 + (e * 80) % (FLOOR_HEIGHT * FLOOR_TILE_SIZE - 200);
                DrawCircle((int)ex, (int)ey, 12, (Color){150, 50, 50, 255});
            }
            
            DrawRectangle((int)playerX - playerSize/2, (int)playerY - playerSize/2, playerSize, playerSize, (Color){255, 255, 255, 255});
            
            int barWidth = 150;
            DrawRectangle(20, 50, barWidth, 15, (Color){40, 40, 40, 255});
            float hpRatio = health / maxHealth;
            unsigned char hpColor = (unsigned char)(200 * hpRatio + 55);
            DrawRectangle(20, 50, (int)(barWidth * hpRatio), 15, (Color){hpColor, hpColor, hpColor, 255});
            DrawRectangleLines(20, 50, barWidth, 15, (Color){100, 100, 100, 255});
            
            char hpText[20];
            sprintf(hpText, "HP: %.0f", health);
            DrawText(hpText, 25, 52, 10, (Color){0, 0, 0, 255});
            
            char floorText[30];
            sprintf(floorText, "Andar: %d | Maleta: %s", currentFloor + 1, hasBriefcase ? "SIM" : "NAO");
            DrawText(floorText, 20, 20, 16, (Color){200, 200, 200, 255});
            
            char instrText[60];
            sprintf(instrText, "WASD=Mover | ESPACO=Atirar | Elevador=a direita");
            DrawText(instrText, 20, screenHeight - 30, 12, (Color){100, 100, 100, 255});
        }
        
        if (gameWon) {
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 200});
            DrawText("MISSION COMPLETA!", screenWidth/2 - 100, screenHeight/2 - 20, 30, (Color){100, 255, 100, 255});
            DrawText("Voce escapou com a maleta!", screenWidth/2 - 90, screenHeight/2 + 20, 20, (Color){200, 200, 200, 255});
        }
        
        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 200});
            DrawText("GAME OVER", screenWidth/2 - 60, screenHeight/2 - 20, 30, (Color){255, 100, 100, 255});
            DrawText("Voce foi derrotado!", screenWidth/2 - 70, screenHeight/2 + 20, 20, (Color){200, 200, 200, 255});
        }

        EndDrawing();
        
        if (gameWon || gameOver) {
            break;
        }
    }

    CloseWindow();
    return 0;
}