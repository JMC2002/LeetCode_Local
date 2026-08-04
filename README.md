# 开发环境
VS2026 + WSL + GCC17 + C++29

基于静态反射，编译期加载题目、校验样例、加载样例

## 拉取和运行题目

直接拉取题目、C++ 模板和题面样例：

```powershell
python tools/fetch_problem.py 1
```

省略题号时会读取 `current_problem.txt`：

```powershell
python tools/fetch_problem.py
```

常用选项：

```powershell
python tools/fetch_problem.py 1 --select
python tools/fetch_problem.py 1 --force
python tools/fetch_problem.py 1 --inputs-only
python tools/fetch_problem.py 1 --dry-run
```

`solution.cpp` 永远不会被覆盖；已有 `cases.txt` 只有传入 `--force` 才会覆盖。
中国站拒绝无浏览器请求时，脚本会回退国际站并使用英文标题。

`select.ps1` 和 `run.ps1` 在本地没有对应题目时会自动调用拉取器：

```powershell
.\run.ps1 20
```

CMake 提供可执行目标 `fetch`。它会显示在 Visual Studio 顶部的启动目标下拉框中。
使用步骤：

1. 把 `current_problem.txt` 改成要拉取的题号。
2. 在启动目标下拉框中选择 `fetch`。
3. 点击绿色运行按钮。
4. 拉取完成后等待 Visual Studio 重新配置，然后切回 `lc`。

命令行也可以构建并运行同一个目标：

```powershell
cmake --preset wsl-gcc17-cxx29-debug
cmake --build --preset wsl-gcc17-cxx29-debug --target fetch
wsl.exe -d Ubuntu-26.04 --cd $PWD ./out/build/wsl-gcc17-cxx29-debug/fetch
```

CMake 还提供 `LC_AUTO_FETCH_MISSING` 选项，但默认关闭。只有在 CMake 直接使用
Windows 工作区或 `/mnt/c` 下的源目录时，才建议打开：

```powershell
cmake --preset wsl-gcc17-cxx29-debug -DLC_AUTO_FETCH_MISSING=ON
```

项目的 Visual Studio 预设启用了 `forceWSL1Toolset`，让 WSL 直接使用 Windows 工作区，
因此从 `fetch` 启动目标生成的题目会出现在当前仓库，而不是仅留在 WSL 镜像目录。
