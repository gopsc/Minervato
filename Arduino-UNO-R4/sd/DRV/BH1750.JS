
// 测量模式定义
let CONTINUOUS_HIGH_RES   = 0x10;  // 连续高分辨率(1lx)
let CONTINUOUS_HIGH_RES_2 = 0x11;  // 连续高分辨率2(0.5lx)
let CONTINUOUS_LOW_RES    = 0x13;  // 连续低分辨率(4lx)
let ONE_TIME_HIGH_RES     = 0x20;  // 单次高分辨率(1lx)
let ONE_TIME_HIGH_RES_2   = 0x21;  // 单次高分辨率2(0.5lx)
let ONE_TIME_LOW_RES      = 0x23;  // 单次低分辨率(4lx)

// 全局变量(存储传感器状态)
let bh1750_addr = 0x23;                  // I2C地址(默认0x23，可改为0x5C)
let current_mode = ONE_TIME_LOW_RES;  // 当前测量模式
let measurement_delay = 120;             // 测量延迟时间(ms)

// 上电命令
i2c.trans(bh1750_addr);
i2c.write(0x01); 
i2c.close();

// 重置命令
i2c.trans(bh1750_addr);
i2c.write(0x07); 
i2c.close();

// 根据模式更新测量延迟
if (current_mode === CONTINUOUS_HIGH_RES || current_mode === CONTINUOUS_HIGH_RES_2 || current_mode === ONE_TIME_HIGH_RES || current_mode === ONE_TIME_HIGH_RES_2) {
    measurement_delay = 120;
}
else if (current_mode === CONTINUOUS_LOW_RES || current_mode === ONE_TIME_LOW_RES) {
    measurement_delay = 16;
}
else {
    measurement_delay = 120;
}

// 单次模式需要每次发送测量命令
if (current_mode >= ONE_TIME_HIGH_RES) {
    i2c.trans(bh1750_addr);
    i2c.write(current_mode);
    i2c.close();
}

delay(measurement_delay);  // 等待测量完成

// 读取2字节数据
i2c.request(bh1750_addr, 2);
let dd0 = i2c.read();
let dd1 = i2c.read();


// 组合原始数据
let raw_data = (dd0 << 8) | dd1;

let result = 0;
// 计算lux值(根据模式选择转换公式)
if (current_mode === CONTINUOUS_HIGH_RES_2 || current_mode === ONE_TIME_HIGH_RES_2) {
    result = raw_data / 2.4;
} else {
    result = raw_data / 1.2;
}

result;