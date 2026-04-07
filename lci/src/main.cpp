#include "Arguments.h"
#include "Term.h"
#include "KeyboardEvent.h"
#include "FrameBuf.h"
#include "CmdArea.h"
#include "CommonThread.h" /* 旧版 */
#include "th/Th.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#define TARGET_TTY 8
#define ORIGIN_TTY 1
using namespace qing;

/* 用于检查一个std::string的内容是否是数字。 */
bool isInteger(std::string &str) {
	for (char c: str) { //遍历整个字符串
		if (!std::isdigit(c)) return false;
	}
	return true;
}

/* 执行中间进程 使用 execlp 去切换到另一个命令的执行。 */
void runIntermediateProcess(int command_pipe_read, int slavefd);

namespace qing {

	/* 传入一个控制台设备文件的文件描述符，以及你要设定的行列值 */
	void set_terminal_size(int fd, int rows, int cols) {
		struct winsize sz = { .ws_row = (unsigned short)rows, .ws_col = (unsigned short)cols };
		if (ioctl(fd, TIOCSWINSZ, &sz) == -1) {
			throw std::runtime_error("ioctl");
		}
	}

	/* PrintThread 打印线程 */
	class p_th: public CommonThread {
		public:
			/**
			 * 构造函数 传入要读取的控制台设备文件描述符，背景画布，以及命令行区域
			 * 
			 * 这里我们主要是存储这几个东西（或它们的指针）
			 */
			p_th(std::string name, int masterfd, Drawable *back, CmdArea *font): CommonThread(name) {
				this->masterfd = masterfd; 
				this->back = back;
				this->font = font;
			}

			/**
			 * 析构函数 关闭线程
			 * 
			 * 我在可控制线程中主要遇到的问题便是线程的自主终止，
			 * 
			 * 在 Python 中似乎是只能实现可控制线程的手动退出的，
			 * 
			 * 我在想是不是在cpp中也做成那样。
			 */
			~p_th(){

				//使线程自主关闭
				if (this->chk() != SSHUT){
					this->shut();
				}

				//等待线程退出
				this->WaitClose();

			}

			/**
			 * 暂时删除复制构造函数，因为复制出一个与原来线程无关的空白线程似乎没有什么作用
			 * 
			 * 如果让两个变量引用同一个线程，会使得它们在结束的时候出现错误。
			 */
			p_th(p_th&) = delete;


			/**
			 * 线程睡眠阶段的操作函数，线程等待。
			 * 
			 * suspend() 方法似乎不太稳定，我还得再测试测试。
			 */
			void StopEvent() override {
				this->suspend();
			}

			/**
			 * 线程苏醒之后执行的操作，线程启动。
			 * 
			 * 我不知道为什么要等待1秒。
			 */
			void WakeEvent() {

				/* 将文件描述符集合清零 */
				FD_ZERO(&read_fds);

				/* 清空缓存字符串 */
				output = "";

				/* 设置完毕，线程进入运行态 */
				this->run();

			}

			/**
			 * 线程运行阶段的循环体，线程运行。
			 * 
			 * 使用 SELECT 进行缓冲区的探测。
			 */
			void LoopEvent() {

				std::cout << 1 << std::endl;
				/* 对可读集合进行清零 */
				//FD_ZERO(&read_fds);
				/* 设置可读集合 */
				//FD_SET(masterfd, &read_fds);

				/* 设置超时时间 */
				timeout.tv_sec = 0;
				timeout.tv_usec = 20000;

				/**
				 * 仅在集合中不存在控制台描述符时，才往其中添加该描述符
				 * 
				 * 在这个集合中，应该只会存在这一个文件描述符吧
				 */
				if (!FD_ISSET(masterfd, &read_fds)) FD_SET(masterfd, &read_fds);

				int ret = select(masterfd + 1, &read_fds, NULL, NULL, &timeout);
				if (ret == -1) {
					//throw std::runtime_error("select");
					//this->stop();
					return;
				} else if (ret == 0) { // No data to read 无值可读
					if (output == "") return;
					font->print((char*)output.c_str(), 5); // 按次为单位进行刷新应该可以大大提高效率
					back->flush(font, false);
					output = "";
				} else {
					if (FD_ISSET(masterfd, &read_fds)) { /* 有值可读 */
						nread = read(masterfd, buffer, sizeof(buffer));
						if (nread > 0) {
							buffer[nread] = '\0';
							output += buffer;
						}
					}
				}

			}

