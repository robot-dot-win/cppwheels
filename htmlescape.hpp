//  HTML escaping tools.
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

#include <variant>
#include <ostream>
#include "strext.hpp"

//========================================================================
// 重要安全提醒：
// 1、htmljsesc() 仅适用于字符串字面量内部。不要用它转义整个 <script> 标签的内容或JS
//   代码结构。对于 <script> 标签内容，应使用 htmlesc() 并将 </ 替换为 <\/。
// 2、CSS 转义不能防御所有注入。如果攻击者能控制完整的 CSS 属性名或选择器，转义值无效。此
//   函数仅适用于 属性值内部（如 color: {{user_input}}）。
// 3、永远不要用同一个转义函数处理多个上下文。嵌套场景（如 HTML 属性中包含 JS）需要由内
//   向外逐层转义：先 htmljsesc()，再 htmlattesc()。
//========================================================================

//------------------------------------------------------------------------
// HTML转义函数智能返回类型：根据是否转义，自动选择零拷贝 (View) 或独占内存 (String)
//
class HtmlEscapedStr {
private:
    std::variant<std::string_view, std::string> data_;

public:
    explicit HtmlEscapedStr(std::string_view sv) : data_(sv) {}
    explicit HtmlEscapedStr(std::string&& s) : data_(std::move(s)) {}

    // 仅允许左值（有持久生命周期的变量）隐式转换为 std::string_view
    operator std::string_view() const & noexcept {
        if (auto* sv = std::get_if<std::string_view>(&data_)) return *sv;
        if (auto* s = std::get_if<std::string>(&data_)) return *s;
        return {};
    }

    // 拒绝右值（临时对象）的隐式转换，在编译期阻断 Use-After-Free
    operator std::string_view() const && = delete;

    std::string to_string() const {
        return std::string(static_cast<std::string_view>(*this));
    }

    // 专为输出流量身定制的友元重载，避免多余的隐式转换
    friend std::ostream& operator<<(std::ostream& os, const HtmlEscapedStr& str) {
        if (auto* sv = std::get_if<std::string_view>(&str.data_)) {
            os << *sv;
        } else if (auto* s = std::get_if<std::string>(&str.data_)) {
            os << *s;
        }
        return os;
    }
};

//------------------------------------------------------------------------
// 高性能 HTML 转义函数
//
// 用法 1：直接输出 (触发隐式转换 operator std::string_view)
//        std::cout << htmlesc(textsv);
// 用法 2：强制获取 std::string
//        std::string final_str = htmlesc(textsv).to_string();
//
inline HtmlEscapedStr htmlesc(std::string_view src, std::string_view special_chars = "&<>\"'")
{
    if (src.empty()) return HtmlEscapedStr(src);

    // 1. 第一阶段：极速扫描 (SIMD 加速)
    // 如果没有找到任何特殊字符，直接返回原 View，真正做到 0 分配 0 拷贝！
    size_t first_pos = src.find_first_of(special_chars);
    if (first_pos == std::string_view::npos) return HtmlEscapedStr(src);

    // 2. 第二阶段：不可避免地发生转义，进入内存分配模式
    std::string result;
    // 经验预分配：假设只有少量字符被转义，减少动态扩容开销
    result.reserve(src.size() + (src.size() / 8));

    // 将扫描到的第一段安全字符直接塞进去
    result.append(src.data(), first_pos);

    size_t pos = first_pos;
    while (pos < src.size()) {
        char c = src[pos];
        switch (c) {
        case '&':  result += "&amp;";  break;
        case '<':  result += "&lt;";   break;
        case '>':  result += "&gt;";   break;
        case '"':  result += "&quot;"; break;
        case '\'': result += "&#39;";  break;
        default:   fmt_char(result, "&#x{:02X};", "&#x%02X;", c);
        }

        pos++;

        // 查找下一段安全文本
        size_t next_pos = src.find_first_of(special_chars, pos);
        if (next_pos == std::string_view::npos) {
            result.append(src.data() + pos, src.size() - pos);
            break;
        }
        result.append(src.data() + pos, next_pos - pos);
        pos = next_pos;
    }

    return HtmlEscapedStr(std::move(result));
}

