#define NOB_IMPLEMENTATION
#include "nob.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FOV 60.0f
#define FPS 60
#define NORMAL_RADIUS 0.2f
#define CENTROID_RADIUS 0.4f
#define MAX_CENTROIDS 10

typedef struct {
    Vector3 pos;
    Color color;
} Point;

typedef struct {
    Point* items;
    size_t count;
    size_t capacity;
} Points;

static Camera initialization()
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Dots");

    SetTargetFPS(FPS);

    DisableCursor();

    return (Camera){
        (Vector3){0.0f, 0.0f, 5.0f}, //
        (Vector3){0.0f, 0.0f, 0.0f}, //
        (Vector3){0.0f, 1.0f, 0.0f}, //
        FOV,
        CAMERA_PERSPECTIVE,
    };
}

static void handle_input(float* plane, Color* color, bool* camera_flag,
                         bool* point_flag, Camera* camera)
{
    static bool used_centroids[MAX_CENTROIDS] = {0};

    if (IsKeyDown(KEY_E)) {
        *plane -= GetFrameTime() * 5.0f;
    }

    if (IsKeyDown(KEY_Q)) {
        *plane += GetFrameTime() * 5.0f;
    }

    if (IsKeyPressed(KEY_O)) {
        if (IsCursorHidden()) {
            EnableCursor();
        } else {
            DisableCursor();
        }
    }

    if (IsKeyPressed(KEY_P)) {
        if (*camera_flag)
            *camera_flag = 0;
        else
            *camera_flag = 1;
    }

    // centroid
    if (!used_centroids[0] && IsKeyPressed(KEY_ONE)) {
        *point_flag       = 1;
        *color            = PINK;
        used_centroids[0] = true;
    }

    // centroid
    if (!used_centroids[1] && IsKeyPressed(KEY_TWO)) {
        *point_flag       = 1;
        *color            = YELLOW;
        used_centroids[1] = true;
    }

    if (*camera_flag) {
        UpdateCamera(camera, CAMERA_FREE);
    }
}

void kmeans(Points* normal_points, Points* centroids)
{
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        // calculate one k-means iteration
        struct {
            Vector3 pos;
            size_t size;
        } new_cents[MAX_CENTROIDS] = {0};

        for (int i = 0; i < MAX_CENTROIDS; ++i) {
            printf("size %zu: %f, %f, %f\n", new_cents[i].size,
                   new_cents[i].pos.x, new_cents[i].pos.y, new_cents[i].pos.z);
        }

        nob_da_foreach(Point, c, centroids)
        {
            size_t index         = c - centroids->items;
            new_cents[index].pos = c->pos;
        }

        nob_da_foreach(Point, n, normal_points)
        {
            float min        = INT_MAX;
            size_t min_index = 0;

            nob_da_foreach(Point, c, centroids)
            {
                float dist = Vector3DistanceSqr(n->pos, c->pos);
                if (dist < min) {
                    min       = dist;
                    min_index = c - centroids->items;
                }
            }

            n->color = centroids->items[min_index].color;

            new_cents[min_index].pos =
                Vector3Add(new_cents[min_index].pos, n->pos);
            new_cents[min_index].size += 1;
        }

        for (int i = 0; i < MAX_CENTROIDS; ++i) {
            printf("size %zu: %f, %f, %f\n", new_cents[i].size,
                   new_cents[i].pos.x, new_cents[i].pos.y, new_cents[i].pos.z);
        }

        nob_da_foreach(Point, c, centroids)
        {
            size_t index = c - centroids->items;
            c->pos       = Vector3Scale(new_cents[index].pos,
                                        1 / (float)new_cents[index].size);

            printf("centroid %zu: %f, %f, %f\n", index, c->pos.x, c->pos.y,
                   c->pos.z);
        }
    }
}

static void loop(Camera camera)
{
    Points normal_points = {0};
    Points centroids     = {0};

    float plane = 0, d = 0;
    bool camera_flag = 1; // one = camera on
    bool point_flag  = 0; // zero = normal points
    Color color      = RED;

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
            p.color = color;

            if (!point_flag) {
                da_append(&normal_points, p);
            } else {
                da_append(&centroids, p);
            }

            point_flag = 0;
            color      = RED;
        }

        handle_input(&plane, &color, &camera_flag, &point_flag, &camera);

        kmeans(&normal_points, &centroids);

        BeginDrawing();

        ClearBackground(BLUE);

        BeginMode3D(camera);

        nob_da_foreach(Point, p, &normal_points)
        {
            DrawSphere(p->pos, NORMAL_RADIUS, p->color);
        }

        nob_da_foreach(Point, p, &centroids)
        {
            DrawSphere(p->pos, CENTROID_RADIUS, p->color);
        }

        EndMode3D();

        EndDrawing();
    }
}

int main(void)
{
    Camera camera = initialization();

    loop(camera);

    CloseWindow();

    return 0;
}
