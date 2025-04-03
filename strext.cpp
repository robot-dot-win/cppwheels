//  Strings library extentions.
//
//  Copyright (C) 2024, Martin Young <martin_young@live.cn>
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

#include <random>
#include <unordered_set>

#include "strext.hpp"

const std::string EMPTY_STR{};
//const std::string SPACE_CHARS{" \t\n\r\f\v"};

//----------------------------------------------------------------------------------------------------
// My version
// std::string replall(const std::string& src, const std::string& sfind, const std::string& swith)
// {
//     if( src.empty() || sfind.empty() ) return src;
//
//     size_t pos = src.find(sfind);
//     if (nposs(pos)) return src;
//
//     size_t nfrom{};
//     std::string res;
//     do {
//         res += src.substr(nfrom, pos-nfrom)+swith;
//         nfrom = pos+sfind.length();
//         pos = src.find(sfind, nfrom);
//     } while( !nposs(pos) );
//     if( nfrom < src.size() ) res += src.substr(nfrom);
//     return res;
// }
//----------------------------------------------------------------------------------------------------
// Deepseek version
std::string replall(std::string_view src, std::string_view sfind, std::string_view swith) noexcept
{
    // 边界条件处理
    if (src.empty() || sfind.empty())  return std::string{src};
    if (sfind.length() > src.length()) return std::string{src};

    // Boyer-Moore 搜索器初始化
    const auto searcher{std::boyer_moore_searcher(sfind.begin(), sfind.end())};

    // 第一阶段：收集所有匹配位置
    std::vector<size_t> matches{};
    auto it{src.begin()};
    const auto end{src.end()};

    while (true) {
        const auto match{std::search(it, end, searcher)};
        if (match == end) break;

        const auto pos{static_cast<size_t>(match - src.begin())};
        matches.emplace_back(pos);
        it = match + sfind.length();
    }

    if (matches.empty()) return std::string{src};

    // 预分配内存
    const size_t find_len{sfind.length()};
    const size_t replace_diff{swith.length() - find_len};
    std::string result{};
    result.reserve(src.length() + matches.size() * replace_diff);

    // 第二阶段：构建结果
    size_t last_pos{};
    for (const auto& pos : matches) {
        result.append(src.substr(last_pos, pos - last_pos));
        result.append(swith);
        last_pos = pos + find_len;
    }
    result.append(src.substr(last_pos));

    return result;
}

//----------------------------------------------------------------------------------------------------
// My version
// std::string& replall(std::string& res, const std::string& src, const std::string& sfind, const std::string& swith)
// {
//     if( src.empty() || sfind.empty() ) {
//         res.clear();
//         return res;
//     }
//
//     size_t pos = src.find(sfind);
//     if (nposs(pos)) {
//         res = src;
//         return res;
//     }
//
//     size_t nfrom{};
//     res.clear();
//     do {
//         res += src.substr(nfrom, pos-nfrom)+swith;
//         nfrom = pos+sfind.length();
//         pos = src.find(sfind, nfrom);
//     } while( !nposs(pos) );
//     if( nfrom < src.size() ) res += src.substr(nfrom);
//     return res;
// }
//----------------------------------------------------------------------------------------------------
// Deepseek version
std::string& replall(std::string& result, std::string_view src, std::string_view sfind, std::string_view swith) noexcept
{
    // 清空并重置结果容器
    result.clear();
    result.shrink_to_fit();

    // 边界条件处理
    if (src.empty() || sfind.empty() || sfind.size() > src.size()) {
        result = src;
        return result;
    }

    // Boyer-Moore算法初始化
    const auto searcher{std::boyer_moore_searcher(sfind.begin(), sfind.end())};

    // 第一阶段：收集所有匹配位置
    std::vector<size_t> matches{};
    auto it{src.begin()};
    const auto end{src.end()};

    while (true) {
        const auto match{std::search(it, end, searcher)};
        if (match == end) break;

        const auto pos{static_cast<size_t>(match - src.begin())};
        matches.emplace_back(pos);
        it = match + sfind.size();
    }

    if (matches.empty()) {
        result = src;
        return result;
    }

    // 预计算所需内存
    const size_t src_len     {src.size()};
    const size_t find_len    {sfind.size()};
    const size_t replace_diff{swith.size() - find_len};
    const size_t total_size  {src_len + matches.size() * replace_diff};

    // 内存管理优化
    if (result.capacity() < total_size) {
        result.reserve(total_size * 1.2);  // 预分配额外20%空间
    } else {
        result.reserve(total_size);        // 复用已有容量
    }

    // 第二阶段：构建结果
    size_t last_pos{};
    for (const auto& pos : matches) {
        result.append(src.data() + last_pos, pos - last_pos);
        result.append(swith);
        last_pos = pos + find_len;
    }
    result.append(src.data() + last_pos, src_len - last_pos);

    return result;
}

//----------------------------------------------------------------------------------------
std::string genPassword(PasswordSecurityLevel level, size_t length)
{
    const std::string lowercase    {"abcdefghijklmnopqrstuvwxyz"};
    const std::string uppercase    {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    const std::string digits       {"0123456789"};
    const std::string specialChars {"!@#$%^&*()_+-=[]{}|;':\",./<>?"};

    std::string charset;
    switch (level) {
        case PasswordSecurityLevel::LOW:
            charset = lowercase + digits;
            break;
        case PasswordSecurityLevel::MEDIUM:
            charset = lowercase + uppercase + digits;
            break;
        case PasswordSecurityLevel::HIGH:
        default:
            charset = lowercase + uppercase + digits + specialChars;
            break;
    }

    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dis(0, charset.size() - 1);

    std::string password;
    password.reserve(length);

    std::generate_n(std::back_inserter(password), length, [&]{return charset[dis(gen)];});

    return password;
}

//----------------------------------------------------------------------------------------
bool chkPassword(std::string_view password, PasswordSecurityLevel level)
{
    constexpr std::string_view specialChars {"!@#$%^&*()_+-=[]{}|;':\",./<>?"};

    enum CharFlags : uint8_t {
        LOWER = 1 << 0,
        UPPER = 1 << 1,
        DIGIT = 1 << 2,
        SPECIAL = 1 << 3
    };

    uint8_t flags{};
    static const std::unordered_set special_set(specialChars.begin(), specialChars.end());

    const auto check_complete = [level, flags] {
        switch (level) {
            case PasswordSecurityLevel::LOW:
                return (flags & LOWER) && (flags & DIGIT);
            case PasswordSecurityLevel::MEDIUM:
                return (flags & LOWER) && (flags & UPPER) && (flags & DIGIT);
            case PasswordSecurityLevel::HIGH:
                return (flags & LOWER) && (flags & UPPER) &&
                       (flags & DIGIT) && (flags & SPECIAL);
        }
        return false;
    };

    for (const char c : password) {
        if (std::islower(c))       flags |= LOWER;
        else if (std::isupper(c))  flags |= UPPER;
        else if (std::isdigit(c))  flags |= DIGIT;
        else if (special_set.contains(c)) flags |= SPECIAL;     // C++ 20

        if (check_complete()) return true;
    }
    return check_complete();
}
