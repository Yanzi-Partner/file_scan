/**
 * file_scan - 文件扫描与查找工具
 *
 * 功能：
 *   1. 递归扫描指定目录（默认当前目录）下的所有文件
 *   2. 按文件名关键词查找（子串匹配，默认大小写不敏感）
 *   3. 按扩展名过滤（如 .cpp / cpp）
 *   4. 交互式查找：扫描完成后可反复输入关键词筛选
 *   5. 输出文件相对路径、大小，并统计扫描/匹配数量
 *
 * 用法：
 *   file_scan [目录] [-k 关键词] [-e 扩展名] [-c] [-i] [-h]
 *     目录    要扫描的根目录（默认：程序当前目录）
 *     -k 关键词   只显示文件名包含关键词的文件
 *     -e 扩展名   只显示指定扩展名的文件（.cpp 与 cpp 均可）
 *     -c          大小写敏感（默认不敏感）
 *     -i          交互模式：扫描后反复输入关键词继续查找
 *     -h          显示帮助
 *
 * 编译（g++ / MinGW，Windows）：
 *   g++ -std=c++17 -O2 -Wall -Wextra -o file_scan.exe main.cpp
 */
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <windows.h> // SetConsoleOutputCP：解决中文乱码

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// 小工具函数
// ---------------------------------------------------------------------------

// 转小写（仅处理 ASCII；中文没有大小写，原样保留）
static std::string toLowerAscii(const std::string& s)
{
    std::string r = s;
    for (char& c : r)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

// 判断 text 是否包含子串 key，按 caseSensitive 决定是否区分大小写
static bool contains(const std::string& text, const std::string& key, bool caseSensitive)
{
    if (key.empty())
        return true;
    if (caseSensitive)
        return text.find(key) != std::string::npos;
    return toLowerAscii(text).find(toLowerAscii(key)) != std::string::npos;
}

// 字节数转可读字符串，如 1536 -> "1.5 KB"
static std::string humanSize(uintmax_t bytes)
{
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4)
    {
        v /= 1024.0;
        ++u;
    }
    char buf[64];
    if (u == 0)
        std::snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
    else
        std::snprintf(buf, sizeof(buf), "%.1f %s", v, units[u]);
    return buf;
}

// ---------------------------------------------------------------------------
// 查找选项与结果
// ---------------------------------------------------------------------------

struct SearchOptions
{
    fs::path root;         // 扫描根目录
    std::string keyword;   // 文件名关键词（空 = 不过滤）
    std::string extension; // 扩展名（空 = 不过滤）
    bool caseSensitive = false;
    bool interactive = false; // 是否进入交互式查找
};

struct FileInfo
{
    fs::path relPath; // 相对扫描根目录的路径
    uintmax_t size = 0;
};

// ---------------------------------------------------------------------------
// 递归扫描目录，收集所有常规文件
// 使用 directory_options::skip_permission_denied：遇到无权限/被占用的目录
// 会跳过而不是抛异常导致程序崩溃
// ---------------------------------------------------------------------------
static void collectFiles(const fs::path& dir,
                         const fs::path& root,
                         std::vector<FileInfo>& out,
                         uintmax_t& totalScanned)
{
    std::error_code ec;
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec)
        return; // 本目录无法访问，跳过

    fs::directory_iterator end;
    for (; it != end; it.increment(ec))
    {
        if (ec) // 遍历中出错（如文件被删除），跳过该条目继续
        {
            ec.clear();
            continue;
        }
        const fs::directory_entry& entry = *it;
        std::error_code ec2;

        if (entry.is_directory(ec2))
        {
            collectFiles(entry.path(), root, out, totalScanned);
        }
        else if (entry.is_regular_file(ec2))
        {
            ++totalScanned;
            fs::path rel = fs::relative(entry.path(), root, ec2);
            if (ec2)
                rel = entry.path().filename(); // 相对路径计算失败时退回文件名
            out.push_back({rel, entry.file_size(ec2)});
        }
    }
}

