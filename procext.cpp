//  Process library extentions.
//
//  Copyright (C) 2026, Martin Young <martin_young@live.cn>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include "procext.hpp"

#ifdef _WIN32
  #include <cerrno>
  #define PCLOSE _pclose
  #define POPEN  _popen
#else
  #include <cerrno>
  #include <sys/wait.h> // POSIX 宏必需的系统头文件
  #define PCLOSE pclose
  #define POPEN  popen
#endif

// GNU libc++ extensions
#if defined(__GNUC__) && !defined(_MSC_VER)
  #include <ext/stdio_filebuf.h>
  #include <istream>
#endif

namespace ns_execute_cmd {

// 将核心管道读取逻辑合并，由 setcmd 和 cmdoutputline 共同复用
int execute_cmd(const std::string& cmd, void* ctx, void(*cb)(void*, std::string_view), std::string& errmsg) {
    errmsg.clear();
    if (cmd.empty()) return 0;

    int rawStatus = 0;
    auto closer = [&rawStatus](FILE* f) { if (f) rawStatus = PCLOSE(f); };
    std::unique_ptr<FILE, decltype(closer)> pipe(POPEN(cmd.c_str(), "r"), closer);

    if (!pipe) {
        errmsg = "popen failed: " + std::string(std::strerror(errno));
        return -1;
    }

#if defined(__GNUC__) && !defined(_MSC_VER)
    __gnu_cxx::stdio_filebuf<char> filebuf(pipe.get(), std::ios::in);
    std::istream is(&filebuf);
    std::string line;
    while (std::getline(is, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (!line.empty() && cb) cb(ctx, std::string_view(line));
    }
#else
    std::array<char, 256> buffer;
    std::string currentLine;
    currentLine.reserve(256);

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string_view chunk(buffer.data());
        currentLine += chunk;

        auto endPos = currentLine.find_first_of("\r\n");
        while (endPos != std::string::npos) {
            std::string_view line(currentLine.data(), endPos);
            if (!line.empty() && cb) cb(ctx, line);

            auto nextStart = currentLine.find_first_not_of("\r\n", endPos);
            if (nextStart == std::string::npos) {
                currentLine.clear();
                break;
            } else {
                currentLine.erase(0, nextStart);
                endPos = currentLine.find_first_of("\r\n");
            }
        }
    }

    if (!currentLine.empty()) {
        while (!currentLine.empty() && (currentLine.back() == '\r' || currentLine.back() == '\n'))
            currentLine.pop_back();
        if (!currentLine.empty() && cb) cb(ctx, std::string_view(currentLine));
    }
#endif

    // 显式断开流引用，确保 pclose 执行，获取正确的退出信号码
    pipe.reset();

    int exitCode = -1;
#ifdef _WIN32
    exitCode = rawStatus;
#else
    if (rawStatus != -1 && WIFEXITED(rawStatus)) {
        exitCode = WEXITSTATUS(rawStatus);
    }
#endif

    // 进程退出异常状态捕获
    if (exitCode == -1) {
        if (rawStatus == -1) {
            errmsg = "pclose failed";
        } else {
#ifndef _WIN32
            if (WIFSIGNALED(rawStatus)) {
                errmsg = "killed by signal " + std::to_string(WTERMSIG(rawStatus));
            } else
#endif
            {
                errmsg = "unknown error getting exit status";
            }
        }
    } else if (exitCode != 0)
        errmsg = "command exited with code " + std::to_string(exitCode);

    return exitCode;
}

} // namespace ns_execute_cmd

//------------------------------------------------------------------------
void cmdoutput::setcmd(const std::string& cmd)
{
    exitcode = 0;
    errmsg.clear();
    lines.clear();

    if (cmd.empty()) return;

    // 回调函数注入：将被统一接口生成的每一行输出注入到自身的 TStrVec 容器中
    auto cb = [](void* ctx, std::string_view line) {
        static_cast<cmdoutput*>(ctx)->lines.emplace_back(line);
    };

    exitcode = ns_execute_cmd::execute_cmd(cmd, this, cb, errmsg);
}
//------------------------------------------------------------------------
