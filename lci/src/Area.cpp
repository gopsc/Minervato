#include "utf8.h"
#include "ansi.h"
#include "Drawable.h"
#include "Area.h"
#include <cstdlib>
namespace qing {

	/*
	 * 构造函数，除了可绘制对象的组成元素之外。
	 *     还需要传入一个可绘制对象的指针。
	 * --------
	 *   区域对象持有这个可绘制对象的地址
	 *     表示它是这个可绘制对象的一个区域
	 */
	Area::Area(Drawable *d, int w, int h, int x, int y, int rotate = 0, int fontsize=18, Color fontClr = white):
             Drawable(w, h, x, y, rotate, fontsize, fontClr) {
		this->d = d;
	}

	/*
	 * 获取区域内一个点的颜色值
	 *
	 * 20250922(qing): 增加了边界检查
	 *
	 * _修改意见：抛出异常
	 */
	Color Area::_get(int x, int y) {
		if (x < 0 || x >= w || y < 0 || y >= h) return Color(0, 0, 0);
		return d->get_color(this->x + x, this->y + y);
	}

	/*
	 * 在区域内绘制一个点
	 *
	 * 20250922(qing): 增加了边界检查
	 */
	void Area::_p(int x, int y, Color &clr) {
		if (x < 0 || x >= w || y < 0 || y >= h) return;
		d->point(this->x + x, this->y + y, clr);
	}

	/*
	 * 区域向上移动
     *
     * 20250922(qing): 增加了边界检查
     *
     * 虽然d->up()可能拥有边界检查，但是我们这里还是再检查一遍
     *
     * level还没有检查，不太清楚机制
     *
	 */
	void Area::_up(int x, int y, int w, int h, int level) {
        if ( x<0 || x >= this->w || y<0 || y >= this->h
                || (x+w) < 0 || (x+w) >= this->w || (y+h) < 0 || (y+h) >= this->h )
            return;
        //if (level > ?) return;
		d->up(this->x + x, this->y + y, w, h, level);
	}

    /* 区域向下移动 */
	void Area::_down(int x, int y, int w, int h, int level) {
        if ( x<0 || x >= this->w || y<0 || y >= this->h
                || (x+w) < 0 || (x+w) >= this->w || (y+h) < 0 || (y+h) >= this->h )
            return;
        //if (level > ?) return;
		d->down(this->x + x, this->y + y, w, h, level);
	}

    /* 区域向左移动 */
	void Area::_left(int x, int y, int w, int h, int level) {
        if ( x < 0 || x >= this->w || y < 0 || y >= this->h
                || (x+w) < 0 || (x+w) >= this->w || (y+h) < 0 || (y+h) >= this->h )
            return;
        //if (level > ?) return;
		d->left(this->x + x, this->y + y, w, h, level);
	}

    /* 区域向右移动 */
	void Area::_right(int x, int y, int w, int h, int level) {
        if ( x < 0 || x >= this->w || y < 0 || y >= this->h
                || (x+w) < 0 || (x+w) >= this->w || (y+h) < 0 || (y+h) >= this->h )
            return;
        //if (level > ?) return;
		d->right(this->x + x, this->y + y, w, h, level);
	}

    /*
     * 将一个可绘制对象的内容复制到另一个可绘制对象上
     *
     * 如果绘制对象比被绘制对象大，或者越界了，则会发生未定义事件
     */
	void Area::flush(Drawable *n, bool border) {
		Rectangle size = n->get_size();
		Rectangle pos = n->get_pos();
		Color &clr = n->getFontsColor();
		if (border)
			rectangle(pos.w-1, pos.h-1, size.w+1, size.h+1, clr);
		Area *p = dynamic_cast<Area*>(n); // 向下转换（Downcasting）
		if (p != nullptr) return; // 转换成功，立即返回？？
                                  // _修改意见：我觉得应该修改
		// too slow 太慢了
		for (int x = 0; x < size.w; ++x) {
			for (int y = 0; y < size.h; ++y) {
				Color color = n->get_color(x, y);
				point(pos.w + x, pos.h + y, color);
			}
		}
	}
}
