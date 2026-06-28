[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/robot-dot-win/cppwheels)

cppwheels
=========

1\. 项目概述
--------

`cppwheels` 是一个严格基于 C++20 标准构建的静态实用工具库 。该工程旨在填补 C++ 标准库在基础计算范式上的接口空白，提供一组经过优化、泛型化且高复用的基础设施。针对微型及小型软件架构，本项目通过原生实现规避了对外部重型框架（如 Boost, ICU）的依赖，从而最小化了编译期开销与工程部署复杂度 。

2\. 模块架构与功能特性
-------------

基于底层数据类型与操作逻辑，该代码库系统性地划分为以下核心功能域：

### 2.1 字符串处理与操控 (`strext`, `strjoin`)

* **词法分解 (Lexical Tokenization)**: 实现严谨的字符串分割算法 (`splits`, `splitsv`)，内置基于经验值的高效分隔符位置缓存机制 (`splitiv`)，并支持位运算选项（如修剪空白、丢弃空串）。

* **变换与截断**: 提供原地修改及返回副本的字符串操作（大小写转换、`ltrim`/`rtrim`/`trim`）。在内存安全的前提下，广泛采用 `std::string_view` 以确保零拷贝 (Zero-Copy) 语义。

* **子串替换**: `replall` 函数底层采用 Boyer-Moore 搜索器 (`std::boyer_moore_searcher`)，实现针对无重叠匹配项的高性能全量替换。

* **零拷贝拼接**: 引入惰性求值机制 (`strjoin`)，用于对容器内的字符串实体进行拼接。在支持 C++23 的环境下自动分发至 `std::views::join_with`，否则回退至 C++20 标准的自定义迭代器代理实现。

* **安全工具**: 包含基于信息熵分级的密码生成 (`genPassword`) 与正则校验 (`chkPassword`) 算法。

### 2.2 泛型容器算法 (`algext`)

* **成本感知的容器操作**: 为异构的 C++ 容器（顺序容器 `SequentialContainer`、关联容器 `AssociativeContainer`）实现泛型的集合代数运算（差集、并集）。

* **启发式减法 (`subContainer`)**: 在执行差集运算时，动态计算不同策略（扫描A、哈希B、扫描B）的算法复杂度，并执行时间复杂度最优的路径。

* **节点级转移 (`mvtoContainer`)**: 利用 C++17 引入的节点提取特性 (`extract()`)，在关联容器之间执行零拷贝的内存节点转移。

### 2.3 时间与时钟序列 (`chronoext`)

* **时间点格式化**: 封装 `std::chrono` 核心功能，若编译器支持则自动桥接 C++20 `std::chrono::zoned_time` 特性，实现高精度的本地日期与时间字符串生成。

* **超时管理**: 引入状态化的 `TTimeout` 实体用于执行确定性的过期判定 (`expires()`)，并提供 `TimeoutManager` 映射表用于集中的时钟状态追踪。

### 2.4 网络协议基础设施 (`ip4`)

* **IPv4 语义**: 封装稳健的 IPv4 地址解析逻辑，严格适配目标系统的字节序 (`__BYTE_ORDER__`)。支持无符号整数的位移、自增及比较运算。

* **CIDR 路由计算**: `ip4net` 模块支持解析 CIDR 表示法（如 `/24`），以代数方式精确计算子网掩码、子网地址及广播地址。

### 2.5 系统与子进程调度 (`procext`)

* **跨平台子进程管理**: 建立统一的执行接口，底层抹平了 POSIX `popen`/`pclose` 与 Windows `_popen`/`_pclose` 的系统差异。

* **流式处理**: 提供受 C++20 `std::invocable` 概念约束的 `cmdoutputline` 模板，通过函数式回调逐行处理标准输出流，并同步捕获进程退出码与异常信号。

### 2.6 数据格式与标识符 (`cfgfile`, `nejson`, `ltuuid`)

* **配置解析 (`cfgfile`)**: 执行对 INI/CFG 语法文本的词法解析，支持基于 Section 的键值对映射，并兼容行内注释剔除规则。

* **确定性 JSON (`nejson`)**: 作为 `nlohmann::json` 的“无异常 (No-Exception)”架构包装器，通过强制提供 Fallback 默认值，确保 JSON 树遍历 (`value`, `get_to`) 过程在 `noexcept` 语义下运行。

* **UUID 生成 (`ltuuid`)**: 采用线程局部 (thread-local) 的 `std::mt19937_64` 随机数引擎，实现完全符合 RFC 4122 标准的 UUID v4 生成器。通过模板特化提供对 C++20 `std::format` 的原生支持。

3\. 系统要求与构建指导
-------------

* **编译器规范**: 严格要求编译器提供对 C++20 标准的完整支持 (`cxx_std_20`) 。

* **构建系统**: 工程通过 CMake (`CMakeLists.txt`) 配置，默认构建为静态链接库 (Static Library) 输出 。

