/*
 * 我们之间的感情就和春天能够穿衬衫的时间一样稀少。
 * --------
 * 有些昆虫往往只能活一个春天，那个春天就是它们拥有的全部了。
 */
#pragma once
#include "Color.h"
#include "Drawable.h"
namespace qing {
	/* Phase Plotting Method 相图法（？）*/
	class Area: public Drawable {
		public:
			Area(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize, Color fontClr);
			Color _get(int x, int y) override; // 获取区域内的一个点的颜色
			void _p(int x, int y, Color &clr) override; // 绘制区域内的一个点
			void _up(int x, int y, int w, int h, int level) override; // 区域移动方法
			void _down(int x, int y, int w, int h, int level) override;
			void _left(int x, int y, int w, int h, int level) override;
			void _right(int x, int y, int w, int h, int level) override;
			void flush(Drawable *n, bool border = false) override; // 将一个区域整个绘制到另一个
		protected:
			/*
			 * “区域”总是指另一个可绘制对象的一部分，
			 *     它是直接在这个可绘制对象上绘制的。
			 *         这是一种比较巧妙的方式。
			 */
			Drawable *d;
	};
}
