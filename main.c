#define NOB_IMPLEMENTATION
#include "nob.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#include "rcamera.h"

#define FS                                                                     \
    "/home/aregmk/MyStuff/git-stuff/raylib/examples/shaders/resources/"        \
    "shaders/glsl330/lighting.fs"

#define VS                                                                     \
    "/home/aregmk/MyStuff/git-stuff/raylib/examples/shaders/resources/"        \
    "shaders/glsl330/lighting.vs"

typedef struct {
    Vector3* items;
    size_t count;
    size_t capacity;
} Dots;

int main(void)
{
    Dots ds           = {0};
    Vector2 mouse_pos = {0};
    Vector3 pos       = {0};
    float depth_pos = 0, plane = 0, d = 0;
    bool camera_flag = 0;

    InitWindow(1280, 720, "Dots");

    Camera camera     = {0};
    camera.position   = (Vector3){0.0f, 0.0f, 5.0f};
    camera.target     = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up         = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy       = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

            // draw only on the Z = 0 plane.
            // d = (P0 - l0)*n / l*n
            // where:
            // P0 = (0, 0, z)
            // l0 = ray.position
            // l = ray.direction
            // n = (0,0,1)

            Ray ray = {camera.position, Vector3Normalize(Vector3Subtract(
                                            camera.target, camera.position))};

            if (ray.direction.z != 0) {
                d = (plane - ray.position.z) / ray.direction.z;
            }

            pos = (Vector3){
                ray.position.x + ray.direction.x * d,
                ray.position.y + ray.direction.y * d,
                plane,
            };

            // printf("%f, %f, %f\n", pos.x, pos.y, pos.z);

            da_append(&ds, pos);
        }

        if (IsKeyDown(KEY_P)) {
            plane -= GetFrameTime() * 5.0f;
        }

        if (IsKeyDown(KEY_O)) {
            plane += GetFrameTime() * 5.0f;
        }

        if (IsKeyPressed(KEY_E)) {
            if (IsCursorHidden()) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        if (IsKeyPressed(KEY_Q)) {
            if (camera.projection == CAMERA_ORTHOGRAPHIC) {
                camera.projection = CAMERA_PERSPECTIVE;
            } else {
                camera.projection = CAMERA_ORTHOGRAPHIC;
            }
        }

        if (IsKeyPressed(KEY_R)) {
            if (camera_flag)
                camera_flag = 0;
            else
                camera_flag = 1;
        }

        if (camera_flag) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);

        nob_da_foreach(Vector3, dot, &ds)
        {
            DrawSphere(*dot, 0.2f, RED);
        }

        EndMode3D();

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
