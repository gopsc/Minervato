
## 什么是LINUX中文界面（LCI模块）？
简单读写帧缓冲内存，使得部分没有图形界面的LINUX设备可以显示中文。

### 使用方法

``` bash
sudo apt update
sudo apt install g++ make
cd lci
make
sudo make install
sudo lci -e {YOUR_CMD}
```

(!) 开发不完全，新手慎用
(!) Ctrl + Alt + F1 回到图形界面
(!) 需要在内部程序运行结束后才按下ESC
