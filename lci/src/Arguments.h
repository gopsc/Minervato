#pragma once
#include <iostream>
#include <map>
namespace qing {

	/*

	什么是C++的命令行参数？


	在C++中，命令行参数是程序从操作系统接受的输入数据，以下是关于它的详细介绍：

	1.定义

	argc：是一个整数类型（int）的参数，用于表示传递给程序的命令行参数的数量，包括程序本身的名称。

	argv：是一个字符串数组（char* argv[]），其中每个元素都是一个指向字符串的指针，

	这些字符串就是具体的命令行参数。


	2. 作用

	提供灵活性：使程序在运行时能够根据用户输入的不同参数来调整其行为，而无需修改程序代码本身。

	方便调试和配置：可以用于指定配置文件的路径、设置运行模式（如调试模式、发布模式等）、
	
	输入输出文件的名称等，从而方便程序的调试和使用。

	增强程序的通用性：同一个程序可以通过不同的命令行参数执行不同的任务，提高了程序的复用性和通用性。


	3.实例

	……


	4. 注意事项

	参数类型转换：由于命令行参数通常是字符串形式，
	
	如果下需要将其转换为其他数值类型（如整数、浮点数等），

	可以使用标准库函数进行转换，如“atoi”、“atof”等。但在转换时需要注意错误处理，

	以防止输入不合法的参数导致程序异常。

	参数验证：应始终对命令行参数进行验证，检查参数的数量是否符合预期，以及每个参数的值是否合法。

	如果参数不符合要求，应该给出相应的错误提示信息，并以一种合理的方式退出程序。


	总之，C++中的命令行参数是一种重要的机制，它允许程序在运行时接收来自操作系统的输入数据，

	并根据这些数据来调整程序的行为。通过合理地使用命令行参数，可以提高程序的灵活性、

	可配置性和通用性。

	*/

	class Arguments {
		public:
			Arguments(int argc, char **argv) {
				for (int i=1; i<argc; ++i) {

					std::string item = argv[i];

					if (item.length() > 1 && item[0] == '-' && item[1] != '-') {

						for (unsigned long j=1; j<item.length(); ++j) {

							if (item.length() == 2 && i < argc-1 && argv[i+1][0] != '-') {


								std::string args = "";
								while (i < argc-1 && argv[i+1][0] != '-') {
									args += (args == "") ? "" : " ";
									args += std::string(argv[++i]);
								}
								mc.insert(std::pair(item[j], args));
								std::cout << item[j] << "=" << args << std::endl;

							} else {

								mc.insert(std::pair(item[j], ""));
								std::cout << item[j] << std::endl;

							}

						}

					} else if (item.length() > 2 && item[0] == '-' && item[1] == '-' && item[2] != '-') {

						std::string arg = get_long_param_key(item);
						size_t pos =arg.find('=');
						if (pos == (long unsigned int)-1) {
							std::cout << arg << std::endl;
							m.insert(std::pair<std::string, std::string>(arg, ""));
						} else {
							std::string k = arg.substr(0, pos);
							std::string v = arg.substr(pos+1, arg.length()-pos-1);
							std::cout << k << "=" << v << std::endl;
							m.insert(std::pair<std::string, std::string>(k, v));
						}

					} else {

						throw WrongArgument(item);
					}

				}
			}

			std::map<std::string, std::string> &get_m() {
				return m;
			}

			std::map<char, std::string> &get_mc() {
				return mc;
			}

			class WrongArgument: std::runtime_error {
				public:
					WrongArgument(std::string key): std::runtime_error(key) {}
			};

		private:
			std::map<std::string, std::string> m;
			std::map<char, std::string> mc;
			
			std::string get_long_param_key(std::string &s) {
				return s.substr(2, s.length() -2);
			}

	};

}