// ---------------------------------------------------------------------------
// 判断文件是否满足查找条件
// ---------------------------------------------------------------------------
static bool matches(const SearchOptions& opt, const FileInfo& fi)
{
    const std::string fileName = fi.relPath.filename().string(); // 完整文件名（含扩展名）

    // 1) 关键词匹配（对完整文件名做子串匹配）
    if (!contains(fileName, opt.keyword, opt.caseSensitive))
        return false;

    // 2) 扩展名过滤
    if (!opt.extension.empty())
    {
        std::string want = opt.extension;
        if (want.front() != '.')
            want = "." + want; // 统一成 ".cpp" 形式
        std::string got = fi.relPath.extension().string();
        if (!opt.caseSensitive)
        {
            want = toLowerAscii(want);
            got = toLowerAscii(got);
        }
        if (got != want)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 打印帮助
// ---------------------------------------------------------------------------
static void printHelp()
{
    std::cout
        << "file_scan - 文件扫描与查找工具\n"
        << "\n"
        << "用法:\n"
        << "  file_scan [目录] [-k 关键词] [-e 扩展名] [-c] [-i] [-h]\n"
        << "\n"
        << "参数:\n"
        << "  目录           要扫描的根目录（默认: 程序当前目录）\n"
        << "  -k 关键词      只显示文件名包含关键词的文件\n"
        << "  -e 扩展名      只显示指定扩展名的文件（.cpp 与 cpp 均可）\n"
        << "  -c             大小写敏感（默认不敏感）\n"
        << "  -i             交互模式: 扫描后反复输入关键词继续查找\n"
        << "  -h, --help     显示本帮助\n"
        << "\n"
        << "示例:\n"
        << "  file_scan                      列出当前目录所有文件\n"
        << "  file_scan D:/projects -k main  在 D:/projects 中查找文件名含 main 的文件\n"
        << "  file_scan -e .cpp -c           查找当前目录下所有 .cpp 文件（区分大小写）\n"
        << "  file_scan -i                   扫描后进入交互式查找\n";
}

// ---------------------------------------------------------------------------
// 主函数
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Windows 控制台输出 UTF-8，解决中文乱码（VS Code / Windows Terminal 适用）
    SetConsoleOutputCP(CP_UTF8);

    // ---- 1. 解析命令行参数 ----
    SearchOptions opt;
    opt.root = fs::current_path();

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            printHelp();
            return 0;
        }
        else if (arg == "-k" && i + 1 < argc)
        {
            opt.keyword = argv[++i];
        }
        else if (arg == "-e" && i + 1 < argc)
        {
            opt.extension = argv[++i];
        }
        else if (arg == "-c")
        {
            opt.caseSensitive = true;
        }
        else if (arg == "-i")
        {
            opt.interactive = true;
        }
        else
        {
            opt.root = arg; // 未以 - 开头的参数视为扫描目录
        }
    }

    // ---- 2. 校验目录 ----
    std::error_code ec;
    if (!fs::exists(opt.root, ec) || !fs::is_directory(opt.root, ec))
    {
        std::cerr << "错误：路径不存在或不是文件夹 —— " << opt.root << std::endl;
        return -1;
    }

    // ---- 3. 递归扫描 ----
    std::vector<FileInfo> all;
    uintmax_t totalScanned = 0;
    collectFiles(opt.root, opt.root, all, totalScanned);

    // ---- 4. 过滤 + 排序 ----
    std::vector<FileInfo> results;
    results.reserve(all.size());
    for (const FileInfo& fi : all)
        if (matches(opt, fi))
            results.push_back(fi);

    std::sort(results.begin(), results.end(),
              [](const FileInfo& a, const FileInfo& b)
              { return a.relPath.generic_string() < b.relPath.generic_string(); });

    // ---- 5. 输出结果 ----
    auto printResults = [&]()
    {
        std::cout << "\n==== 文件查找结果 ====\n";
        std::cout << "扫描目录: " << opt.root.generic_string() << "\n";
        if (!opt.keyword.empty())
            std::cout << "关键词:   " << opt.keyword
                      << (opt.caseSensitive ? "（区分大小写）" : "（不区分大小写）") << "\n";
        if (!opt.extension.empty())
            std::cout << "扩展名:   " << opt.extension << "\n";
        std::cout << "------------------------------------------\n";

        for (size_t i = 0; i < results.size(); ++i)
        {
            std::printf("%4zu | %-8s | %s\n",
                        i + 1,
                        humanSize(results[i].size).c_str(),
                        results[i].relPath.generic_string().c_str());
        }

        std::cout << "------------------------------------------\n";
        std::cout << "共扫描 " << totalScanned << " 个文件，匹配 " << results.size() << " 个文件\n";
    };
    printResults();

    // ---- 6. 交互式查找（可选）----
    if (opt.interactive)
    {
        std::cout << "\n进入交互查找：输入关键词过滤（直接回车退出）\n";
        std::string line;
        while (true)
        {
            std::cout << "> ";
            std::getline(std::cin, line);
            if (line.empty())
                break;

            opt.keyword = line;
            results.clear();
            for (const FileInfo& fi : all)
                if (matches(opt, fi))
                    results.push_back(fi);
            printResults();
        }
        std::cout << "已退出交互模式。\n";
    }

    return 0;
}
