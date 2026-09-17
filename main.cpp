#include <iostream>
#include <filesystem>
#include <windows.h>   // 新增，用于控制台编码设置
namespace fs = std::filesystem;

int main()
{
    SetConsoleOutputCP(CP_ACP); // 设置控制台输出GBK，解决中文乱码

    // 默认扫描当前程序所在目录；也可以改成你自己的目标文件夹路径
    fs::path scanDir = fs::current_path();
    if (!fs::exists(scanDir) || !fs::is_directory(scanDir))
    {
        std::cout << "路径不存在或不是文件夹！" << std::endl;
        return -1;
    }
    std::cout << "====扫描路径下所有文件====" << std::endl;
    std::cout << "扫描目录：" << scanDir << std::endl;
    //递归遍历所有子文件夹
    for (const auto& entry : fs::recursive_directory_iterator(scanDir))
    {
        if (entry.is_regular_file())
        {
            std::string fileNameNoExt = entry.path().stem().string();
            std::string fileExt = entry.path().extension().string();
            std::string fullFileName = entry.path().filename().string();
            std::cout << "完整文件名：" << fullFileName << std::endl;
            std::cout << "文件名：" << fileNameNoExt << std::endl;
            std::cout << "文件类型(后缀)：" << fileExt << "\n" << std::endl;
        }
    }
    return 0;
}
