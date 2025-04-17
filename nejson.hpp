//  Fed up with the exceptions of nlohmann::json library.
//  Wrapping it without exceptions - the No-Exception-Json.
//
//  Copyright (C) 2025, Martin Young <martin_young@live.cn>
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

#ifndef QQ_NEJSON_HPP
#define QQ_NEJSON_HPP

#include <string>
#include <utility>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace nejson {

//------------------------------------------------------------------------------------------------
inline bool kexists(const json& j, const std::string& key) noexcept {
    return j.is_object() && j.contains(key);
}

template <typename... Args>
bool kexists(const json& j, const std::string& first_key, Args&&... rest_keys) noexcept {
    return kexists(j, first_key) && kexists(j.at(first_key), std::forward<Args>(rest_keys)...);
}

//------------------------------------------------------------------------------------------------
// Gemini version: 2nd param is default_value
//-----------------------------------------------
//template <typename ValueType>
//ValueType getValueRecursiveHelper(const json& j, ValueType&& default_value) {
//    return j.template get<ValueType>();
//}
//
//template <typename ValueType, typename FirstKey, typename... RestKeys>
//ValueType getValueRecursiveHelper(const json& j, ValueType&& default_value, const FirstKey& key, const RestKeys&... rest_keys) {
//    if (!j.contains(key)) {
//        return default_value;
//    }
//    return getValueRecursiveHelper(j[key], std::forward<ValueType>(default_value), rest_keys...);
//}
//
//template <typename ValueType, typename... Keys>
//ValueType getValueRecursive(const json& j, ValueType&& default_value, const Keys&... keys) {
//    try {
//        return getValueRecursiveHelper(j, std::forward<ValueType>(default_value), keys...);
//    } catch (const std::exception& e) {
//        return default_value;
//    }
//}
//------------------------------------------------------------------------------------------------
// Gemini version: last param is default_value
//-----------------------------------------------
//template <typename ValueType>
//ValueType getValueOrDefault(const json& j, const std::vector<std::string>& keys, const ValueType& default_value) {
//    const json* current = &j;
//    for (const auto& key : keys) {
//        if (!current->contains(key)) {
//            return default_value;
//        }
//        current = &(*current)[key];
//    }
//    try {
//        return current->template get<ValueType>();
//    } catch (const std::exception& e) {
//        return default_value;
//    }
//}
//
//template <typename ValueType>
//auto getValue = [](const json& j, const std::vector<std::string>& keys, const ValueType& default_value) {
//    return getValueOrDefault(j, keys, default_value);
//};
//------------------------------------------------------------------------------------------------
// My simple version
template<class ValueType>
ValueType value(const json& j, const std::string& key1, ValueType&& default_value) noexcept
{ try {return j.value(key1, std::forward<ValueType>(default_value));} catch (...) {return std::forward<ValueType>(default_value);} }

template<class ValueType>
ValueType value(const json& j, const std::string& key1, const std::string& key2, ValueType&& default_value) noexcept
{ return kexists(j,key1)? value(j.at(key1), key2, std::forward<ValueType>(default_value)) : std::forward<ValueType>(default_value); }

template<class ValueType>
ValueType value(const json& j, const std::string& key1, const std::string& key2, const std::string& key3, ValueType&& default_value) noexcept
{ return kexists(j,key1,key2)? value(j.at(key1).at(key2), key3, std::forward<ValueType>(default_value)) : std::forward<ValueType>(default_value); }

template<class ValueType>
ValueType value(const json& j, const std::string& key1, const std::string& key2, const std::string& key3, const std::string& key4, ValueType&& default_value) noexcept
{ return kexists(j,key1,key2,key3)? value(j.at(key1).at(key2).at(key3), key4, std::forward<ValueType>(default_value)) : std::forward<ValueType>(default_value); }

//------------------------------------------------------------------------------------------------
template <typename ValueType>
bool get_to(const json& j, ValueType& v) noexcept
{ try {j.get_to(v); return true;} catch (...) {return false;} }

//------------------------------------------------------------------------------------------------
} // namespace nejson

#endif // QQ_NEJSON_HPP