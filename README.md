## srect

srect is a simple collision library. The only supported shapes are AABBs (rectangles that are incapable of rotating). The purpose is for easier prototyping of simple 2D games. Simple platformers, roquelikes, and puzzle games rarely need complex physics solvers, and the verbosity of libraries like box2d can get in the programmer's way.

srect is written in stb style, meaning you need to `#define SRECT_IMPLEMENTATION` in exactly one file and `#include "srect.h"` in all files that need to use the library. srect does not require libm and can be configured to use a custom `realloc` and `free` (`SR_REALLOC` `SR_FREE`). It is written in ANSI C. 

## Raylib Example (C99)

```c
#include "raylib.h"

#define SRECT_IMPLEMENTATION
#include "srect.h"

#define WIDTH 800
#define HEIGHT 600
#define WIN_NAME "testing"
#define FPS 60.0f

#define MOVE_SPEED (120.0f / FPS)

int main(void) {
    sr_Context ctx;
    sr_Body_Id b1, b2, b3;
    float xmove, ymove;
    sr_Rect r;

    xmove = 0;
    ymove = 0;

    sr_context_init(&ctx, 100, SR_SWEEP_X);
    b1 = sr_new_body(&ctx, 200, 200, 60, 60, SR_CENTER, 0, 0);
    b2 = sr_new_body(&ctx, 150, 150, 60, 60, SR_CENTER, 0, 0);
    b3 = sr_new_body(&ctx, 300, 300, 60, 60, SR_CENTER, SR_PRIORITY_STATIC, 0);

    InitWindow(WIDTH, HEIGHT, WIN_NAME);
    SetTargetFPS(FPS);

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_UP)) {
            ymove = -MOVE_SPEED;
        } 
        if (IsKeyDown(KEY_DOWN)) {
            ymove = MOVE_SPEED;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            xmove = MOVE_SPEED;
        }
        if (IsKeyDown(KEY_LEFT)) {
            xmove = -MOVE_SPEED;
        }

        sr_translate_body(&ctx, b1, xmove, ymove);
        sr_resolve_collisions(&ctx);

        BeginDrawing();

        ClearBackground(WHITE);
        sr_get_body_rect(&r, &ctx, b1);
        DrawRectangle(r.min.x, r.min.y, r.max.x - r.min.x, r.max.y - r.min.y, BLUE);

        sr_get_body_rect(&r, &ctx, b2);
        DrawRectangle(r.min.x, r.min.y, r.max.x - r.min.x, r.max.y - r.min.y, RED);

        sr_get_body_rect(&r, &ctx, b3);
        DrawRectangle(r.min.x, r.min.y, r.max.x - r.min.x, r.max.y - r.min.y, GREEN);

        EndDrawing();

        xmove = 0.0f;
        ymove = 0.0f;
    }

    CloseWindow();

    sr_context_deinit(&ctx);

    return 0;
}

```

## Details

The library uses the prune and sweep algorithm to reduce the number of collision checks it must perform. This can be configured in `sr_context_init()` by passing either `SR_SWEEP_X` or `SR_SWEEP_Y`. A level/simulation that is very vertical will prefer to use `SR_SWEEP_Y`, whereas a very flat, horizontal level will prefer to use `SR_SWEEP_X`.

It uses collision priorities to determine how to resolve collisions. If two bodies overlap, then the body with the lower collision priority will be moved. If a body has a collision priority of `SR_PRIORITY_STATIC` or `INT_MAX` then it is considered static and cannot be moved during collision resolution. These static bodies are considered either a wall, ground, or ceiling, and they are used in the `sr_did_body_collide_wall()` functions, which can be useful for platformers (wall jump, climbing, etc).

The library assumes right is the positive x direction and down is the positive y direction.

Any function that returns an `int` or an `sr_Body_Id` can error. A returned value of -1 indicates error, usually a `realloc()` failure or an attempt to use an `sr_Body_Id` that does not exist (buffer overrun). `int` is also used as a boolean return type. For example, the function `sr_did_body_collide()` returns an `int`. It returns -1 if the function errored, 0 if the body did not collide, and 1 if the body did collide.
