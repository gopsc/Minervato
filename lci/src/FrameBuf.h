#pragma once
#include "Color.h"
#include "Rectangle.h"
#include "Drawable.h"
#include <iostream>
#include <linux/fb.h>
namespace qing{
	/*
     * 帧缓冲区类型
     *
     * 这个类的绘制点方法过于复杂 _修改意见：尝试将复杂的部分移动到Color类中
     */
	class FrameBuf: public Drawable {
		public:

			FrameBuf(const char *path, int rotate, int fontsize, Color fontClr); /* 传入帧缓冲设备文件路径 */
			~FrameBuf();    /* 释放资源 */
			int get_bpp(); /* bits_per_pixel 像素比特 */
			virtual Color _get(int x, int y) override; /* 获取像素点颜色 */
			virtual void _p(int x, int y, Color &color) override; /* 绘制点 */
			virtual void _up(int x, int y, int w, int h, int level) override; /* 区域移动函数 */
			virtual void _down(int x, int y, int w, int h, int level) override;
			virtual void _left(int x, int y, int w, int h, int level) override;
			virtual void _right(int x, int y, int w, int h, int levle) override;
			virtual void flush(Drawable *d, bool border = false) override; /* 将一个可绘制对象复制到另一个可绘制对象上面 */
			void clear(); /* _修改意见：许把这个方法加到抽象父类中？
                                 有道理 */

			/* 未知标准的像素比特*/
			class UnknowBPP: std::runtime_error {
				public:
					UnknowBPP(): std::runtime_error("Unknow the number of Bits Per Pixel.") {}
			};

		private:

			int d = -1; /* 设备文件句柄 */
			struct fb_var_screeninfo var_info; /* 帧缓冲区屏幕可变信息 */
			struct fb_fix_screeninfo fix_info; /* 帧缓冲区屏幕固定信息 */
			int x_over = 0; //横轴过量
			int line_jump = 0; //行跳量
			char *buf = nullptr; /* mmap内存映射位置 */

			/**** 私有成员方法 ****/
			Color get16(__u32 x, __u32 y); /* 针对16像素比特（真彩色）的获取点颜色 */
			Color get32(__u32 x, __u32 y); /* 针对32位像素比特的获取点颜色 */
			void p32(__u32 x, __u32 y, Color &color); /* 针对32位像素比特的绘制点颜色 */
			void p16(__u32 x, __u32 y, Color &color); /* 针对16位像素比特的绘制点颜色 */

	};
}
