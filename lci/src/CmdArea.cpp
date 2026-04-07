#include "CmdArea.h"
#include "utf8.h"
#include "ansi.h"
#include <cstring>
#include <vector>
constexpr auto H = 2;
namespace qing {

    CmdArea::CmdArea(Drawable *d, int w, int h, int x, int y, int rotate = 0, int fontsize=18, Color clr = white):
                 Area(d, w, h, x, y, rotate, fontsize, clr) {
        auto w_std = w - 2 * x; /* 神秘的两个变量 */
	auto w_tar = 16 * fontsize;
        area = std::make_unique<InputBox>( /* 构建输入框 */
            this,
            (w_tar < w_std) ? w_tar :  w_std,
            H*fontsize,
            x,
            fontsize*2,
            rotate,
            fontsize,
            green
        );
    }

    /* 增加一个输入字符 */
    void CmdArea::Input(char c) {
       	std::unique_lock<std::mutex> lck(this->lock);
        this->input += c;
    }

    /* 清除所有输入 */
    void CmdArea::ClearInput() {
        std::unique_lock<std::mutex> lck(this->lock);
        this->input = "";
    }

    /* 将输入发送到输入框 */
    void CmdArea::update_input() {
       	std::unique_lock<std::mutex> lck(this->lock);
        area->print((char*)this->input.c_str(), 0);
    }

    /* 
     * 清除整个输入框的内容
     *
     * _修改：难道不该高度也减一吗
     */
    void CmdArea::clearBox() {
        std::unique_lock<std::mutex> lck(this->lock);
        auto clr = black;
        auto size = area->get_size();
        area->rectangle_fill(0, 0, size.w-1, size.h, clr);
    }

    /* 删除一个字符 */
    void CmdArea::delete_input() {
        std::unique_lock<std::mutex> lck(this->lock);
        auto l = this->input.length();
        this->input = this->input.substr(0, l - 1);
    }

    /* 获取输入缓冲区的所有内容并且清除输入缓冲区 */
    std::string CmdArea::get_input_and_clear() {
        std::unique_lock<std::mutex> lck(this->lock);
        auto cur = this->input;
        this->input = "";
        return cur;
    }

    /* 打印到命令行区域 */
    void CmdArea::print(char *utf8, int d = 0) {

        auto subpos = area->get_pos();
        auto subsize = area->get_size();
        auto color = black;
        auto fontsize = FontSize_get();
        auto half_f = fontsize / 2;

        this->lock.lock();
        rectangle_fill(subpos.w-1, subpos.h-1, subsize.w+2, subsize.h+2, color);

        char *ansi = utf8::to_ansi(utf8);
        char *p = ansi;
        Rectangle l = get_size();
        while (1) {

            //int size = ansi::size_gb18030((unsigned char*)p, 4);
            /**
             * 这个老的矢量字库应该是 GB2312 的
             */
            int size = ansi::size_gb2312(*(unsigned char *)p);

            if (*p == '\n') {

                x_f = 0;
                y_f += fontsize;

            } else if (*p == '\0') {

                break;

            } else if (*p == '\t') {

                int n = x_f / half_f;
                int m = 8 - n % 8;
                x_f += m * half_f;

            } else if (*p == '\r') {
				
                ;

            } else if (*p == '\033') {
                p++;
                if (*p != '[' && *p != ']') continue;

                if (*p == ']') {
                    p++;
                    while (*p != '\007') {
                        p++;
                    }
                }

                else if (*p == '[') {
                    p++;
                    int top = 0, t2 = 0;
                    char buffer[32], buf2[16];
                    while((!(*p > 'a' && *p < 'z') && !(*p >'A' && *p < 'Z')) && (*p != '\0')) {
                        buffer[top++] = *(p++);
                    }
                    if (*p == '\0') break;
                    buffer[top++] = *p;
                    buffer[top] = '\0';
                    //printf("%s\n", buffer);
                    int i = 0;
                    while (i <= top) {
                        if (buffer[i] == ';') {
                            buf2[t2] = '\0';
                            handle_ansi_code(buf2);
                            buf2[0] = '\0';
                            t2 = 0;
                        } else {
                            buf2[t2++] = buffer[i];
                        }
                        i++;
                    }
                    buf2[t2] = '\0';
                    handle_ansi_code(buf2);
                }

            } else { // 正常的字 

                char c;
                if (size == 1) {
                    c = *(p + 1);
                    *(p + 1) = '\0';
                }
                drawvfont(x_f, y_f, *(unsigned short*)p, clr);
                if (size == 1) {
                    *(p + 1) = c;
                }
                x_f += (size == 1) ? fontsize / 2 : fontsize;

            }

            if (x_f >= l.w - fontsize) {

                y_f += fontsize;
                x_f = 0;

            }

            if (y_f >= l.h - (d + 1) * fontsize) {

                int level = (y_f % fontsize == 0) ? fontsize : y_f % fontsize;
                y_f -= level;
                up(0, 0, l.w - 1, l.h - d * fontsize - 1, level);

            }

            p += size;

        }
        free(ansi);
        subpos.w = ((x_f + fontsize + subsize.w) > l.w - fontsize) ? l.w - fontsize - subsize.w : x_f + fontsize;
        subpos.h = y_f + fontsize;
        area->set_pos(subpos);
        this->lock.unlock();
        this->update_input();
        flush(area.get(), true);
    }

    /* 处理颜色、位置等代码 */
    void CmdArea::handle_ansi_code(char *buffer) {
        static std::vector<std::string> v;
        std::string cur = buffer;
        char c = '\0';
        v.push_back(cur);
        if (!cur.empty() && std::isalpha((c = cur.back()))) {
            for ( const auto& elem: v ) {

                if (elem == "32m") {
                    Color clr = green;
                    set_color(clr);
                } else if (elem == "34m") {
                    Color clr = blue;
                    set_color(clr);
                } else if (elem == "30m") {
                    Color clr = gray;
                    set_color(clr);
                } else if (elem == "31m") {
                    Color clr = red;
                    set_color(clr);
                } else if (elem == "0m" || elem == "00m") {
                    Color clr = white;
                    set_color(clr);
                } else if (elem == "1m" || elem == "01m") {
                    if (v[0] == "40") {
                        ;
                    }

                    if (v[1] == "33") {
                        //Color clr = yellow;
                        //set_color(clr);
                    }
                }


                /* 调试用，重要 */
                std::cout << elem << " ";

            }
            v.clear();
            std::cout << std::endl;
        }

    }

}
