# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

山东石油化工学院宿舍管理系统（ShanShiDormManager）—— C++/Qt6 Widgets 桌面应用，为 2025 级程序设计实习课程作业。当前处于领域模型搭建阶段，UI 层仅占位，尚无业务逻辑接入。

## 构建与运行

工具链：CMake ≥ 3.16、Qt 6.10.x（MSVC2022 x64）、Ninja。Qt 安装路径在 `CMakeUserPresets.json` 中硬编码为 `D:/Qt/6.10.2/msvc2022_64`（另有 6.10.3 备选），通过 `QTDIR` 环境变量传入 `CMAKE_PREFIX_PATH`。**`CMakeUserPresets.json` 已被 `.gitignore` 忽略，机器本地路径不会进版本库。**

```bash
# 配置（二选一）
cmake --preset Qt-Debug      # 构建目录 out/build/debug
cmake --preset Qt-Release    # 构建目录 out/build/release

# 编译
cmake --build out/build/debug

# 运行
out/build/debug/ShanShiDormManager.exe
```

无测试框架、无 lint 配置。`AUTOMOC`/`AUTOUIC`/`AUTORCC` 均开启，新增 `Q_OBJECT` 类、`.ui`、`.qrc` 时无需手动跑 moc/uic/rcc。新增源文件**必须**同时加入 `CMakeLists.txt` 的 `PROJECT_SOURCES`，否则不会参与编译。

## 架构

源码组织为三层，均在 `src/` 下，且 `src/` 已加入 include 路径（`target_include_directories`）：

- `src/core/` —— 领域模型：`student`（学生）、`dorm`（宿舍）、`studentmanager`（管理全校学生本体）、`building`（宿舍楼，目前仅骨架）。纯数据 + 校验封装，不依赖 Qt Widgets（仅用 `QString`/`QVector`/`QHash`）。
- `src/system/` —— `check` 静态工具类，集中所有字段合法性校验。
- `src/ui/` —— Qt Widgets 界面，目前只有空的 `MainWidget`。

### 学生本体归属与存储模型（重要）

全校学生对象只存一份，归 `studentmanager` 持有（`QHash<int, student>`，学号→学生本体）。`dorm` 不再存学生对象，只存学号：成员 `QVector<int> beds`，下标 = 床位号 - 1、值 = 学号、`0` = 空床、长度始终等于 `max_num`。因此 `dorm` 不依赖 `student.h`，只认学号做增删查。需要学生详细信息时，拿学号去 `studentmanager::get` 换本体。

这带来一条本轮**尚未补上的边界**：`dorm` 与 `studentmanager` 之间没有协调层，谁都不负责把 `student` 的位置字段（`dorm_id`/`bed_id`/`building_id`/`floor`）和 `dorm.beds` 保持同步。所以现阶段 `student` 的这四个位置字段处于无人维护状态，且 `dorm` 能存入 `studentmanager` 里并不存在的学号——「学号必须真实存在」「反向字段同步」都留给未来的住宿协调层。床位占用的权威是 `dorm.beds`。

### 关键约定（跨多文件才看得出的隐含规则）

**类名全小写**：`student`、`dorm`、`check` 均为小写，与 Qt 的 PascalCase 风格相反。新增领域类请沿用此约定。

**Setter 返回 bool、校验下沉到 `check`**：`student::set_*` 与 `dorm::set_*` 全部返回 `bool`，内部调用 `check::is_valid_*` 做边界校验，校验失败则不赋值并返回 `false`。新增字段需同时在 `check` 加校验函数、在对应类加 `set_`/`get_`，并保持这一返回值契约。校验范围（见 `check.cpp`）：
- 学号 10000000~99999999（8 位：前 2 位年级后两位、3~4 位班级、5~8 位序列号）
- 班级号 1~99、年级 2000~2999、宿舍号 1001~9999、宿舍楼号 1~99
- 床位号 1~max_num、楼层 1~max_floor、姓名 1~20 字符且禁含 `"error"`

**管理/查询方法返回 int 错误码**：`dorm` 的 `add_student`、`remove_student`、`swap_student`、`is_bed_occupied`、`get_student_id` 返回 `int`，正值表示成功（同时携带语义信息如床位号、学号），负值和零区分不同失败原因。`add_student(int student_id)` 错误码约定：`>0=成功（即分配的床位号）`/`-1=student_id 非法`/`-3=已在本宿舍`/`-4=满员`/`-5=宿舍未配置（id 非法或 max_num<1）`；指定床位重载 `add_student(int student_id, int bed_id)` 额外有 `-1=bed_id 非法`、`-2=床位占用`。具体返回值约定见 `dorm.h` 注释。`is_student_exist(int student_id)`（按学号判在不在本宿舍）返回 `bool`。

**`add_student` 前置校验**：`dorm::add_student` 进入判重/分配前先做两项前置校验——dorm 自身必须已配置（`id` 合法且 `max_num<1` 不成立，否则返回 `-5`），传入学号必须格式合法（否则返回 `-1`）。注意这里只校验学号**格式**（`is_valid_student_id`，8 位区间），**不校验该学号是否真的在 `studentmanager` 里注册过**——这是「dorm 只认学号、不做协调」的直接结果，真实存在性留给未来协调层。

**`add_student` 只改 beds、不碰学生本体**：`dorm::add_student` 成功后只把学号写进 `beds` 对应下标，不触碰任何 `student` 对象、也不同步 `student` 的位置字段。单参版自动找最小空床位并返回床位号；双参版按指定床位入住。

**`is_bed_occupied` 与 `is_student_exist` 区分命名**：两个方法参数同为 `int`，语义不同，特意分名以免重载歧义。`is_bed_occupied(int bed_id)`（按床位号查）三态返回 `int`：`1=有人`/`0=空床`/`-1=bed_id 非法`；`is_student_exist(int student_id)`（按学号查该生是否在本宿舍）返回 `bool`。

