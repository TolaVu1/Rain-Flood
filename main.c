#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lib/lcd_i2c.h"
#include "hardware/regs/sio.h"
#include "hardware/structs/sio.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/structs/padsbank0.h"
#include "hardware/structs/iobank0.h"

// Định nghĩa các chân sử dụng
#define TRIG_PIN 2        // Chân trigger của cảm biến siêu âm
#define ECHO_PIN 3        // Chân echo của cảm biến siêu âm
#define KY024_PIN 0       // Chân tín hiệu của cảm biến mưa KY-024
#define BUZZER_PIN 6      // Chân điều khiển còi báo động

#define SDA_PIN 4         // Chân SDA cho I2C LCD
#define SCL_PIN 5         // Chân SCL cho I2C LCD
#define I2C_PORT i2c0
#define LCD_ADDRESS 0x27  // Địa chỉ I2C của LCD

#define MM_PER_TIP 0.1f
#define UPDATE_INTERVAL_US (3600LL * 1000000LL) // Cập nhật mỗi 1 giờ (tính bằng micro giây)

volatile uint32_t tip_count = 0; // Số lần cảm biến mưa kích hoạt
volatile absolute_time_t last_interrupt_time; // Thời điểm lần ngắt gần nhất

// Đọc trạng thái mức logic của chân GPIO (input) bằng thao tác bit trên thanh ghi SIO
static inline bool gpio_get_direct(uint pin) {
    return (sio_hw->gpio_in & (1u << pin)) != 0;
}

// Đưa chân GPIO lên mức cao (output)
static inline void gpio_set_high(uint pin) {
    sio_hw->gpio_set = (1u << pin);
}

// Đưa chân GPIO về mức thấp (output)
static inline void gpio_set_low(uint pin) {
    sio_hw->gpio_clr = (1u << pin);
}

// Thiết lập chiều của chân GPIO là output (kích hoạt OE - output enable)
static inline void gpio_set_dir_output(uint pin) {
    sio_hw->gpio_oe_set = (1u << pin);
}

// Thiết lập chiều của chân GPIO là input (tắt OE - output enable)
static inline void gpio_set_dir_input(uint pin) {
    sio_hw->gpio_oe_clr = (1u << pin);
}

// Khởi tạo chân GPIO: set chức năng là SIO (GPIO), không bật pull-up/pull-down
static inline void gpio_init_direct(uint pin) {
    // Đặt chức năng chân về GPIO (SIO)
    iobank0_hw->io[pin].ctrl = (iobank0_hw->io[pin].ctrl & ~0x1F) | GPIO_FUNC_SIO;
    // Không bật pull-up/pull-down ở đây
}

// Bật điện trở kéo lên (pull-up) cho chân GPIO
static inline void gpio_pull_up_direct(uint pin) {
    pads_bank0_hw->io[pin] |= PADS_BANK0_GPIO0_PUE_BITS;
}

// Đặt mức logic cho chân output (1 hoặc 0)
static inline void gpio_put_direct(uint pin, bool value) {
    if (value)
        gpio_set_high(pin);
    else
        gpio_set_low(pin);
}

// Đợi đến khi chân GPIO đạt mức logic mong muốn hoặc hết thời gian timeout
bool wait_for_level(uint pin, bool level, uint32_t timeout_us) {
    absolute_time_t start = get_absolute_time();
    while (gpio_get_direct(pin) != level) {
        if (absolute_time_diff_us(start, get_absolute_time()) > timeout_us)
            return false; // Hết thời gian chờ
    }
    return true;
}

// Khởi tạo cảm biến siêu âm: TRIG là output, ECHO là input
void init_ultrasonic() {
    gpio_init_direct(TRIG_PIN);
    gpio_set_dir_output(TRIG_PIN);
    gpio_set_low(TRIG_PIN);

    gpio_init_direct(ECHO_PIN);
    gpio_set_dir_input(ECHO_PIN);
}

