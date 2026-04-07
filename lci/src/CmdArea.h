/* 
 * 对大部分人来说，拿到一件产品是家常便饭。可对于设计者
 *     来说，仿佛度过了漫长的一生。
 *
 *  在舞台上是一生，在下水道里亦是一生，如果有的选，
 *     那就不叫人生。
 */

#pragma once
#include "Area.h"
#include "InputBox.h"
#include <memory>
#include <mutex>
namespace qing {
    /* 命令行区域 */
    class CmdArea: public Area {
        public:
            CmdArea(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize, Color fontClr);
            void Input(char c); /* 输出一个字符 */
            void ClearInput(); /* 清除输出 */
            virtual void print(char *utf8, int d); /* 在屏幕上打印文字 */
            void update_input(); /* 将缓冲区里的文字刷新到屏幕 */
            void delete_input(); /* 删除缓冲区里的一个文字 */
            void clearBox(); /* 清除整个输入框的内容 */
            std::string get_input_and_clear(); /* 获取输出，然后删除所有缓冲区的内容 */
        private:
            std::mutex lock; /* 线程安全 */
            int x_f=0, y_f=0; /* 文字的坐标 */
            std::unique_ptr<InputBox> area; /* 持有输入框 */
            std::string input = ""; /* 输入缓冲区 */
            void handle_ansi_code(char *buffer); /* 处理ansi代码 */
    };
}
