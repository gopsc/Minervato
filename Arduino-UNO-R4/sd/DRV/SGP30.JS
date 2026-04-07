// SGP30 空气质量传感器


let SGP30_I2C_ADDR = 0x58;

// SGP30命令定义
let SGP30_CMD_INIT_AIR_QUALITY = 0x2003; // 初始化
let SGP30_CMD_MEASURE_AIR_QUALITY = 0x2008; // 测量
let SGP30_CMD_GET_BASELINE = 0x2015;
let SGP30_CMD_SET_BASELINE = 0x201E;
let SGP30_CMD_SET_HUMIDITY = 0x2061;
let SGP30_CMD_MEASURE_TEST = 0x2032;
let SGP30_CMD_GET_FEATURE_SET = 0x202F;
let SGP30_CMD_MEASURE_RAW = 0x2050;
let SGP30_CMD_GET_SERIAL_ID = 0x3682;

// 开始通信
let cmd0 = (SGP30_CMD_MEASURE_AIR_QUALITY >> 8) & 0xFF;
let cmd1 = SGP30_CMD_MEASURE_AIR_QUALITY & 0xFF;
i2c.trans(SGP30_I2C_ADDR);
i2c.write(cmd0);
i2c.write(cmd1);
i2c.close();
delay(12);

// 请求数据
i2c.request(SGP30_I2C_ADDR, 6);
let r0 = i2c.read();
let r1 = i2c.read();
let r2 = i2c.read();
let r3 = i2c.read();
let r4 = i2c.read();
let r5 = i2c.read();

// 验证结果
let res_eco2 = (r0 << 8) | r1;
let res_tvoc = (r3 << 8) | r4;

// 输出
let res = {
  msg: "OK",
  eCO2: res_eco2,
  TVOC: res_tvoc
};

res;