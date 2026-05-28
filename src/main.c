#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "http.h"
#include "cube.h"
#include "helpers.h"

#define MAX_HMI sizeof(char) * 58

typedef enum {
    STATE_IDLE,
    STATE_WAIT_SERVER,
    STATE_RUN_MOTOR,
    STATE_ERROR,
    STATE_HTTP_ERROR,
    STATE_SOLVE_SUCCESS
} system_state;

typedef enum {
    HMI_STOP,
    HMI_CUBE_DATA,
} cmd;

typedef enum {
    APP_EVENT_HMI_STOP,
    APP_EVENT_HMI_CUBE,
    APP_EVENT_HTTP_SUCCESS,
    APP_EVENT_HTTP_FAIL,
    APP_EVENT_CUBE_FINISH,
} app_event_type_t;

typedef struct {
    app_event_type_t type;
    char data[128];
} app_event_t;

QueueHandle_t uart_queue;
QueueHandle_t xAppEventQueue;
MessageBufferHandle_t xCubeState;
MessageBufferHandle_t xCubeSolution;


void vFSM( void * pvParameters) {
    // initial state
    system_state state = STATE_IDLE;

    // event notifications
    app_event_t event;



    for(;;){
        // awaits a task notification
        xQueueReceive(xAppEventQueue, &event, portMAX_DELAY);

        switch (state) {
            case STATE_IDLE:
                if (event.type == APP_EVENT_HMI_CUBE)
                {
                    xMessageBufferSend(xCubeState, event.data, strlen(event.data), pdMS_TO_TICKS(100));
                    state = STATE_WAIT_SERVER;

                }
                break;

            case STATE_WAIT_SERVER:
                if (event.type == APP_EVENT_HTTP_SUCCESS)
                {
                    xMessageBufferSend(xCubeSolution, event.data, strlen(event.data), pdMS_TO_TICKS(100));
                    state = STATE_RUN_MOTOR;
                } else {
                    state = STATE_HTTP_ERROR;
                }

                break;

            case STATE_RUN_MOTOR:
                if (event.type == APP_EVENT_CUBE_FINISH){
                    state = STATE_IDLE;
                } else {
                    state = STATE_ERROR;
                }

                break;
            case STATE_ERROR:
                // will write to uart about error
                break;
            case STATE_HTTP_ERROR:
                // will write to uart about http error
                break;
            default:
                state = STATE_ERROR;
        }
    }
}

// hmi interrupt
// when data (the cube state) is received from the hmi device, put it in a queue and start the server task
void uart_intr_handle(void * pvParamters){
    // need to setup a queue to put data

    uart_event_t u_event;
    uint8_t data[128];

    for (;;)
    {
        if (xQueueReceive(uart_queue, &u_event, portMAX_DELAY))
        {
            if (u_event.type == UART_DATA)
            {
                int len = uart_read_bytes(
                        UART_NUM_1,
                        data,
                        u_event.size,
                        pdMS_TO_TICKS(100)
                );

                if (len > 0)
                {
                    for (int i = 0; i < len; i++)
                    {
                        if (data[i] == 0xFF) data[i] = '\0'; // nextion sends 3 0xff at the end
                    }

                    // create the app event signal
                    app_event_t event = {
                        .type = APP_EVENT_HMI_CUBE
                    };

                    strncpy(event.data, data, sizeof(event.data));
                    xQueueSend(xAppEventQueue, &event, portMAX_DELAY);
                }
            }
        }
    }
}

// server task
// send the data to the server. Wait for a response. when the response is received, insert it into a queue.
// start a timer

void vHttpTask( void * pvParameters)
{
    char data[55];
    char server_response[MAX_HTTP_RESPONSE];

    for (;;){
        // gets the cube state from the MessageBuffer
        size_t received = xMessageBufferReceive(xCubeState, data, sizeof(data), portMAX_DELAY);

        if (received > 0){
            server_response[received] = '\0';
            bool ok = send_post_request(data, server_response, sizeof(server_response)); //sends the post request

            app_event_t event = {0};
            
            // fills in data to event if response is good
            if (ok){
                event.type = APP_EVENT_HTTP_SUCCESS;
                strncpy(event.data, server_response, sizeof(event.data) -1);
            } else {
                event.type = APP_EVENT_HTTP_FAIL;
            }
            // sends the event to queue
            xQueueSend(xAppEventQueue, &event, portMAX_DELAY);
        }
    }
}

void vMotorTask(void * pvParameters)
{
    uint8_t moves[128];
    uint8_t cur_move[2];

    for (;;){
        size_t received = xMessageBufferReceive(xCubeSolution, moves, sizeof(moves), portMAX_DELAY);

        if (received > 0){
            app_event_t event = {0};
            moves[received] = '\0';

            for (int i = 0, j = 0; i <= strlen(moves); i++){
                if(moves[i] == ' ' || moves[i] == '\0'){
                    do_move(cur_move);
                    cur_move[0] = '\0';
                    cur_move[1] = '\0';
                    j = 0;
                } else {
                    cur_move[j++];
                }
            }
            
            event.type = APP_EVENT_CUBE_FINISH;
            xQueueSend(xAppEventQueue, &event, portMAX_DELAY);

        }
    }
}

void init_gpio()
{
    gpio_reset_pin(MOTOR_U_DIR);
    gpio_set_direction(MOTOR_U_DIR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_D_DIR);
    gpio_set_direction(MOTOR_D_DIR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_F_DIR);
    gpio_set_direction(MOTOR_F_DIR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_B_DIR);
    gpio_set_direction(MOTOR_B_DIR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_L_DIR);
    gpio_set_direction(MOTOR_L_DIR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_R_DIR);
    gpio_set_direction(MOTOR_R_DIR, GPIO_MODE_OUTPUT);

    gpio_reset_pin(MOTOR_U_STEP);
    gpio_set_direction(MOTOR_U_STEP, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_D_STEP);
    gpio_set_direction(MOTOR_D_STEP, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_F_STEP);
    gpio_set_direction(MOTOR_F_STEP, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_B_STEP);
    gpio_set_direction(MOTOR_B_STEP, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_R_STEP);
    gpio_set_direction(MOTOR_R_STEP, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_L_STEP);
    gpio_set_direction(MOTOR_L_STEP, GPIO_MODE_OUTPUT);

    gpio_reset_pin(MOTOR_ENABLE);
    gpio_set_direction(MOTOR_ENABLE, GPIO_MODE_OUTPUT);    
}


void app_main() {

    init_gpio();

    xCubeSolution = xMessageBufferCreate(128);
    xCubeState = xMessageBufferCreate(128);
    xAppEventQueue = xQueueCreate(1, sizeof(app_event_t));
    
    
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = 1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_1, &uart_config);

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 2048, 2048, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    


}