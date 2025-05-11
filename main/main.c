#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

// --- TLV Type IDs (example, adjust to your protocol) ---
#define TLV_TYPE_RPM            0x01
#define TLV_TYPE_BOOST_PRESSURE 0x02
#define TLV_TYPE_OIL_PRESSURE   0x03
#define TLV_TYPE_FUEL_LEVEL     0x04
#define TLV_TYPE_SPEED          0x05
#define TLV_TYPE_STATUS_FLAGS   0x06
#define TLV_TYPE_STEERING_ANGLE 0x07
#define TLV_TYPE_BRAKE_PRESSURE 0x08
#define TLV_TYPE_THROTTLE_POS   0x09
#define TLV_TYPE_GEAR_POS       0x0A

// --- Serial port config ---
#define UART_NUM UART_NUM_0
#define UART_TX_PIN 1
#define UART_RX_PIN 3
#define SERIAL_BAUD 115200

static const char *TAG = "mock_data";

// --- Helper: Send TLV packet ---
void send_tlv(uint8_t type, uint8_t len, const uint8_t* value) {
    uart_write_bytes(UART_NUM, (const char*)&type, 1);
    uart_write_bytes(UART_NUM, (const char*)&len, 1);
    uart_write_bytes(UART_NUM, (const char*)value, len);
}

// --- Helper: Send uint16_t as TLV ---
void send_tlv_u16(uint8_t type, uint16_t value) {
    uint8_t buf[2];
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
    send_tlv(type, 2, buf);
}

// --- Helper: Send int16_t as TLV ---
void send_tlv_i16(uint8_t type, int16_t value) {
    uint8_t buf[2];
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
    send_tlv(type, 2, buf);
}

// --- Helper: Send uint8_t as TLV ---
void send_tlv_u8(uint8_t type, uint8_t value) {
    send_tlv(type, 1, &value);
}

void app_main(void)
{
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = SERIAL_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "Mock data generator started");

    while (1) {
        float t = fmod((float)esp_timer_get_time() / 1000000.0f, 60.0f); // seconds, loop every 60s

        uint16_t rpm = 1000, speed = 0, oil_pressure = 1600, boost_mbar = 800, brake_pressure = 0, fuel_level = 3500;
        uint8_t throttle_pos = 0, gear_pos = 1, status_flags = 0;
        int16_t steering_angle = 0;

        if (t < 10) { // Idle
            rpm = 1000 + (uint16_t)(sin(t) * 100);
            speed = 0;
            throttle_pos = 2 + (uint8_t)(sin(t) * 2);
            gear_pos = 1;
            boost_mbar = 800;
            oil_pressure = 1600 + (uint16_t)(sin(t) * 50);
        } else if (t < 20) { // Start & Acceleration
            float f = (t - 10) / 10.0f;
            rpm = 1000 + (uint16_t)(f * 3000);
            speed = (uint16_t)(f * 40);
            throttle_pos = 10 + (uint8_t)(f * 50);
            gear_pos = 1;
            boost_mbar = 800 + (uint16_t)(f * 400);
            oil_pressure = 1700 + (uint16_t)(f * 500);
        } else if (t < 22) { // 1st to 2nd Gear Shift
            float f = (t - 20) / 2.0f;
            rpm = 4000 - (uint16_t)(f * 2000);
            speed = 40 + (uint16_t)(f * 5);
            throttle_pos = 60 - (uint8_t)(f * 30);
            gear_pos = 2;
            boost_mbar = 1200 - (uint16_t)(f * 200);
        } else if (t < 35) { // Cruising
            float f = (t - 22) / 13.0f;
            rpm = 2000 + (uint16_t)(sin(t) * 500);
            speed = 45 + (uint16_t)(sin(t * 0.5f) * 15);
            throttle_pos = 20 + (uint8_t)(sin(t) * 20);
            gear_pos = 2;
            boost_mbar = 900 + (uint16_t)(sin(t) * 100);
        } else if (t < 45) { // Acceleration & Gear Upshift
            float f = (t - 35) / 10.0f;
            rpm = 2500 + (uint16_t)(f * 3500);
            speed = 60 + (uint16_t)(f * 60);
            throttle_pos = 40 + (uint8_t)(f * 50);
            gear_pos = (t < 40) ? 2 : ((t < 43) ? 3 : 4);
            boost_mbar = 1100 + (uint16_t)(f * 700);
        } else if (t < 50) { // Braking
            float f = (t - 45) / 5.0f;
            rpm = 6000 - (uint16_t)(f * 4500);
            speed = 120 - (uint16_t)(f * 90);
            throttle_pos = 0;
            brake_pressure = (uint16_t)(f * 1200);
            gear_pos = (t < 48) ? 4 : 2;
            boost_mbar = 1000;
        } else { // Idle again
            rpm = 1000 + (uint16_t)(sin(t) * 100);
            speed = 0;
            throttle_pos = 2 + (uint8_t)(sin(t) * 2);
            gear_pos = 1;
            boost_mbar = 800;
            oil_pressure = 1600 + (uint16_t)(sin(t) * 50);
            brake_pressure = 0;
        }

        // Simulate fuel level (slowly decreases, resets every 60s)
        fuel_level = 3500 - (uint16_t)(t * 50);

        // Simulate steering angle (gentle oscillation)
        steering_angle = (int16_t)(sin(t * 0.5f) * 300);

        // --- Send as TLV packets ---
        send_tlv_u16(TLV_TYPE_RPM,            rpm);
        send_tlv_u16(TLV_TYPE_BOOST_PRESSURE, boost_mbar);
        send_tlv_u16(TLV_TYPE_OIL_PRESSURE,   oil_pressure);
        send_tlv_u16(TLV_TYPE_FUEL_LEVEL,     fuel_level);
        send_tlv_u16(TLV_TYPE_SPEED,          speed);
        send_tlv_u8 (TLV_TYPE_STATUS_FLAGS,   status_flags);
        send_tlv_i16(TLV_TYPE_STEERING_ANGLE, steering_angle);
        send_tlv_u16(TLV_TYPE_BRAKE_PRESSURE, brake_pressure);
        send_tlv_u8 (TLV_TYPE_THROTTLE_POS,   throttle_pos);
        send_tlv_u8 (TLV_TYPE_GEAR_POS,       gear_pos);

        vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz
    }
}
