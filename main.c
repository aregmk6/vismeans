#define NOB_IMPLEMENTATION
#include "nob.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FOV 60.0f
#define FPS 60
#define SENSITIVITY 0.1f
#define NORMAL_RADIUS 0.2f
#define CENTROID_RADIUS 0.4f
#define MAX_CENTROIDS 10
#define NOT_FOUND -10
#define DEF_PLANE_DIST 10

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
        (Vector3){0.0f, 0.0f, 10.0f}, //
        (Vector3){0.0f, 0.0f, 0.0f},  //
        (Vector3){0.0f, 1.0f, 0.0f},  //
        FOV,
        CAMERA_PERSPECTIVE,
    };
}

static void my_camera_update(Camera* camera, bool camera_flag)
{
    static Vector3 movement;
    static Vector3 rotation;

    movement = (Vector3){(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * 0.1f -
                             (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * 0.1f,
                         (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * 0.1f -
                             (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * 0.1f,
                         IsKeyDown(KEY_SPACE) * 0.1f -
                             IsKeyDown(KEY_LEFT_CONTROL) * 0.1f};
    rotation = (Vector3){GetMouseDelta().x * SENSITIVITY,
                         GetMouseDelta().y * SENSITIVITY, 0.0f};

    if (camera_flag) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            UpdateCameraPro(camera, Vector3Scale(movement, 2.0f), rotation,
                            0.0f);
        } else {
            UpdateCameraPro(camera, movement, rotation, 0.0f);
        }
    }
}

static void handle_general_input(float* plane, Camera* camera)
{
    static bool camera_flag = 1; // one = camera on
    static bool plane_flag  = 0; // one = plane movement on

    if (plane_flag) {
        *plane = camera->position.z - DEF_PLANE_DIST;
    }

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
        if (camera_flag)
            camera_flag = 0;
        else
            camera_flag = 1;
    }

    if (IsKeyPressed(KEY_F)) {
        if (plane_flag)
            plane_flag = 0;
        else
            plane_flag = 1;
    }

    my_camera_update(camera, camera_flag);
}

static void kmeans(Points* normal_points, Points* centroids)
{
    if (IsKeyDown(KEY_T)) {
        // calculate one k-means iteration
        struct {
            Vector3 pos;
            size_t size;
        } new_cents[MAX_CENTROIDS] = {0};

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
            if (new_cents[index].size != 0) {
                c->pos = Vector3Scale(new_cents[index].pos,
                                      1 / (float)new_cents[index].size);
            }

            printf("centroid %zu: %f, %f, %f\n", index, c->pos.x, c->pos.y,
                   c->pos.z);
        }
    }
}

static Point calc_new_point(Camera* camera, Color* color, float plane)
{

    /*
     *  draw only on the Z = 0 plane.
     *  d = (P0 - l0)*n / l*n
     *  where:
     *  P0 = (0, 0, z)
     *  l0 = ray.position
     *  l = ray.direction
     *  n = (0,0,1)
     */

    float d = 0;
    Point p = {0};
    Ray ray = {camera->position, Vector3Normalize(Vector3Subtract(
                                     camera->target, camera->position))};

    if (ray.direction.z != 0) {
        d = (plane - ray.position.z) / ray.direction.z;
    }

    p.pos = (Vector3){
        ray.position.x + ray.direction.x * d,
        ray.position.y + ray.direction.y * d,
        plane,
    };
    p.color = *color;
    return p;
}

static void pick_centroid(int* cur_cent, bool* used_centroids)
{
    if (*cur_cent != NOT_FOUND) {
        *cur_cent += (int)GetMouseWheelMove();

        if (*cur_cent <= 0) {
            *cur_cent = 0;
        } else if (*cur_cent >= MAX_CENTROIDS - 1) {
            *cur_cent = MAX_CENTROIDS - 1;
        }
        if (used_centroids[*cur_cent]) {
            // printf("cent %d is used, checking for unused:\n", cur_cent);
            for (int i = (*cur_cent + 1) % MAX_CENTROIDS; i != *cur_cent;
                 i     = (i + 1) % MAX_CENTROIDS) {
                if (!used_centroids[i]) {
                    // printf("FOUND! cent %d is unused\n", i);
                    *cur_cent = i;
                    break;
                }
                // printf("cent %d is also used, move on...\n", i);
                if (i == *cur_cent - 1) {
                    printf("NOT FOUND! there are no unused cents!\n");
                    *cur_cent = NOT_FOUND;
                    break;
                }
            }
        }
    }
}

static void loop(Camera camera)
{
    static bool used_centroids[MAX_CENTROIDS] = {0}; // zero = unused
    static Color color_picker[]               = {
        GRAY, YELLOW,   ORANGE, PINK,  MAROON,
        LIME, DARKBLUE, VIOLET, BEIGE, BROWN,
    };
    static const char* color_names[] = {
        "GRAY", "YELLOW",   "ORANGE", "PINK",  "MAROON",
        "LIME", "DARKBLUE", "VIOLET", "BEIGE", "BROWN",
    };
    static Color default_color = RED;

    Points normal_points = {0};
    Points centroids     = {0};

    float plane  = 0;
    int cur_cent = 0;

    while (!WindowShouldClose()) {

        pick_centroid(&cur_cent, used_centroids);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            da_append(&normal_points,
                      calc_new_point(&camera, &default_color, plane));
        }

        if (cur_cent != NOT_FOUND && !used_centroids[cur_cent] &&
            IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            used_centroids[cur_cent] = true;
            da_append(&centroids,
                      calc_new_point(&camera, &color_picker[cur_cent], plane));
        }

        handle_general_input(&plane, &camera);

        kmeans(&normal_points, &centroids);

        BeginDrawing();

        ClearBackground(WHITE);

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

        if (cur_cent != NOT_FOUND) {
            DrawText(TextFormat("Current Centroid: %s", color_names[cur_cent]),
                     10, 10, 20, RED);
        } else {
            DrawText("Out of Centroids", 10, 10, 20, RED);
        }

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
