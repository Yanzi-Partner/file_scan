# file_scan — 文件扫描与查找工具

基于 C++17 `<filesystem>` 的跨平台（Windows / Linux / macOS）命令行工具：递归扫描目录下的所有文件，支持按文件名关键词、扩展名过滤查找，并可进入交互式查找模式。

## 功能

| 功能 | 说明 |
| --- | --- |
| 递归扫描 | 遍历指定目录（默认当前目录）下所有子文件夹 |
| 关键词查找 | `-k 关键词`，对完整文件名做子串匹配，默认大小写不敏感 |
| 扩展名过滤 | `-e cpp` 或 `-e .cpp`，只显示指定后缀的文件 |
| 大小写控制 | `-c` 开启大小写敏感 |
| 交互式查找 | `-i`，扫描后反复输入关键词筛选，回车退出 |
| 结果展示 | 相对路径 + 人类可读文件大小，按路径排序，显示扫描/匹配总数 |
| 健壮性 | 跳过无权限/被占用的目录，不会因单个错误崩溃 |

## 编译

Windows + MinGW（g++）：

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o file_scan.exe main.cpp
```

Linux / macOS（g++ / clang++）：

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o file_scan main.cpp
```

## 使用

```
file_scan [目录] [-k 关键词] [-e 扩展名] [-c] [-i] [-h]
```

| 参数 | 含义 |
| --- | --- |
| `目录` | 要扫描的根目录，默认程序当前目录 |
| `-k 关键词` | 只显示文件名包含关键词的文件 |
| `-e 扩展名` | 只显示指定扩展名的文件（`.cpp` 与 `cpp` 均可） |
| `-c` | 大小写敏感（默认不敏感） |
| `-i` | 交互模式：扫描后反复输入关键词查找，回车退出 |
| `-h, --help` | 显示帮助 |

### 示例

```bash
file_scan                        # 列出当前目录所有文件
file_scan D:/projects -k main    # 在 D:/projects 中查找文件名含 main 的文件
file_scan -e .cpp -c             # 查找当前目录下所有 .cpp（区分大小写）
file_scan -i                     # 扫描后进入交互式查找
```

示例输出：

```
==== 文件查找结果 ====
扫描目录: C:/projects/file_scan
关键词:   cpp（不区分大小写）
------------------------------------------
   1 | 10.3 KB  | main.cpp
------------------------------------------
共扫描 31 个文件，匹配 1 个文件
```

## 在 VS Code 中使用

项目已附带 `.vscode/` 配置：

- `tasks.json` — 一键编译任务（`Ctrl+Shift+B`）
- `launch.json` — gdb 调试配置（`F5`），调试前自动编译
- `c_cpp_properties.json` — IntelliSense 配置（C++17）

依赖：安装 [C/C++ 扩展](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)，并确保 `g++`、`gdb` 在系统 PATH 中。

## 代码结构

| 函数 | 作用 |
| --- | --- |
| `toLowerAscii` | 字符串转小写（仅 ASCII） |
| `contains` | 子串匹配（可开关大小写） |
| `humanSize` | 字节数转可读大小（KB/MB/GB） |
| `collectFiles` | 递归扫描目录收集文件，跳过权限错误 |
| `matches` | 判断文件是否满足关键词 + 扩展名条件 |
| `printHelp` | 打印帮助信息 |
| `main` | 解析参数 → 扫描 → 过滤排序 → 输出 → 可选交互 |