			/**
			 * 线程执行过后的清理函数，清理资源。
			 * 
			 * 什么也不做。我们在唤醒时并未申请资源。
			 */
			void ClearEvent() {
				;
			}


		private:

			/* 对象中存储的要读取的控制台设备文件描述符。 */
			int masterfd;

			/* 对象中存储的背景画布指针 */
			Drawable *back;

			/* 对象中存储的命令行区域指针 */
			CmdArea *font;

			/* 从终端设备中读取的进程输出 */
			std::string output;

			//--------------------------------------------
			/* 下面的变量并不是公用的，而是为了提高速度，避免重复申请的局部变量 */

			/* 文件描述符集合，用于存储 SELECT 返回的可读区域 */
			fd_set read_fds;

			/* 时间结构体 用于传入 SELECT 函数的超时时间 */
			struct timeval timeout;

			/* 这一次从控制台中所读取大小 */
			ssize_t nread;

			/* 存储从控制台中读取字符的缓冲区 */
			char buffer[1024];

	};
}


/**
 * 入口函数
 * 
 */
int main(int argc, char **argv) {


	/* 本次运行将要执行的命令 */
	std::string cmd = "";

	/* 如果为真，本次运行运行将仅做测试 */
	bool IsTest = false;

	/* 屏幕旋转角度（0-3） */
	int rotate = 0;

	/* 终端字号 */
	int fontsize = 18;


	/* 解析命令行参数 */
	Arguments args(argc, argv);

	/* 读取解析结果 单横线参数 */
	for (auto it = args.get_m().begin(); it != args.get_m().end(); ++it) {
		     if (it->first == "test" && it->second == "") IsTest = true;
		else if (it->first == "rotate" && it->second != "" && isInteger(it->second)) rotate = std::stoi(it->second);
		else if (it->first == "fontsize" && it->second != "" && isInteger(it->second)) fontsize = std::stoi(it->second);
		else if (it->first == "exec" && it->second != "") cmd = it->second;
	}

	/* 读取解析结果 单横线参数 */
	for (auto it = args.get_mc().begin(); it != args.get_mc().end(); ++it) {
		if (it->first == 'e' && it->second != "") cmd = it->second;
	}


	/**
	 * 测试模式
	 * 
	 * 该模式暂不可用
	 */
	if (IsTest) {
		//TestLib::test(&con);
		//TestLib::test1(&con);
		exit(0);
	}
	

	/**
	 * 如果输入了要运行的指令，就进入正常执行的分支
	 * 
	 * 
	 */
	if (cmd != "") {

		/* 控制台文件描述符 */
		int masterfd;

		/* 虚拟控制台 pty 的名字 */
		char *slave_name;

		// 打开一个新的虚拟控制台
		if ((masterfd = posix_openpt(O_RDWR)) == -1) {
			throw std::runtime_error("posix_openpt");
		}

		// ？解锁虚拟控制台
		if (grantpt(masterfd) != 0 || unlockpt(masterfd) != 0) {
			close(masterfd);
			throw std::runtime_error("grantpt/unlockpt");
		}

		// 获取虚拟控制台的名字
		slave_name = ptsname(masterfd);
		if (slave_name == NULL) {
			close(masterfd);
			throw std::runtime_error("ptsname");
		}

		std::cout << "Slave device is " << slave_name << std::endl;

		//--------------------------------------------------
		/* 创建两个管道：一个用于发送命令，一个用于接收输出 */
		int command_pipe[2]; // [0] 是读端，[1] 是写端
		if (pipe(command_pipe) == -1) {
			perror("pipe");
			return 1;
		}

		//--------------------------------------------------
		/* Fork 中间子进程 */
		pid_t intermediate_pid = fork();

		// pid 为-1通常意味着 分叉失败
		if (intermediate_pid == -1) {

			perror("fork");
			return 1;

		} else if (intermediate_pid == 0) {
			// 如果 pid 为0表示这是子进程
			// 中间子进程

			close(command_pipe[1]); // 关闭写端

			int slavefd = open(slave_name, O_RDWR);
			if (slavefd == -1) {
				std::runtime_error("open slave pty");
			}

			runIntermediateProcess(command_pipe[0], slavefd);

			//close(command_pipe[0]);
			//_exit(0);

		} else {
			// 主进程

			close(command_pipe[0]); // 关闭读端


			Term term(TARGET_TTY, ORIGIN_TTY);
			//std::vector<std::unique_ptr<Drawable>> v;
			auto fb = std::make_unique<FrameBuf>(std::string("/dev/fb0").c_str(), rotate, fontsize, white);
			auto kbe = std::make_unique<KeyboardEvent>();

			// 屏幕大小
			Rectangle rect = fb->get_size();
			set_terminal_size( masterfd, rect.h / (fontsize/2), rect.w / (fontsize/2) - 1 );

			int w = rect.w - 2 * 20;
			int h = rect.h - 2 * 20 - 14;
			int margin_left = 20;
			int margin_top = 20 + 14;

			auto cmdArea = std::make_unique<CmdArea>(fb.get(), w, h, margin_left, margin_top, 0, fontsize, white);
			fb->flush(cmdArea.get(), false);


			// 发送命令到中间子进程
			write(command_pipe[1], cmd.c_str(), cmd.size());
			const char* sig = "\n";
			write(command_pipe[1], sig, strlen(sig));

			p_th th("Printer", masterfd, fb.get(), cmdArea.get());
			th.wake();
			th.WaitStart(10000);

			bool shift = false;
			while(true) try {

				struct input_event ev = kbe->get();
				std::cout << "Key Press:\t" << ev.code << "," << ev.value << std::endl;

				if (ev.type == EV_KEY && ev.value == 1 && ev.code == 1) {

					break;

				} else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 14) {

					cmdArea->delete_input();
					cmdArea->clearBox();
					cmdArea->update_input();

				} else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 28) {

					auto input = cmdArea->get_input_and_clear();
					cmdArea->clearBox();
					cmdArea->update_input();
					if (input == "") {
						input = "\n";
					}
					const char *msg = input.c_str();
					write(masterfd, msg, input.length());

				} else if (ev.type == EV_KEY && ev.code == 42) {

					shift = (ev.value) ? true : false;

				} else if (ev.type == EV_KEY && ev.value == 1) {

					char c = '\0';
					switch (ev.code) {
						case 2:
							c = (shift) ? '!' : '1';
							break;
						case 3:
							c = (shift) ? '@' : '2';
							break;
						case 4:
							c = (shift) ? '#' : '3';
							break;
						case 5:
							c = (shift) ? '$' : '4';
							break;
						case 6:
							c = (shift) ? '%' : '5';
							break;
						case 7:
							c = (shift) ? '^' : '6';
							break;
						case 8:
							c = (shift) ? '&' : '7';
							break;
						case 9:
							c = (shift) ? '*' : '8';
							break;
						case 10:
							c = (shift) ? '(' : '9';
							break;
						case 11:
							c = (shift) ? ')' : '0';
							break;
						case KEY_A:
							c = (shift) ? 'A' : 'a';
							break;
						case KEY_B:
							c = (shift) ? 'B' : 'b';
							break;
						case KEY_C:
							c = (shift) ? 'C' : 'c';
							break;
						case KEY_D:
							c = (shift) ? 'D' : 'd';
							break;
						case KEY_E:
							c = (shift) ? 'E' : 'e';
							break;
						case KEY_F:
							c = (shift) ? 'F' : 'f';
							break;
						case KEY_G:
							c = (shift) ? 'G' : 'g';
							break;
						case KEY_H:
							c = (shift) ? 'H' : 'h';
							break;
						case KEY_I:
							c = (shift) ? 'I' : 'i';
							break;
						case KEY_J:
							c = (shift) ? 'J' : 'j';
							break;
						case KEY_K:
							c = (shift) ? 'K' : 'k';
							break;
						case KEY_L:
							c = (shift) ? 'L' : 'l';
							break;
						case KEY_M:
							c = (shift) ? 'M' : 'm';
							break;
						case KEY_N:
							c = (shift) ? 'N' : 'n';
							break;
						case KEY_O:
							c = (shift) ? 'O' : 'o';
							break;
						case KEY_P:
							c = (shift) ? 'P' : 'p';
							break;
						case KEY_Q:
							c = (shift) ? 'Q' : 'q';
							break;
						case KEY_R:
							c = (shift) ? 'R' : 'r';
							break;
						case KEY_S:
							c = (shift) ? 'S' : 's';
							break;
						case KEY_T:
							c = (shift) ? 'T' : 't';
							break;
						case KEY_U:
							c = (shift) ? 'U' : 'u';
							break;
						case KEY_V:
							c = (shift) ? 'V' : 'v';
							break;
						case KEY_W:
							c = (shift) ? 'W' : 'w';
							break;
						case KEY_X:
							c = (shift) ? 'X' : 'x';
							break;
						case KEY_Y:
							c = (shift) ? 'Y' : 'y';
							break;
						case KEY_Z:
							c = (shift) ? 'Z' : 'z';
							break;
						case KEY_SPACE:
							c = ' ';
							break;
						case 51:
							c = (shift) ? '<' : ',';
							break;
						case 52:
							c = (shift) ? '>' : '.';
							break;
						case 53:
							c = (shift) ? '?' : '/';
							break;
						case 12:
							c = (shift) ? '_' : '-';
							break;
						case 43:
							c = (shift) ? '|' : '\\';
							break;

					}

					if (c != '\0') {
						cmdArea->Input(c);
						cmdArea->clearBox();
						cmdArea->update_input();
					}

				} else if (ev.type == EV_REL && ev.code == REL_X) {
					std::cout << "X:\t\t" << ev.value << std::endl;
				} else if (ev.type == EV_REL && ev.code == REL_Y) {
					std::cout << "Y:\t\t" << ev.value << std::endl;
				}
			} catch( KeyboardEvent::NoEvent _ ) {
				;
			}


			// 等待中间子进程终止
			waitpid(intermediate_pid, nullptr, 0);
			// 关闭管道
			close(command_pipe[1]);
			close(masterfd);
		}



	}
}


