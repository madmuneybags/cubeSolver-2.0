#include "cube.h"

bool parse_move(const char *step, cube_move_t *move)
{
    if ( step == NULL || move == NULL){
        return false;
    }
    switch (step[0]){
        case 'R':
            move->face = FACE_R;
            break;
        case 'L':
            move->face = FACE_L;
            break;
        case 'F':
            move->face = FACE_F;
            break;
        case 'B':
            move->face = FACE_B;
            break;
        case 'U':
            move->face = FACE_U;
            break;
        case 'D':
            move->face = FACE_D;
            break;
        default: 
            return false;
    }
    switch (step[1]){
        case '\0':
            move->move = CW;
            break;
        case '2':
            move->move = D_180;
            break;
        case '\'':
            move->move = CCW;
            break;
        default:
            return false;
    }
    return true;
}

bool do_move(const char *step)
{
    cube_move_t move;
    if (parse_move(step, &move)){
        switch (move.face){
            case FACE_U:
                rotate_motor(MOTOR_U_STEP, MOTOR_U_DIR, move.move);

        }
    }
}

void rotate_motor(int step_pin, int dir_pin, cube_rotation_t move)
{
    if (move == CCW){
        gpio_set_level(dir_pin, 1);
    } else if (move == CW){
        gpio_set_level(dir_pin, 0);
    }
    esp_rom_delay_us(2);  
    for (int i = 0; i < MOTOR_HALF_TURN; i++){
        gpio_set_level(step_pin, 1);
        esp_rom_delay_us(2);
        gpio_set_level(step_pin, 0);
        esp_rom_delay_us(800);
    }
}