// ================================================================
// 属性值转义 (HTML Attribute Context)
// 适用: <div title="..."> <a href="..."> <input value="...">
// 必须编码: & " ' < > ` (反引号在部分旧浏览器中可作为属性定界符)
// 推荐始终使用双引号包裹属性值
// ================================================================
inline HtmlEscapedStr htmlattesc(std::string_view src)
{
    constexpr std::string_view special = "&\"'<>`";
    if (src.find_first_of(special) == std::string_view::npos)
        return HtmlEscapedStr{src};

    std::string result;
    result.reserve(src.size() + src.size() / 4);
    for (char c : src) {
        switch (c) {
        case '&':  result += "&amp;";  break;
        case '"':  result += "&quot;"; break;
        case '\'': result += "&#x27;"; break;  // &apos; 不在 HTML4 实体中
        case '<':  result += "&lt;";   break;
        case '>':  result += "&gt;";   break;
        case '`':  result += "&#x60;"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' && c != '\r')
                fmt_char(result,"&#x{:02X};", "&#x%02X;", c);
            else
                result.push_back(c);
        }
    }
    return HtmlEscapedStr{std::move(result)};
}

// ================================================================
// JavaScript 上下文转义 (Inline Script / Event Handler)
// 适用: <script>var x = "...";</script>  onclick="..."
// 策略: 将所有非 ASCII-字母数字字符转为 \uXXXX / \xXX，
//       彻底阻断 </script>、<!--、--> 等 HTML 解析器注入
// 注意: 仅适用于字符串字面量内部，不适用于 JS 代码结构
// ================================================================
inline HtmlEscapedStr htmljsesc(std::string_view src)
{
    // 快速路径：仅含安全字符时无需转义
    auto is_js_safe = [](unsigned char c) noexcept {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '.' || c == ',';
    };

    bool needs_escape = false;
    for (unsigned char c : src)
        if (!is_js_safe(c)) { needs_escape = true; break; }

    if (!needs_escape)  return HtmlEscapedStr{src};

    std::string result;
    result.reserve(src.size() * 6); // \uXXXX 最坏 6 字节/字符
    for (unsigned char c : src)
        if (is_js_safe(c))
            result.push_back(static_cast<char>(c));
        else if (c < 0x80)  // ASCII 控制字符及特殊符号 → \xHH
            fmt_char(result, "\\x{:02X}", "\\x%02X", c);
        else   // 非 ASCII 字节 → \u00HH（逐字节编码，保证UTF-8不被误解析）；若需保留 Unicode 语义，可改为解码后输出 \uXXXX 码点
            fmt_char(result, "\\u{:04X}", "\\u%04X", c);

    return HtmlEscapedStr{std::move(result)};
}

// ================================================================
// CSS 上下文转义 (Inline Style / Style Attribute)
// 适用: <div style="color: ...">  <style>body { content: "..." }</style>
// 策略: CSS 转义语法为 \HHHHHH (1-6位十六进制 + 可选空格)
//       所有非安全字符均转义，防止 expression()/url()/behavior() 注入
// 参考: CSS Syntax Module Level 3 §4.3
// ================================================================
inline HtmlEscapedStr htmlcssesc(std::string_view src)
{
    auto is_css_safe = [](unsigned char c) noexcept {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
    };

    bool needs_escape = false;
    for (unsigned char c : src) {
        if (!is_css_safe(c)) { needs_escape = true; break; }
    }
    if (!needs_escape)
        return HtmlEscapedStr{src};

    std::string result;
    result.reserve(src.size() * 7); // \HHHHHH + space 最坏 7 字节
    for (unsigned char c : src)
        if (is_css_safe(c))
            result.push_back(static_cast<char>(c));
        else    // CSS 转义: \XX + 空格（空格作为终止符，避免与后续字符合并）
            fmt_char(result, "\\{:02X} ", "\\%02X ", c);

    // 去除尾部多余空格（最后一个字符如果是安全字符则不需要终止空格）
    if (!result.empty() && result.back() == ' ')
        result.pop_back();

    return HtmlEscapedStr{std::move(result)};
}
//------------------------------------------------------------------------