**`remove_student(int student_id)` 按学号移除**：按学号在 `beds` 里找人，找到就把该下标置 `0`（腾出床位），返回被腾出的**床位号**（`>0`）；`0=该学生不在本宿舍`、`-1=student_id 非法`。同样只改 `beds`，不碰学生本体。

**`swap_student(from, to)` 交换两下标的值**：`from` 空返回 `0`；`from==to` 返回 `-2`；`bed_id` 非法返回 `-1`；否则直接交换 `beds[from-1]` 与 `beds[to-1]` 两个学号（`to` 空即移动、`to` 有人即互换），成功返回 `1`。

**`set_max_num` 会 resize beds 并防丢人**：`set_max_num` 成功时同步把 `beds` 调到新长度；扩容时新床位默认 `0`（空）；**缩容时若被丢弃的尾部床位仍有人，则拒绝并返回 `false`**，防止静默丢人。

**`is_full` / `get_current_num` / `clear_students`**：`get_current_num()` 遍历统计 `beds` 中非零床位数；`is_full()` 判断 `get_current_num() >= max_num`；`clear_students()` 把 `beds` 全部置 `0`（长度不变，不改 `max_num`）。

**`studentmanager::get` 返回可失效指针**：`studentmanager::get(int student_id)` 按学号返回指向 `QHash` 内部 `student` 本体的指针（有 const 与非 const 两个重载），不存在返回 `nullptr`。**指针在后续 `add`/`remove` 触发 rehash 后可能失效**，请就地用完即弃；需长期引用请存学号、用时再 `get`。`add` 会校验字段合法且学号唯一（重号返回 `false`）；`ids_of_class(int)` 遍历全量列出某班所有学号。

**哨兵值**：`dorm::get_student_id` 在 bed_id 非法或床位为空时返回 `-1`（整型）；`studentmanager::get` 用 `nullptr`。消费这些返回值时需显式判哨兵，不要当作正常数据处理。姓名字段仍禁含 `"error"`（历史约定，`check::is_valid_student_name` 强制）。

**默认构造表达"未设置"**：`student` 默认构造将所有字段置 0/空（`name=""`、`class_num=0`、`grade=0`、`id=0`、`bed_id=0`、`dorm_id=0`、`building_id=0`、`floor=0`），统一表达"未设置"状态。`id=0` 是非法学号（`check::is_valid_student_id` 要求 10000000~99999999），配合 `studentmanager::add` 与 `dorm::add_student` 的校验，默认/未设置 id 的 student 无法被接纳，避免多个默认 student 因共享同一合法 id 而互相撞号。

**dorm 用定长 beds、按床位下标直接定位**：`dorm::beds` 是 `QVector<int>`，长度恒等于 `max_num`，下标 = 床位号 - 1、值 = 学号、`0` = 空床。按床位号取学号是 `beds[bed_id-1]` 直接下标访问，不再遍历。这是本轮重构的核心（旧模型是 `QVector<student>` 存对象副本 + 遍历查 bed_id）——不要改回存对象。

**`dorm::set_floor` 的 max_floor 暂为占位**：当前硬编码 `max_floor = 99`，源码注释说明未来计划用顶层类管理一个 `vector`，按宿舍楼号查各自最大楼层。新增涉及楼层的逻辑时注意此约束尚未最终确定。

**`student` 已摘除 `friend class dorm`**：`dorm` 不再依赖也不再触碰 `student`，原来的 friend 声明已删。`assign_dorm_info`（后门直接写入位置四字段，绕过 setter 校验）仍是 student 的 private 方法，但**本轮没有任何调用者**，留作未来住宿协调层的接口——届时由协调类通过新的 friend 声明取得访问权。`clear_dorm_info`（把位置四字段清 0）仍是公开方法，同样暂无调用者。

**`building` 目前只是骨架**：`building` 类持有 `id`（宿舍楼号）、`max_floor`（最大楼层数）、`dorm_ids`（`QVector<QVector<int>>`，每层的宿舍号二维表，第一维楼层下标、第二维该层宿舍号列表），尚无任何方法实现，只是占位。

**include 风格**：跨模块引用用目录前缀（`#include "system/check.h"`、`#include "core/student.h"`、`#include "ui/mainwidget.h"`），同目录文件用裸名（`#include "dorm.h"`）。`mainwidget.cpp` 中的 `#include "./ui_mainwidget.h"` 是 AUTOUIC 生成的头，无需手动创建。

**commit 风格**：中文、单行、逗号分隔。方法名和属性名后跟括号中文说明，如 `add_student（添加学生）`、`building_id（所在宿舍楼号）`。错误码或返回值约定在括号内简述，如 `int错误码返回值（>0成功/-1参数非法/-2床位占用/-3已存在/-4满员/0空床位）`。句式以"XX类新增/实现/修正YY"起头，用"同步ZZ"收尾关联变更。

## 待规划能力

- **住宿协调层**：`dorm`（存学号）与 `studentmanager`（存本体）之间目前无人协调。入住/退宿/换宿舍时同步 `student` 的位置四字段、校验学号在 `studentmanager` 中真实存在、维护 `dorm.beds` 与 `student` 反向字段一致——这些都还没做。`student::assign_dorm_info` / `clear_dorm_info` 就是为此预留的接口。
- `dormmanager` / `buildingmanager` 等次顶层类（承载宿舍间换人、整楼搬迁等高级操作）与顶层 `school` 类尚未实现。
- 顶层管理类还需承载 `set_floor` 注释中提到的 per-building max_floor 查表。
- UI 层与领域模型的连接尚未开始。
