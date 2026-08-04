#include <cerrno>
#include <cstdio>

#include <sys/wait.h>
#include <unistd.h>

#ifndef LC_FETCH_PYTHON
#error "LC_FETCH_PYTHON 未定义"
#endif

#ifndef LC_FETCH_SCRIPT
#error "LC_FETCH_SCRIPT 未定义"
#endif

int main()
{
    const pid_t child = ::fork();
    if (child < 0) {
        std::perror("无法创建力扣题目拉取进程");
        return errno == 0 ? 1 : errno;
    }

    if (child == 0) {
        // 不传题号时，Python 拉取器会读取工作区的 current_problem.txt。
        // --select 会原子重写选择文件，从而触发 Visual Studio 重新配置 CMake。
        ::execl(
            LC_FETCH_PYTHON,
            LC_FETCH_PYTHON,
            LC_FETCH_SCRIPT,
            "--select",
            static_cast<char*>(nullptr)
        );

        std::perror("无法启动力扣题目拉取器");
        _exit(errno == 0 ? 1 : errno);
    }

    int status = 0;
    if (::waitpid(child, &status, 0) < 0) {
        std::perror("等待力扣题目拉取器失败");
        return errno == 0 ? 1 : errno;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}
