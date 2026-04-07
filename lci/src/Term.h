#pragma once
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
namespace qing {
	class Term {
		public:
			Term(int target_tty, int origin_tty) {
				this->origin_tty = origin_tty;
				chvt(target_tty);
				HideCursor(target_tty);
			}
			~Term(){
				chvt(origin_tty);
			}
		private:
			int origin_tty;
			static void HideCursor(int id) {
				char path[16] = {'\0'};
				snprintf(path, sizeof(path), "/dev/tty%d", id);
				int d = open("/dev/tty8", O_WRONLY);
				if (d == -1){
					throw std::runtime_error(path);
				}
				const char *hide = "\033[?25l";
				if (write(d, hide, strlen(hide)) == -1){
					close(d);
					throw std::runtime_error("term writting");
				}
				close(d);
			}
			static void chvt(int id) {
				char cmd[16] = {'\0'};
				snprintf(cmd, sizeof(cmd), "chvt %d", id);
				int err = system(cmd);
				if (err) throw std::runtime_error("chvt");
			}

	};
}
