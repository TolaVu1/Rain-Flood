#include "lcd_i2c.h"
#include "pico/stdlib.h"

static i2c_inst_t *lcd_i2c = NULL;
static uint8_t lcd_addr = 0x27;
static uint8_t lcd_backlight = 0x08;

#define LCD_CHR  1 // Gửi data
#define LCD_CMD  0 // Gửi lệnh

#define LCD_LINE_1 0x80 // Địa chỉ dòng 1
#define LCD_LINE_2 0xC0 // Địa chỉ dòng 2
#define LCD_LINE_3 0x94 // Địa chỉ dòng 3 (nếu 20x4)
#define LCD_LINE_4 0xD4 // Địa chỉ dòng 4 (nếu 20x4)

// Prototypes
static void lcd_byte(uint8_t bits, uint8_t mode);
static void lcd_toggle_enable(uint8_t bits);

void lcd_init(i2c_inst_t *i2c, uint8_t addr) {
    lcd_i2c = i2c;
    lcd_addr = addr;

    sleep_ms(50); // chờ LCD sẵn sàng

    lcd_byte(0x33, LCD_CMD); // khởi tạo
    lcd_byte(0x32, LCD_CMD); // khởi tạo
    lcd_byte(0x06, LCD_CMD); // di chuyển con trỏ sang phải
    lcd_byte(0x0C, LCD_CMD); // bật màn hình, tắt con trỏ
    lcd_byte(0x28, LCD_CMD); // 2 dòng, 5x8 font
    lcd_byte(0x01, LCD_CMD); // clear màn hình
    sleep_ms(5);
}

void lcd_clear(void) {
    lcd_byte(0x01, LCD_CMD); // clear màn hình
    sleep_ms(2);
}

void lcd_set_cursor(int row, int col) {
    uint8_t row_offsets[] = {LCD_LINE_1, LCD_LINE_2, LCD_LINE_3, LCD_LINE_4};
    if (row < 0) row = 0;
    if (row > 3) row = 3;
    lcd_byte(row_offsets[row] + col, LCD_CMD);
}

void lcd_string(const char *str) {
    while (*str) {
        lcd_byte(*str++, LCD_CHR);
    }
}

// --- Low level --- //
static void lcd_byte(uint8_t bits, uint8_t mode) {
    uint8_t high = mode | (bits & 0xF0) | lcd_backlight;
    uint8_t low  = mode | ((bits << 4) & 0xF0) | lcd_backlight;

    uint8_t buf[1];

    buf[0] = high;
    i2c_write_blocking(lcd_i2c, lcd_addr, buf, 1, false);
    lcd_toggle_enable(high);

    buf[0] = low;
    i2c_write_blocking(lcd_i2c, lcd_addr, buf, 1, false);
    lcd_toggle_enable(low);
}

static void lcd_toggle_enable(uint8_t bits) {
    uint8_t buf[1];

    buf[0] = bits | 0x04; // Enable bit
    i2c_write_blocking(lcd_i2c, lcd_addr, buf, 1, false);
    sleep_us(500);

    buf[0] = bits & ~0x04; // Disable Enable bit
    i2c_write_blocking(lcd_i2c, lcd_addr, buf, 1, false);
    sleep_us(500);
}