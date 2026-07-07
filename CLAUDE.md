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

- `src/core/` —— 领域模型：`student`、`dorm`。纯数据 + 校验封装，不依赖 Qt Widgets（仅用 `QString`/`QVector`）。
- `src/system/` —— `check` 静态工具类，集中所有字段合法性校验。
- `src/ui/` —— Qt Widgets 界面，目前只有空的 `MainWidget`。

### 关键约定（跨多文件才看得出的隐含规则）

**类名全小写**：`student`、`dorm`、`check` 均为小写，与 Qt 的 PascalCase 风格相反。新增领域类请沿用此约定。

**Setter 返回 bool、校验下沉到 `check`**：`student::set_*` 与 `dorm::set_*` 全部返回 `bool`，内部调用 `check::is_valid_*` 做边界校验，校验失败则不赋值并返回 `false`。新增字段需同时在 `check` 加校验函数、在对应类加 `set_`/`get_`，并保持这一返回值契约。校验范围（见 `check.cpp`）：
- 学号 10000000~99999999（8 位：前 2 位年级后两位、3~4 位班级、5~8 位序列号）
- 班级号 1~99、年级 2000~2999、宿舍号 1001~9999、宿舍楼号 1~99
- 床位号 1~max_num、楼层 1~max_floor、姓名 1~20 字符且禁含 `"error"`

**管理/查询方法返回 int 错误码**：`dorm` 的 `add_student`、`remove_student(int)`、`swap_student`、`is_student_exist(int)` 返回 `int`，正值表示成功（同时携带语义信息如床位号），负值和零区分不同失败原因。具体返回值约定见 `dorm.h` 注释。`remove_student(student&)` 和 `is_student_exist(const student&)` 保持 `bool`（仅两种结果）。

**`add_student` 自动同步原始对象**：`dorm::add_student` 传入非 const 引用，成功后将原始 `student` 对象的 `bed_id`、`dorm_id`、`building_id`、`floor` 同步更新。`student` 持有 `building_id` 和 `floor` 属性（分别复用 `check::is_valid_building_id` 和 `check::is_valid_floor`，`floor` 的 max_floor 当前硬编码为 99）。

**`remove_student(student&)` 自动清零**：`dorm::remove_student(student&)` 传入非 const 引用，移除成功后调用 `student::clear_dorm_info()` 将原对象的位置四字段（`bed_id`/`dorm_id`/`building_id`/`floor`）重置为 0。`clear_dorm_info()` 是 student 的公开方法，绕过 setter 校验直接赋 0，专用于此清零场景。

**`get_student` / `get_student_by_id` 返回 const 指针**：返回指向 dorm 内部 QVector 中 student 的只读指针，不存在时返回 `nullptr`。注意：指针在 `add_student`、`remove_student`、`clear_students` 等修改 students 列表的操作后可能失效（QVector 重分配）。

**`is_full` / `clear_students`**：`is_full()` 封装 `students.size() >= max_num`，`add_student` 内部已改为调用此方法。`clear_students()` 清空整个学生列表，内部持有的 student 对象随之销毁。

**哨兵值**：`get_*` 在输入非法或床位未入住时返回 `"error"`（字符串）或 `-1`（整型），**因此 `"error"` 被列为非法姓名**。消费这些返回值时需显式判哨兵，不要当作正常数据处理。

**dorm 持有动态学生列表，不按床位下标索引**：`dorm::students` 是 `QVector<student>`，按 `bed_id` 取学生信息时遍历查找（见 `dorm.cpp` 中 `get_student_name/id/class_num` 及 `get_student_*_list`）。这是有意为之的"动态列表模型"——不要改回按下标索引。空床位返回哨兵值，不占列表位置。

**`dorm::set_floor` 的 max_floor 暂为占位**：当前硬编码 `max_floor = 99`，源码注释说明未来计划用顶层类管理一个 `vector`，按宿舍楼号查各自最大楼层。新增涉及楼层的逻辑时注意此约束尚未最终确定。

**include 风格**：跨模块引用用目录前缀（`#include "system/check.h"`、`#include "core/student.h"`、`#include "ui/mainwidget.h"`），同目录文件用裸名（`#include "dorm.h"`）。`mainwidget.cpp` 中的 `#include "./ui_mainwidget.h"` 是 AUTOUIC 生成的头，无需手动创建。

**commit 风格**：中文、单行、逗号分隔。方法名和属性名后跟括号中文说明，如 `add_student（添加学生）`、`building_id（所在宿舍楼号）`。错误码或返回值约定在括号内简述，如 `int错误码返回值（>0成功/-1参数非法/-2床位占用/-3已存在/-4满员/0空床位）`。句式以"XX类新增/实现/修正YY"起头，用"同步ZZ"收尾关联变更。

## 待规划能力

- 顶层管理类（管理多栋宿舍楼、多宿舍，承载 `set_floor` 注释中提到的 per-building max_floor 查表）尚未实现。
- UI 层与领域模型的连接尚未开始。
