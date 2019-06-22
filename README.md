# CoffeeBeansLife of Arduino

## 流程圖

![Alt text](../assets/program_flow_chart.png "program flow chart")

## 使用相關硬體與元件

 Item | describe |
 -------|---------
  Arduino misco | 微控制器主板 |
  HC-05 | 藍芽模組
  MAX6675 | K型電熱偶感溫模組 |
  MLX9614(5V) | 非接觸紅外線感溫模組 |
  Buzzer | 蜂鳴器 |
  LED RED | |
  LED GREEN | |
   SRD(5v) | 電磁式 繼電器 |

## 電路圖

**電路接線**

![Alt text](./assets/circuit_diagram_bb.png "電路接線圖")

**電路架構圖**

![Alt text](./assets/circuit_diagram_fframework.png "電路架構圖")

>MAX6675 晶片腳位如下圖

![Alt text](./assets/MAX6675-K-Thermocouple-Module.png "電路佈線圖")

>MLX90614 晶片腳位如下圖
![Alt text](./assets/mlx90614.png "電路佈線圖")

## Arduino misco與相關傳感器連接腳位

   Arduino Pin   |   Sensor Pin   |
---------------- | -------------- |
 D2 | MLX90614_SDA |
 D3 | MLX90614_SCL |
 5v | MLX90614_Vdd |
 GND | MLX90614_Vss |
 D4 | SRD_IN |
 5v | SRD_VCC |
 GND | SRD_GND |
 D5 | HC-05_SCK |
 TX | HC-05_RX |
 RX | HC-05_TX |
 5V | HC-05_VVV |
 GND | HC-05_GND |
 D8 | MAX6675_SCK |
 D9 | MAX6675_CS |
 D10 | MAX6675_SO |
 5V | MAX6675_VCC |
 GND | MAX6675_GND |
 D11 | LED RED _+|
 GND | LED RED_- |
 D12 | LED GREEN_+|
 GND | LED GREEN_- |
