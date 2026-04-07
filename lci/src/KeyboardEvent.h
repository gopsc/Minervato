#pragma once
#include <iostream>
#include <string>
#include <map>
#include <linux/input.h>
#define MAX 64
namespace qing {
	/* 键盘事件类 */
	class KeyboardEvent{
		public:
			class NoEvent: std::runtime_error{
				public:
					NoEvent(std::string s): std::runtime_error(s){}
			};
			KeyboardEvent();
			~KeyboardEvent();
			struct input_event get();
		private:
			std::map<std::string, int> pool;
			static bool is_event_with_number(const char *filename);
			static void set_noblocking(int d);
			int maxd();
			static void print(struct input_event ev);
	};
}
