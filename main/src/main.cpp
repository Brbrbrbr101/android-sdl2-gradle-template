#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

struct Vec2 {
    float x, y;
    Vec2 operator+(const Vec2& v) const { return {x + v.x, y + v.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
};

float getDist(Vec2 a, Vec2 b) {
    return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

class Bullet {
public:
    Vec2 pos;
    Vec2 velocity;
    bool active = false;
    bool enemyBullet = false;
    void update(float dt) { if (active) pos = pos + (velocity * dt); }
};

class Powerup {
public:
    Vec2 pos;
    int type; // 1: Fast Fire, 2: Spread
    bool active = false;
};

class Actor {
public:
    Vec2 pos;
    float rotation = 0;
    int health = 100;
    float aiTimer = 0;
    Vec2 aiDir = {0, 0};
    Actor(float x, float y) : pos({x, y}) {}
    void updateAI(float dt, Vec2 playerPos, float speedBoost) {
        aiTimer -= dt;
        if (aiTimer <= 0) {
            float dx = playerPos.x - pos.x, dy = playerPos.y - pos.y;
            float len = sqrtf(dx*dx + dy*dy + 0.1f);
            aiDir = {dx/len, dy/len};
            aiTimer = 0.5f + (float)rand()/RAND_MAX;
        }
        pos = pos + (aiDir * (200.0f + speedBoost) * dt);
        rotation = atan2f(playerPos.y - pos.y, playerPos.x - pos.x);
    }
};

class GameEngine {
public:
    Actor* player;
    std::vector<Actor*> enemies;
    static const int MAX_BULLETS = 150;
    Bullet bulletPool[MAX_BULLETS];
    std::vector<Powerup> powerups;
    Mix_Music* bgm = nullptr;
    float spawnTimer = 0, gameTime = 0, spawnInterval = 3.0f, powerupTimer = 0;
    int powerupType = 0;
    Vec2 lJoyBase, rJoyBase, lJoyHandle, rJoyHandle;
    float joyRadius = 120.0f;
    Vec2 moveInput = {0, 0};
    bool isFiring = false;

    GameEngine(int w, int h) {
        player = new Actor(w / 2, h / 2);
        lJoyBase = {(float)w * 0.15f, (float)h * 0.75f};
        rJoyBase = {(float)w * 0.85f, (float)h * 0.75f};
        lJoyHandle = lJoyBase; rJoyHandle = rJoyBase;
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) >= 0) {
            bgm = Mix_LoadMUS("AQF.mp3");
            if (bgm) { Mix_VolumeMusic(60); Mix_PlayMusic(bgm, -1); }
        }
        for (int i=0; i<MAX_BULLETS; i++) bulletPool[i].active = false;
    }

    void spawnBullet(Vec2 start, float angle, bool isEnemy) {
        for (int i=0; i<MAX_BULLETS; i++) {
            if (!bulletPool[i].active) {
                bulletPool[i].active = true;
                bulletPool[i].pos = start;
                bulletPool[i].enemyBullet = isEnemy;
                float s = isEnemy ? 400.0f : 1100.0f;
                bulletPool[i].velocity = {cosf(angle)*s, sinf(angle)*s};
                return;
            }
        }
    }

    void update(float dt, int w, int h) {
        gameTime += dt; spawnTimer += dt;
        if (powerupTimer > 0) powerupTimer -= dt; else powerupType = 0;
        float diffMult = floor(gameTime / 60.0f);
        spawnInterval = std::max(0.5f, 2.0f - (diffMult * 0.4f)); 

        if (spawnTimer >= spawnInterval) {
            enemies.push_back(new Actor(rand()%w, rand()%2 == 0 ? 0 : h));
            spawnTimer = 0;
            if (rand()%12 == 0) powerups.push_back({{ (float)(rand()%(w-100)+50), (float)(rand()%(h-100)+50) }, rand()%2+1, true});
        }

        player->pos = player->pos + (moveInput * 500.0f * dt);
        player->pos.x = std::clamp(player->pos.x, 50.0f, (float)w - 50.0f);
        player->pos.y = std::clamp(player->pos.y, 50.0f, (float)h - 50.0f);

        static float fRate = 0; fRate += dt;
        float delay = (powerupType == 1) ? 0.06f : 0.18f;
        if (isFiring && fRate > delay) {
            if (powerupType == 2) { 
                for(float a=-0.25f; a<=0.25f; a+=0.25f) spawnBullet(player->pos, player->rotation + a, false); 
            } else { spawnBullet(player->pos, player->rotation, false); }
            fRate = 0;
        }

        for (auto& p : powerups) if (p.active && getDist(player->pos, p.pos) < 60) { p.active = false; powerupType = p.type; powerupTimer = 8.0f; }

        for (int i=0; i<MAX_BULLETS; i++) {
            if (!bulletPool[i].active) continue;
            bulletPool[i].update(dt);
            if (bulletPool[i].pos.x < 0 || bulletPool[i].pos.x > w || bulletPool[i].pos.y < 0 || bulletPool[i].pos.y > h) { bulletPool[i].active = false; continue; }
            if (!bulletPool[i].enemyBullet) {
                for (auto* e : enemies) if (getDist(bulletPool[i].pos, e->pos) < 45) { e->health -= 40; bulletPool[i].active = false; break; }
            } else if (getDist(bulletPool[i].pos, player->pos) < 30) { player->health -= 5; bulletPool[i].active = false; }
        }

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            (*it)->updateAI(dt, player->pos, diffMult * 40.0f);
            if (rand()%150 == 0) spawnBullet((*it)->pos, (*it)->rotation, true);
            if ((*it)->health <= 0) { delete *it; it = enemies.erase(it); } else ++it;
        }
        if (player->health <= 0) { player->health = 100; gameTime = 0; enemies.clear(); powerups.clear(); }
    }
};

