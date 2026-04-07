#include "prj.h"
namespace qing {
  bool Fs::try_before_all() {

    if (!sd_has_init && !begin()) /* 之前就未能启动，且重启失败 */
    {
      out.print("SD init failed\n");
      return false;
    }

    else
    {

      /* 重启成功 */
      out.print("SD restart successful\n");

      /* 更新标志 */
      return (sd_has_init = true);

    }

  }

  bool Fs::begin()  {
    return (sd_has_init = SD.begin(cs_pin));
  }

  bool Fs::isDirectory(String path) {

    /* SD没启动 */
    if (!try_before_all()) {
      return false;
    }

    /* 之前sd卡是启动了的 */
    File file = SD.open(path);

    /* 打开了 */
    return file && file.isDirectory();

  }

  bool Fs::isWorking() {

    /* SD没启动 */
    if (!try_before_all()) {
      return false;
    }

    /* 尝试打开根目录 */
    if (!isDirectory("/")) {
      out.print("SD Wrong\n");
      sd_has_init = false;
      return false;
    }

    else { /* 成功 */
      return true;
    }

  }

  bool Fs::isFile(String path) {

    /* SD卡启动失败 */
    if (!try_before_all()) {
      return false;
    }
    
    /* SD卡是启动了的 */
    File file = SD.open(path);

    /* 打开了文件且不是目录 */
    return file && !file.isDirectory();
    
  }


  String Fs::Read(String filename) {

    if (!isWorking()) { /* SD卡不正常 */
      return "";
    }

    if (isDirectory(filename)) {/* 是个目录 */
      out.print("Fs.Read() with a dirent.");
      return "";
    }


    File file = SD.open(filename);    /* 尝试打开文件或目录 */

    if (!file) { /* 打开失败 */
      out.print("SD file openning failed");
      return "";
    }

    else if (file && !file.isDirectory() && file.size() > FS_MAX_SIZE) { /* 打开失败 */
      out.print("SD big file can't read.");
      return "";
    }

    else if (file && !file.isDirectory() && file.size() <= FS_MAX_SIZE) { /* 是个文件 */
      String content = "";
      while (file.available()) {
        content += char(file.read());
      }
      file.close();
      return content;
    }

    else {
      //throw Arduino_Exception("Unknow Exception");
      out.print("fs_read(): 未知异常");
      return "";
    }
  }


  String Fs::List(String filepath) {

    if (!isWorking()) { /* SD卡不正常 */
      return "";
    }

    if (isFile(filepath)) {/* 是个文件 */
      out.print("Fs.List() with a file path.");
      return "";
    }

    File file = SD.open(filepath);    /* 尝试打开文件或目录 */

    if (!file) { /* 打开失败 */
      out.print("SD file openning failed");
      return "";
    }

    else if (file && file.isDirectory()){ /* 是个目录 */
      String list = "";
      
      // 添加父目录链接（如果不是根目录）
      if (filepath != "/") {
        String parentPath = filepath;
        int lastSlash = parentPath.lastIndexOf('/');
        if (lastSlash > 0) {
          parentPath = parentPath.substring(0, lastSlash);
        } else {
          parentPath = "/";
        }
      }
      
      // 列出目录内容
      while (true) {
        
        File entry = file.openNextFile();
        if (!entry) { /* 还有下个文件 */
          break;
        }

        if (list != "") { /* 添加换行符 */
          list += "\n";
        }
        
        String entryName = entry.name(); /* 获取基本文件名（去掉路径前缀）*/
        if (entryName.startsWith(filepath) && entryName.length() > filepath.length()) {
          entryName = entryName.substring(filepath.length());
          if (entryName.startsWith("/")) {
            entryName = entryName.substring(1);
          }
        }
        
        if (entryName.length() > 0) {
          
          /* 追加到列表 */
          list += entryName;
          
          /* 如果是文件夹 */
          //if (entry.isDirectory()) {
          //  list += "/";
          //}

        }
        entry.close();
      }
      

      file.close();
      return list;
    }
    

    else {
      //throw Arduino_Exception("Unknow Exception");
      out.print("Fs.List(): 未知异常");
      return "";
    }

  }


}