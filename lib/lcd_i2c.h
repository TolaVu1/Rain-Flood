#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "hardware/i2c.h"

// Khởi tạo LCD, truyền i2c instance (ví dụ i2c0) và địa chỉ I2C
void lcd_init(i2c_inst_t *i2c, uint8_t addr);

// Xoá toàn bộ màn hình LCD
void lcd_clear(void);

// Đặt vị trí con trỏ (hàng, cột) - hàng: 0/1 (hoặc 0-3 nếu LCD 20x4), cột: 0-15 (hoặc 0-19)
void lcd_set_cursor(int row, int col);

// In chuỗi ra LCD từ vị trí hiện tại
void lcd_string(const char *str);

#endif // LCD_I2C_H