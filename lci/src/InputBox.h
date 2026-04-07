#pragma once
#include "Color.h"
#include "Area.h"
namespace qing {
	class InputBox: public Area {
		public:
			InputBox(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize, Color clr);
			virtual void print(char *utf8, int line);
	};
}