void runIntermediateProcess(int command_pipe_read, int slavefd) {

	char buffer[1024];
	ssize_t nread;

	while (true) {
		// 清空缓冲区
		memset(buffer, 0, sizeof(buffer));

		// 从命令管道读取数据
		nread = read(command_pipe_read, buffer, sizeof(buffer) - 1);
		if (nread <= 0) {
			break; // 如果没有更多数据可读，退出循环
		}

		// 查找命令结束位置
		char* end_of_command = strchr(buffer, '\n');
		if (end_of_command != nullptr) {
			*end_of_command = '\0'; // 替换换行符为 null 终止字符串
		} else {
			continue; // 如果没有找到换行符，继续读取
		}

		// 检查是否收到结束信号
		if (strcmp(buffer, "END") == 0) {
			break; // 收到结束信号，退出循环
		}
		
		if (dup2(slavefd, STDIN_FILENO) != STDIN_FILENO ||
		    dup2(slavefd, STDOUT_FILENO) != STDOUT_FILENO ||
	 	   dup2(slavefd, STDERR_FILENO) != STDERR_FILENO) {
		    throw std::runtime_error("dup2");
		}
		if (slavefd > STDERR_FILENO) close(slavefd);

		// 执行命令
		execlp("/bin/sh", "/bin/sh", "-c", buffer, nullptr);
		perror("execlp");
		_exit(1);
	}
}