// Đo khoảng cách (cm) bằng cảm biến siêu âm HC-SR04
float read_distance_cm() {
    gpio_set_low(TRIG_PIN);
    sleep_us(2);
    gpio_set_high(TRIG_PIN);
    sleep_us(10);
    gpio_set_low(TRIG_PIN);

    // Đợi ECHO lên mức cao (bắt đầu nhận sóng phản xạ)
    if (!wait_for_level(ECHO_PIN, 1, 30000)) return -1;

    absolute_time_t start = get_absolute_time();
    // Đợi ECHO về mức thấp (kết thúc nhận sóng phản xạ)
    if (!wait_for_level(ECHO_PIN, 0, 30000)) return -1;
    absolute_time_t end = get_absolute_time();

    int64_t duration = absolute_time_diff_us(start, end);
    // Công thức tính khoảng cách: duration / 58.0f (áp dụng cho HC-SR04)
    return duration / 58.0f;
}

// Hàm callback xử lý ngắt cạnh xuống (falling edge) từ cảm biến mưa KY-024
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == KY024_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        absolute_time_t now = get_absolute_time();
        // Lọc nhiễu: chỉ chấp nhận 1 xung mỗi 100ms
        if (absolute_time_diff_us(last_interrupt_time, now) > 100000) {
            tip_count++; // Tăng số lần mưa
            last_interrupt_time = now;
        }
    }
}

// Hiển thị thông tin lên LCD
void show_lcd(float total_rain, float distance) {
    char line1[17], line2[17];
    snprintf(line1, sizeof(line1), "Rain: %.1f mm", total_rain);
    if (distance > 0)
        snprintf(line2, sizeof(line2), "Level: %.1f cm", distance);
    else
        snprintf(line2, sizeof(line2), "Level: --.- cm");

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_string(line1);
    lcd_set_cursor(1, 0);
    lcd_string(line2);
}

// Kiểm tra điều kiện báo động và kích hoạt còi
void check_alarm(float rain, float level_cm) {
    bool alert = false;
    // Nếu lượng mưa > 10mm hoặc mực nước < 20cm (nhưng > 0) thì báo động
    if (rain > 10.0f || (level_cm > 0 && level_cm < 20.0f)) {
        alert = true;
    }
    gpio_put_direct(BUZZER_PIN, alert ? 1 : 0);
}

int main() {
    stdio_init_all();

    // Khởi tạo LCD I2C, giữ nguyên hàm SDK vì phức tạp
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    lcd_init(I2C_PORT, LCD_ADDRESS);
    lcd_clear();

    // Khởi tạo cảm biến mưa KY-024
    gpio_init_direct(KY024_PIN);
    gpio_set_dir_input(KY024_PIN);
    gpio_pull_up_direct(KY024_PIN); // Kéo lên nội bộ cho chân đầu vào
    // Đăng ký hàm callback ngắt với SDK (phần này vẫn dùng hàm SDK)
    gpio_set_irq_enabled_with_callback(KY024_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Khởi tạo còi báo động (buzzer)
    gpio_init_direct(BUZZER_PIN);
    gpio_set_dir_output(BUZZER_PIN);
    gpio_set_low(BUZZER_PIN); // Tắt còi lúc đầu

    // Khởi tạo cảm biến siêu âm
    init_ultrasonic();

    last_interrupt_time = get_absolute_time();
    absolute_time_t last_hour_time = get_absolute_time();
    uint32_t last_tip_count = 0;

    while (1) {
        sleep_ms(1000); // Chờ 1 giây mỗi lần lặp

        float distance = read_distance_cm();

        absolute_time_t now = get_absolute_time();
        int64_t elapsed_us = absolute_time_diff_us(last_hour_time, now);
        // Nếu đã đủ thời gian cập nhật (1 giờ)
        if (elapsed_us >= UPDATE_INTERVAL_US) {
            last_tip_count = tip_count;
            last_hour_time = now;
        }

        float total_rain = tip_count * MM_PER_TIP; // Tổng lượng mưa đã ghi nhận
        show_lcd(total_rain, distance);
        check_alarm(total_rain, distance);
    }

    return 0;
}