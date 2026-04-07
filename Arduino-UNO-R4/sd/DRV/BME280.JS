// BME280 -> 温湿度、大气压传感器


let BME280_ADDR = 0x76;
//let BME280_ADDR = 0x77;

let BME280_REG_ID = 0xD0;
let BME280_REG_RESET = 0xE0;
let BME280_REG_CTRL_HUM = 0xF2;
let BME280_REG_CTRL_MEAS = 0xF4;
let BME280_REG_CONFIG = 0xF5;
let BME280_REG_TEMP_MSB = 0xFA;
let BME280_REG_PRESS_MSB = 0xF7;
let BME280_REG_CALIB_START = 0x88;
let BME280_REG_CALIB_HUM = 0xE1;

// 获取ID
i2c.trans(BME280_ADDR);
i2c.write(BME280_REG_ID);
i2c.close();
i2c.request(BME280_ADDR, 1);
let id = i2c.read();

// 判断BME是否通信正常
let res = {};
if (id === 0x60) {

  // 软件复位
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_RESET);
  i2c.close();

  i2c.trans(BME280_ADDR);
  i2c.write(0xB6);
  i2c.close();
  delay(10);

  // 读取温度压力校准参数 (0x88-0x9F)
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_CALIB_START);
  i2c.close();
  i2c.request(BME280_ADDR, 26);
  let d0  = i2c.read();
  let d1  = i2c.read();
  let d2  = i2c.read();
  let d3  = i2c.read();
  let d4  = i2c.read();
  let d5  = i2c.read();
  let d6  = i2c.read();
  let d7  = i2c.read();
  let d8  = i2c.read();
  let d9  = i2c.read();
  let d10 = i2c.read();
  let d11 = i2c.read();
  let d12 = i2c.read();
  let d13 = i2c.read();
  let d14 = i2c.read();
  let d15 = i2c.read();
  let d16 = i2c.read();
  let d17 = i2c.read();
  let d18 = i2c.read();
  let d19 = i2c.read();
  let d20 = i2c.read();
  let d21 = i2c.read();
  let d22 = i2c.read();
  let d23 = i2c.read();
  let d24 = i2c.read();
  let d25 = i2c.read();

  // 计算校准参数
  let T1 = (d1 << 8) | d0;
  let T2 = (d3 << 8) | d2;
  let T3 = (d5 << 8) | d4;

  let P1 = (d7  << 8) | d6;
  let P2 = (d9  << 8) | d8;
  let P3 = (d11 << 8) | d10;
  let P4 = (d13 << 8) | d12;
  let P5 = (d15 << 8) | d13;
  let P6 = (d17 << 8) | d16;
  let P7 = (d19 << 8) | d18;
  let P8 = (d21 << 8) | d20;
  let P9 = (d23 << 8) | d22;

  let H1 = d25;

  // 读取湿度校准参数 (0xE1-0xE7)
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_CALIB_HUM);
  i2c.close();
  i2c.request(BME280_ADDR, 7);
  let dd0 = i2c.read();
  let dd1 = i2c.read();
  let dd2 = i2c.read();
  let dd3 = i2c.read();
  let dd4 = i2c.read();
  let dd5 = i2c.read();
  let dd6 = i2c.read();

  let H2 = (dd1 << 8) | dd0;
  let H3 = dd2;
  let H4 = (dd3 << 4) | (dd4 & 0x0F);
  let H5 = (dd5 << 4) | (dd4 >> 4);
  let H6 = dd6;

  // 配置湿度测量
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_CTRL_HUM);
  i2c.write(0x01);
  i2c.close();
  delay(10);

  // 配置测量控制寄存器：温度1次采样，压力1次采样，正常模式
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_CTRL_MEAS);
  i2c.write(0x25);
  i2c.close();
  delay(10);

  // 配置寄存器：设置待机时间和滤波器
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_CONFIG);
  i2c.write(0x00);
  i2c.close();
  delay(10);


  // 测量，必须使用PRESS*
  i2c.trans(BME280_ADDR);
  i2c.write(BME280_REG_PRESS_MSB);
  i2c.close();
  i2c.request(BME280_ADDR, 8);
  let r0 = i2c.read();
  let r1 = i2c.read();
  let r2 = i2c.read();
  let r3 = i2c.read();
  let r4 = i2c.read();
  let r5 = i2c.read();
  let r6 = i2c.read();
  let r7 = i2c.read();


  // 组合压力数据 (20位)
  let adc_P = (r0 << 12) | (r1 << 4) | ((r2 >> 4) & 0x0F);

  // 组合温度数据 (20位)
  let adc_T = (r3 << 12) | (r4 << 4) | ((r5 >> 4) & 0x0F);
  
  // 组合湿度数据 (16位)
  let adc_H = (r6 << 8) | r7;

  

  // 补偿温度值
  let var1_T = ((((adc_T >> 3) - (T1 << 1))) * T2) >> 11;
  let var2_T = (((((adc_T >> 4) - T1) * ((adc_T >> 4) - T1)) >> 12) * T3) >> 14;
  let t_fine = var1_T + var2_T;
  let temp = (t_fine * 5 + 128) >> 8;
  temp = temp / 100.0;

  
  // 补偿压力值（溢出）
  //let var1_P = t_fine - 128000;
  //let var2_P = var1_P * var1_P * P6;
  //var2_P = var2_P + ((var1_P * P5) << 17);
  //var2_P = var2_P + (P4 << 35);
  //var1_P = ((var1_P * var1_P * P3) >> 8) + ((var1_P * P2) << 12);
  //var1_P = (((1 << 47) + var1_P)) * P1 >> 33;
  
  //let pres = 0;
  //if (var1_P !== 0) {
  //  pres = 1048576 - adc_P;
  //  pres = (((pres << 31) - var2_P) * 3125) / var1_P;
  //  var1_P = (P9 * (pres >> 13) * (pres >> 13)) >> 25;
  //  var2_P = (P8 * pres) >> 19;
  
  //  pres = ((pres + var1_P + var2_P) >> 8) + (P7 << 4);
  //  pres = pres / 25600.0; // 转换为hPa
  //}

  // 补偿湿度值
  let var1_H = (t_fine - 76800);
  var1_H = (((((adc_H << 14) - (H4 << 20) - (H5 * var1_H)) + 
          16384) >> 15) * (((((((var1_H * H6) >> 10) * 
          (((var1_H * H3) >> 11) + 32768)) >> 10) + 
          2097152) * H2 + 8192) >> 14));
  var1_H = (var1_H - (((((var1_H >> 15) * (var1_H >> 15)) >> 7) * H1) >> 4));
  if (var1_H < 0) {
    var1_H =  0;
  }
  if (var1_H > 419430400) {
    var1_H =  419430400;
  }

  let humi = (var1_H >> 12) / 1024.0;

  res = {msg: "OK", temperature: temp, humidity: humi };

}

else {
  res = {msg: "ERR"};
}

res;