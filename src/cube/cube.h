#include "stdio.h"
#include "esp_log.h"
#include "helpers.h"
#include "freertos/FreeRTOS.h"

typedef enum{
    FACE_U,
    FACE_L,
    FACE_R,
    FACE_D,
    FACE_F,
    FACE_B,
} cube_face_t;

typedef enum{
    CW,
    CCW,
    D_180,
} cube_rotation_t;

typedef struct{
    cube_face_t face;
    cube_rotation_t move;
} cube_move_t;

bool parse_move(const char *step, cube_move_t *move);
bool do_move(const char *step);
