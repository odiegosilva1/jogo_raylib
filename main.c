#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define MAX_PARTICLES 100

typedef struct {
    float x, y;
    float size;
    float alpha;
    float vx, vy;
} Particle;

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Island Exploration");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    float playerX = screenWidth / 2.0f;
    float playerY = screenHeight / 2.0f;
    float playerSize = 20.0f;
    float speed = 300.0f;

    Particle particles[MAX_PARTICLES];
    int particleCount = 0;
    float particleTimer = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

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

            playerX += moveX * speed * delta;
            playerY += moveY * speed * delta;

            particleTimer += delta;
            if (particleTimer > 0.05f && particleCount < MAX_PARTICLES) {
                Particle *p = &particles[particleCount++];
                p->x = playerX - moveX * playerSize * 0.6f;
                p->y = playerY - moveY * playerSize * 0.6f;
                p->size = GetRandomValue(3, 8);
                p->alpha = 0.5f;
                p->vx = -moveX * 20.0f + GetRandomValue(-20, 20);
                p->vy = -moveY * 20.0f + GetRandomValue(-20, 20);
                particleTimer = 0.0f;
            }
        }

        playerX = fmaxf(playerSize / 2, fminf(playerX, GetScreenWidth() - playerSize / 2));
        playerY = fmaxf(playerSize / 2, fminf(playerY, GetScreenHeight() - playerSize / 2));

        for (int i = 0; i < particleCount; i++) {
            particles[i].x += particles[i].vx * delta;
            particles[i].y += particles[i].vy * delta;
            particles[i].alpha -= delta * 2.0f;
            particles[i].size -= delta * 10.0f;
        }

        int writeIdx = 0;
        for (int i = 0; i < particleCount; i++) {
            if (particles[i].alpha > 0 && particles[i].size > 0) {
                particles[writeIdx++] = particles[i];
            }
        }
        particleCount = writeIdx;

        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < particleCount; i++) {
            Color c = {200, 200, 200, (unsigned char)(particles[i].alpha * 255)};
            DrawCircle((int)particles[i].x, (int)particles[i].y, particles[i].size, c);
        }

        DrawRectangleRec((Rectangle){playerX - playerSize / 2, playerY - playerSize / 2, playerSize, playerSize}, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}