void drawFlagEnemy(SDL_Renderer* rr, Vec2 pos) {
    int x = (int)pos.x - 35, y = (int)pos.y - 25, w = 70, h = 50;
    SDL_SetRenderDrawColor(rr, 255, 255, 255, 255); SDL_Rect bg = {x, y, w, h}; SDL_RenderFillRect(rr, &bg);
    SDL_SetRenderDrawColor(rr, 0, 56, 184, 255);
    SDL_Rect s1 = {x, y + 5, w, 7}, s2 = {x, y + h - 12, w, 7};
    SDL_RenderFillRect(rr, &s1); SDL_RenderFillRect(rr, &s2);
    SDL_RenderDrawLine(rr, x+w/2, y+15, x+w/2-10, y+35); SDL_RenderDrawLine(rr, x+w/2, y+15, x+w/2+10, y+35);
    SDL_RenderDrawLine(rr, x+w/2-10, y+35, x+w/2+10, y+35);
    SDL_RenderDrawLine(rr, x+w/2, y+40, x+w/2-10, y+20); SDL_RenderDrawLine(rr, x+w/2, y+40, x+w/2+10, y+20);
    SDL_RenderDrawLine(rr, x+w/2-10, y+20, x+w/2+10, y+20);
}

void drawJoy(SDL_Renderer* rr, Vec2 base, Vec2 handle, float r) {
    SDL_Rect b = {(int)base.x-(int)r, (int)base.y-(int)r, (int)r*2, (int)r*2};
    SDL_SetRenderDrawColor(rr, 255, 255, 255, 60); SDL_RenderDrawRect(rr, &b);
    SDL_Rect h = {(int)handle.x-30, (int)handle.y-30, 60, 60};
    SDL_SetRenderDrawColor(rr, 255, 255, 255, 120); SDL_RenderFillRect(rr, &h);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); IMG_Init(IMG_INIT_PNG);
    SDL_DisplayMode dm; SDL_GetCurrentDisplayMode(0, &dm);
    SDL_Window* window = SDL_CreateWindow("Sand Survival", 0, 0, dm.w, dm.h, SDL_WINDOW_FULLSCREEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Surface* s = IMG_Load("player.png"); SDL_Texture* pTex = SDL_CreateTextureFromSurface(renderer, s); SDL_FreeSurface(s);
    GameEngine game(dm.w, dm.h); SDL_Event ev; Uint32 last = SDL_GetTicks(); bool run = true;
    while (run) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) run = false;
            if (ev.type == SDL_FINGERDOWN || ev.type == SDL_FINGERMOTION) {
                float tx = ev.tfinger.x * dm.w, ty = ev.tfinger.y * dm.h;
                if (tx < (float)dm.w/2.0f) {
                    float dx = tx-game.lJoyBase.x, dy = ty-game.lJoyBase.y, d = sqrtf(dx*dx+dy*dy+0.1f);
                    game.moveInput = {dx/d, dy/d}; game.lJoyHandle = {game.lJoyBase.x+(dx/d)*std::min(d, game.joyRadius), game.lJoyBase.y+(dy/d)*std::min(d, game.joyRadius)};
                } else {
                    float dx = tx-game.rJoyBase.x, dy = ty-game.rJoyBase.y, d = sqrtf(dx*dx+dy*dy+0.1f);
                    game.player->rotation = atan2f(dy, dx); game.isFiring = true; 
                    game.rJoyHandle = {game.rJoyBase.x+(dx/d)*std::min(d, game.joyRadius), game.rJoyBase.y+(dy/d)*std::min(d, game.joyRadius)};
                }
            }
            if (ev.type == SDL_FINGERUP) { 
                if (ev.tfinger.x * dm.w < (float)dm.w/2.0f) { game.moveInput = {0,0}; game.lJoyHandle = game.lJoyBase; } 
                else { game.isFiring = false; game.rJoyHandle = game.rJoyBase; } 
            }
        }
        float dt = (SDL_GetTicks() - last) / 1000.0f; 
        if (dt > 0.1f) dt = 0.1f;
        last = SDL_GetTicks(); game.update(dt, dm.w, dm.h);
        SDL_SetRenderDrawColor(renderer, 237, 201, 175, 255); SDL_RenderClear(renderer);
        for (auto& p : game.powerups) if (p.active) { SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); SDL_Rect pr = {(int)p.pos.x-20, (int)p.pos.y-20, 40, 40}; SDL_RenderFillRect(renderer, &pr); }
        if (pTex) { SDL_Rect pr = {(int)game.player->pos.x-45, (int)game.player->pos.y-45, 90, 90}; SDL_RenderCopyEx(renderer, pTex, NULL, &pr, game.player->rotation*180.0/M_PI, NULL, SDL_FLIP_NONE); }
        drawJoy(renderer, game.lJoyBase, game.lJoyHandle, game.joyRadius); drawJoy(renderer, game.rJoyBase, game.rJoyHandle, game.joyRadius);
        for (auto* e : game.enemies) drawFlagEnemy(renderer, e->pos);
        for (int i=0; i<game.MAX_BULLETS; i++) if (game.bulletPool[i].active) {
            SDL_SetRenderDrawColor(renderer, game.bulletPool[i].enemyBullet ? 200:255, 255, game.bulletPool[i].enemyBullet ? 0:255, 255);
            SDL_Rect br = {(int)game.bulletPool[i].pos.x-5, (int)game.bulletPool[i].pos.y-5, 10, 10}; SDL_RenderFillRect(renderer, &br);
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); SDL_Rect hF = {50, 50, (int)(400 * (game.player->health/100.0f)), 40}; SDL_RenderFillRect(renderer, &hF);
        SDL_RenderPresent(renderer);
    }
    return 0;
}
