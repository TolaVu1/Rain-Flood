# Dự Án Đo Mưa Và Mực Nước Sử Dụng Custom Board (RP2040)

## Mô tả dự án

Đây là dự án mô hình thiết bị đo mưa và mực nước sử dụng vi điều khiển Raspberry RP2040 lập trình theo phong cách bare-metal, thao tác trực tiếp với thanh ghi phần cứng để tối ưu tốc độ và tài nguyên hệ thống. Dự án sử dụng cảm biến mưa kiểu tipping bucket (KY-024), cảm biến siêu âm đo mực nước (HC-SR04) và màn hình LCD giao tiếp I2C để hiển thị thông tin lượng mưa, mực nước theo thời gian thực. Có tích hợp còi báo động khi vượt ngưỡng cảnh báo.

## Tính năng chính

- **Đo lượng mưa** sử dụng tipping bucket (KY-024).
- **Đo mực nước** bằng cảm biến siêu âm (HY-SRF05).
- **Hiển thị thông tin** lên LCD I2C (16x2).
- **Còi cảnh báo** khi lượng mưa/mực nước vượt ngưỡng.
- 
## Sơ đồ phần cứng

- **Vi điều khiển:** RP2040 (thạch anh ngoài 12MHz)
- **Cảm biến mưa:** KY-024 (tipping bucket)
- **Cảm biến mực nước:** HY-SRF05 (ultrasonic sensor)
- **Màn hình:** LCD 16x2 I2C (PCF8574)
- **Còi báo động:** Buzzer

| Thiết bị         | Chân kết nối Pico |
|------------------|------------------|
| KY-024           | GPIO0            |
| HY-SR0           | GPIO2, GPIO3     |
| Buzzer           | GPIO6            |
| LCD I2C          | GPIO4, GPIO5     |

## Kiến trúc phần mềm

- **Toàn bộ IO GPIO truy cập trực tiếp thanh ghi**: sử dụng toán tử bit thao tác trên các thanh ghi SIO, IO_BANK0, PADS_BANK0.
- **Đo mưa bằng ngắt cạnh xuống**: mỗi lần tipping bucket nghiêng sẽ tạo một ngắt làm tăng biến đếm tip_count.
- **Đo mực nước bằng siêu âm**: phát xung qua chân TRIG, đo thời gian phản hồi ở chân ECHO, chuyển đổi ra đơn vị cm.
- **Hiển thị LCD**: cập nhật định kỳ thông tin mưa và mực nước.
- **Báo động**: nếu lượng mưa trên 10mm hoặc mức nước dưới 20cm thì kích hoạt còi.
- **Không dùng hàm IO cao cấp của SDK** (trừ phần ngắt và I2C).

## Yêu cầu thư viện/phần mềm

- Pico SDK
- Thư viện LCD I2C (`lib/lcd_i2c.h`)
- CMake/ninja
- ARM GNU

## Biên dịch và nạp chương trình

1. Cài đặt Pico SDK theo hướng dẫn chính thức.
2. Thêm thư viện LCD I2C vào dự án.
3. Biên dịch bằng CMake:
   ```sh
   mkdir build
   cd build
   cmake ..
   make
   ```
4. Nạp file `.uf2` vào Pico.

## Ảnh minh họa

![image](https://github.com/user-attachments/assets/d2dd53af-9176-4844-ae12-6c4bf0b1a1fa)

![image](https://github.com/user-attachments/assets/696b361c-da31-4214-ae7b-42623d81aec2)


---

## Tác giả

- Chủ dự án: **Trần Thanh Vũ và cộng sự** 

---
