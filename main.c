#define NOB_IMPLEMENTATION
#include "nob.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define RADIUS 0.2f

static void handle_input(float* plane, bool* camera_flag, Camera* camera);
static Camera initialization();
static void loop(Camera camera);

typedef struct {
    Vector3 pos;
    Color color;
} Point;

typedef struct {
    Vector3 pos;
    Color color;
} Centroid;

typedef struct {
    Point* items;
    size_t count;
    size_t capacity;
} Points;

int main(void)
{
    Camera camera = initialization();

    loop(camera);

    CloseWindow();

    return 0;
}

static void loop(Camera camera)
{
    Points ds   = {0};
    float plane = 0, d = 0;
    bool camera_flag = 0;

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

            /*
             *  draw only on the Z = 0 plane.
             *  d = (P0 - l0)*n / l*n
             *  where:
             *  P0 = (0, 0, z)
             *  l0 = ray.position
             *  l = ray.direction
             *  n = (0,0,1)
             */

            Point p = {0};
            Ray ray = {camera.position, Vector3Normalize(Vector3Subtract(
                                            camera.target, camera.position))};

            if (ray.direction.z != 0) {
                d = (plane - ray.position.z) / ray.direction.z;
            }

            p.pos = (Vector3){
                ray.position.x + ray.direction.x * d,
                ray.position.y + ray.direction.y * d,
                plane,
            };
            p.color = RED;

            da_append(&ds, p);
        }

        handle_input(&plane, &camera_flag, &camera);

        BeginDrawing();

        ClearBackground(BLUE);

        BeginMode3D(camera);

        nob_da_foreach(Point, p, &ds)
        {
            DrawSphere(p->pos, RADIUS, p->color);
        }

        EndMode3D();

        EndDrawing();
    }
}

static Camera initialization()
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Dots");

    SetTargetFPS(60);

    return (Camera){
        (Vector3){0.0f, 0.0f, 5.0f}, //
        (Vector3){0.0f, 0.0f, 0.0f}, //
        (Vector3){0.0f, 1.0f, 0.0f}, //
        60.0f,
        CAMERA_PERSPECTIVE,
    };
}

static void handle_input(float* plane, bool* camera_flag, Camera* camera)
{
    if (IsKeyDown(KEY_E)) {
        *plane -= GetFrameTime() * 5.0f;
    }

    if (IsKeyDown(KEY_Q)) {
        *plane += GetFrameTime() * 5.0f;
    }

    if (IsKeyPressed(KEY_F)) {
        if (IsCursorHidden()) {
            EnableCursor();
        } else {
            DisableCursor();
        }
    }

    if (IsKeyPressed(KEY_R)) {
        if (camera_flag)
            *camera_flag = 0;
        else
            *camera_flag = 1;
    }

    if (camera_flag) {
        UpdateCamera(camera, CAMERA_FREE);
    }
}
