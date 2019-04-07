# CoffeeBeansLife of Arduino

## Use Sensor & Describe

 sensor | describe
 -------|---------
  HC-05 | 藍芽模組
  MAX6675 | K型電熱偶感溫模組
  MLX9614 | 非接觸紅外線感溫模組
  Buzzer | |
  LED | |
  繼電器(110v, 10A) | |

## Arduino pro mini mapping sensor pin

![Alt text](../assets/layout.png "電路佈線圖")

>MAX6675 晶片腳位如下圖

![Alt text](../assets/MAX6675-K-Thermocouple-Module.png "電路佈線圖")

### 繼電器

Arduino pro mini | 繼電器
---------------- | ------
 D12 | IN

### LED

Arduino pro mini | LED
---------------- | ------
 D11 | Signal

### Buzzer(蜂鳴器)

Arduino pro mini | Buzzer
---------------- | ------
 D13 | Signal

### MAX6675

Arduino pro mini | MAX6675
---------------- | ------
 D4 | SO
 D5 | CS
 D6 | SCK

### HC-05

Arduino pro mini | HC-05
---------------- | ------
 D7 | RX
 D8 | TX
 D9 | STATE

### MLX9614

Arduino pro mini | MLX9614
---------------- | ------
 D2 | SCL
 D3 | SDA