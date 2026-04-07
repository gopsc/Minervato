#pragma once
#include "Rectangle.h"
#include "Color.h"
#include <iostream>

extern unsigned char ttu[];
extern int ttunumber;
extern unsigned char zzhi[];
extern int zzhinumber;

namespace qing {
        /* 可绘制对象 */
	class Drawable {
		public:
		
			// 用于表示旋转方向
			// 使用static_cast<Rotate> (YOUR_NUM)
			enum class Rotate: char {
				R_0 = 0,
				R_90 = 1,
				R_180 = 2,
				R_270 = 3
			};

			/* 非法的旋转模式 */
			class InvalidRotateMode: std::runtime_error {
				public:
					InvalidRotateMode(): std::runtime_error("Invalid rotate mode.") {}
			};
			
			//////////////////////////////////////////////////////////
			/* 可绘制对象 构造函数 */
			// 传入对象的宽高、相对于父对象的位置、旋转方向、字体尺寸
			Drawable(int w, int h, int x, int y, int rotate, int fontsize, Color fontClr);

			//////////////////////////////////////////////////////////
			/* attr 属性区（提供访问器）*/
			virtual Rectangle get_size(); // 获取尺寸
			virtual Rectangle get_pos(); // 获取位置
			virtual void set_pos(Rectangle rect); // 设置位置
			virtual Color get_color(int x, int y); // 获取一个点的颜色
			virtual void set_color(Color &color); // 设置一个点的颜色
			void FontSize(int fontsize); // 设置字体大小
			int FontSize_get(); // 获取字号
			virtual Color& getFontsColor(); // 获取文字颜色
                                            // 为什么构造函数里没有？
			//////////////////////////////////////////////////////////
			/*
             * basic 基本 
             *
             * 具体的实现方式 -> 虚函数
             *
             * _修改建议：将基本方法移动到私有区
             */
			virtual Color _get(int x, int y) = 0; // 获取一个点的颜色
			virtual void _p(int x, int y, Color &clr) = 0; // 绘制一个点
			virtual void _up(int x, int y, int w, int h, int level) = 0; // 区域移动函数
			virtual void _down(int x, int y, int w, int h, int level) = 0;
			virtual void _left(int x, int y, int w, int h, int level) = 0;
			virtual void _right(int x, int y, int w, int h, int level) = 0;
			//virtual void _clear(int x, int y, int w, int h) = 0;  // 区域清理函数
                                                                    // 不知道为什么被注释掉了
			//////////////////////////////////////////////////////////
			/*
             * drawer 绘制 常规方法
             * 提前写好的绘制算法，供子类使用
             */
			virtual void point(int x, int y, Color &clr); // 绘制一个点
			virtual void line(int x1, int y1, int x2, int y2, Color &clr); // 绘制一条直线
			virtual void rectangle(int x, int y, int w, int h, Color &clr); // 绘制一个空心的矩形
			virtual void rectangle_fill(int x, int y, int w, int h, Color &clr); // 绘制一个实心的矩形（可以合并）
			virtual void up(int x, int y, int w, int h, int level); // 区域移动函数
			virtual void down(int x, int y, int w, int h, int level);
			virtual void left(int x, int y, int w, int h, int level);
			virtual void right(int x, int y, int w, int h, int level);
			virtual void flush(Drawable *n, bool border) = 0; // 将一个可绘制对象绘制到另一个
                                                              // _修改意见：我觉得应该实现它
			//////////////////////////////////////////////////////////
			/* printer 打印文字 */
			void drawvfont(int x, int y, unsigned short z, Color &clr);

			//////////////////////////////////////////////////////////
			/* test 图像测试 */
			virtual void test();
			virtual void test1();

		protected:
            //////////////////
			// window
			int w, h, x, y, rotate=0;
			// fonts lib
			int font_w, font_h;
            //////////////////
			unsigned short *chss = (unsigned short *)zzhi;
			unsigned char *vec = ttu;
			int zn;
			int vennum;
			unsigned char *drawvect(int x, int y, unsigned char *vect, int width, int high, Color &clr);
            //////////////////
			/* printer 打印机颜色 */
			Color clr;
	};
}
