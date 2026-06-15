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

#pragma once

#include <string>
#include <string_view>
#include <concepts>
#include <type_traits>
#include "strext.hpp"

//------------------------------------------------------------------------
// 统一的进程执行状态返回体
struct cmd_status {
    int exitcode{0};
    std::string errmsg;

    // 提供便捷的状态检验接口
    [[nodiscard]] bool ok() const noexcept {
        return exitcode == 0 && errmsg.empty();
    }
};
//------------------------------------------------------------------------
namespace ns_execute_cmd {
    // 底层统一接口：类型擦除以隔离所有 OS 宏和文件流扩展
    int execute_cmd(const std::string& cmd, void* ctx, void(*cb)(void*, std::string_view), std::string& errmsg);
}
//------------------------------------------------------------------------
class cmdoutput {
public:
    int exitcode{};             // cmd exit code
    std::string errmsg{};       // Not empty if error occurs or exitcode!=0
    TStrVec lines;

    cmdoutput() = default;
    explicit cmdoutput(const std::string& cmd) { setcmd(cmd); }

    // move construction
    cmdoutput(cmdoutput&&) noexcept = default;
    cmdoutput& operator=(cmdoutput&&) noexcept = default;

    // copy construction
    cmdoutput(const cmdoutput&) = default;
    cmdoutput& operator=(const cmdoutput&) = default;

    void setcmd(const std::string& cmd);
};

//------------------------------------------------------------------------
// Process every output line with 'func' and return the exit code
// C++20: 利用 std::invocable 进行严格的泛型约束
template <std::invocable<std::string_view> F>
[[nodiscard]] cmd_status cmdoutputline(const std::string& cmd, F&& func) {
    auto thunk = [](void* ctx, std::string_view line) {
        (*static_cast<std::remove_reference_t<F>*>(ctx))(line);
    };

    cmd_status status;
    status.exitcode = ns_execute_cmd::execute_cmd(cmd, &func, thunk, status.errmsg);
    return status;
}
//------------------------------------------------------------------------
