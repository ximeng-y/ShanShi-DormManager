> 声明：本项目的所有工作都不需要留档

This file provides guidance to Codex when working with code in this repository.

## 项目概述

山东石油化工学院宿舍管理系统（ShanShiDormManager）——C++/Qt6 Widgets 桌面应用，为 2025 级程序设计实习课程作业。当前领域模型、三个 manager 与顶层 `school` 基础业务层已建立，学生、宿舍、宿舍楼的常用维护、查询、统计和住宿调整入口基本完备；UI 层仍仅占位，尚无业务逻辑接入。

> **重要**：修改代码后**不要**自动执行构建验证，应提示用户自行进行验证。

无测试框架、无 lint 配置。`AUTOMOC`/`AUTOUIC`/`AUTORCC` 均开启；新增 `Q_OBJECT` 类、`.ui`、`.qrc` 时无需手动运行 moc/uic/rcc。新增源文件必须同时加入 `CMakeLists.txt` 的 `PROJECT_SOURCES`。

## 当前架构

源码位于 `src/`，并已加入 include 路径：

- `src/core/`：领域对象 `student`、`dorm`、`building`；同类对象集合管理器 `studentmanager`、`dormmanager`、`buildingmanager`；跨 manager 顶层协调类 `school`。
- `src/system/`：`check` 静态字段校验工具。
- `src/ui/`：Qt Widgets 界面，目前只有空的 `MainWidget`。

依赖方向必须保持：

```text
school
├─ studentmanager → student
├─ dormmanager → dorm
└─ buildingmanager → building
```

三个 manager 均为单例，只管理自己持有的同类本体，不得直接调用同级 manager。凡同时涉及两个及以上 manager 的业务，统一由 `school` 协调。底层对象只修改自身状态，不得反向查询 manager。

### 分层职责

- `student` / `dorm` / `building`：单个对象的字段、局部校验和局部状态变化。
- manager：对应本体的唯一归属、集合 CRUD、同类对象查询与局部写入包装。
- `school`：向 UI 提供统一的资料维护、目录查询和住宿统计入口，并协调入住/退宿、退学籍、性别纠错、调宿换床、单间清退、建删宿舍、删楼、修改楼属性、批量分配及宿舍/学生交换等跨聚合事务。

manager 的跨聚合敏感写接口使用 `private + friend class school` 收口，普通调用者不得绕过 `school` 单独修改床位或学生位置字段。

## 本体归属与事实源

### student

全校学生本体只存一份，由 `studentmanager` 的 `QHash<int, student>` 持有，key 为学号。`get()` 只返回 `const student*`；QHash 增删或 rehash 后指针可能失效，必须就地使用，长期引用保存学号。

学生位置快照包括 `dorm_id`、`bed_id`、`building_id`、`floor`。四字段只允许 `school` 经 `studentmanager::assign_dorm_info` / `clear_dorm_info` 同步，不允许外部独立修改。

### dorm

宿舍本体只由 `dormmanager` 持有，存储为 `QMap<int, QMap<int, dorm>>`，复合主键为 `(building_id, dorm_id)`。QMap 天然按楼号、宿舍号升序，插入不使其它元素指针失效；删除被指向对象后指针失效。

`dorm::beds` 是定长 `QVector<int>`：下标 = 床位号 - 1，值 = 学号，`0` = 空床，长度恒等于 `max_num`。床位占用的事实源是 `dorm.beds`，不得改回存储 `student` 对象副本。

`dorm` 不依赖 `student` 或 `studentmanager`。`add_student` 由调用方传入已校验的 `student_id` 和 `gender`，只修改床位与房间性别锁；学生存在性、重复入住、楼级性别和位置字段同步由 `school` 负责。

### building

宿舍楼本体只由 `buildingmanager` 的 `QMap<int, building>` 持有。`building` 仅保存 `id`、`max_floor`、`for_gender`，不再保存重复的 `dorm_ids` 目录；某楼包含哪些宿舍统一查询 `dormmanager`。

## school 已实现的协调闭环

- 添加学生及修改姓名、班级、年级；添加宿舍楼；修改宿舍最大床位数，缩容时禁止丢弃已有住客。
- 按升序返回学生、宿舍、宿舍楼稳定 ID/复合键目录；支持按班级、入住状态、宿舍楼、宿舍住客和可用宿舍筛选。
- 统计已入住/未入住学生数及全校总床位、已占床位、全部空床位。
- 指定宿舍/指定床位入住，自动或随机寻找可用宿舍。
- 退宿但保留学籍、退宿后删除学籍。
- 已入住学生可原子调往指定宿舍或指定空床，同宿舍可换至空床；失败时尝试恢复原床位，恢复不完整返回 `-10`。
- 两名指定学生可在同宿舍或跨宿舍互换床位，修改前校验目标楼与房间性别约束；同宿舍失败时尝试反向交换恢复，跨宿舍失败时尝试恢复两间宿舍快照，恢复不完整返回 `-6`。
- 性别纠错：未入住直接纠正；已入住时校验楼、房间及其他住客；单人宿舍可同步纠正房间锁；不自动迁宿。
- 添加/删除宿舍，删除前逐床核对学生位置记录；禁止携带预填住客的 dorm 入库。
- 空宿舍预设性别锁必须被所在楼接纳；有住客的宿舍禁止解锁为 0，防止后续异性入住形成混住。
- 删除宿舍楼并级联删除楼内宿舍、清退住客。
- 修改楼性别和最大楼层，存在既有宿舍或住客冲突时拒绝。
- 按楼级 + 房间级性别查询最小顺位/随机可用宿舍，统计指定性别可用空床。
- 全校随机补分、清空后重新分配。
- 清空指定宿舍或全部宿舍（保留/放开房间锁），并同步清除对应学生位置。
- 两间同锁宿舍整体交换、重叠交换、多余住客离宿、混宿楼男女宿舍交换；修改前保存床位快照，失败时尝试恢复原床位和房间锁，恢复不完整返回 `-6`。

`school` 当前已覆盖课程作业所需的基础领域能力，可作为后续 UI 接入的统一业务入口；不代表已经完成持久化、权限等完整学校管理系统能力。

## 关键数据规则

- 学号：`10000000~99999999`。
- 班级号：`1~99`。
- 年级：`2000~2999`。
- 宿舍楼号：`1~99`。
- 最大楼层：`1~99`。
- 学生性别：`0=未设置`、`1=男`、`2=女`。
- 楼适用性别：`1=男`、`2=女`、`3=男女混宿`，不允许 0。
- 房间性别锁：`0=未锁定`、`1=男舍`、`2=女舍`。
- 姓名：1~20 字符，历史规则禁止包含 `"error"`。

### dorm_id 与 floor

宿舍号范围 `101~9999`，末两位为房间号 `01~99`，百位及以上为楼层，末两位 `00` 非法：

```cpp
floor = dorm_id / 100;
```

`dorm` 不独立保存 floor；`student.floor` 是入住时同步的快照。`check::is_valid_dorm_floor(dorm_id, max_floor)` 只做数值边界校验，楼本体查找和楼层约束由 `school::add_dorm` 负责。

## 局部对象规则

- 类名沿用全小写：`student`、`dorm`、`building`、`school`、`check`。
- setter 返回 `bool`，字段校验下沉到 `check`；校验失败不得赋值。
- `set_id` / `set_building_id` 等主键关联 setter 只用于对象入库前构造。manager 的 `get()` 为 const-only，已入库对象不得通过指针绕过 manager/school 修改。
- `dorm::set_max_num` 会同步 resize `beds`；缩容若将丢弃的尾部床位有人，必须返回 `false`。
- `dorm::clear_students` 只清空本间 beds 并保留房间锁；`clear_students_reset_gender` 额外把房间锁归 0。两者不修改 student 本体。
- `dorm::swap_student` 和 `shuffle_beds` 只修改本间床位；`swap_student` 已由 `school` 包装用于换床与指定学生互换，并同步 student 的 `bed_id`；`shuffle_beds` 若未来作为公开业务使用，也必须由 `school` 包装并同步位置。
- `dorm::get_student_id` 在床位非法或为空时返回 `-1`；`studentmanager::get` 不存在返回 `nullptr`，消费时必须显式判断哨兵。
- 默认构造统一表达“未设置”：字段为 0 或空字符串；默认 student 不得直接入库或入住。
- `studentmanager::add` 只接纳住宿位置四字段全为 0 的学生；所有住宿位置必须由 `school` 建立。

## 错误码约定

- 纯判断使用 `bool`。
- 业务操作继续使用项目现有 `int` 错误码，不引入复杂结果类型。
- 通用语义：正数表示成功并可能携带床位号/数量，`0` 表示不存在或无操作，负数表示参数非法或业务冲突。
- 同一调用链中的同一错误码含义必须稳定；新增或修改方法时，在头文件注释中完整写明返回值。
- `school` 的交换方法额外使用 `-5` 表示住客/目标约束异常或执行失败，`-6` 表示失败后未能完整恢复快照。
- `school::move_student_to_dorm` 额外使用 `-10` 表示调宿失败后未能恢复学生原床位。

## include 与编辑风格

- 跨模块引用使用目录前缀，如 `#include "system/check.h"`、`#include "core/student.h"`、`#include "ui/mainwidget.h"`。
- 同目录文件使用裸文件名，如 `#include "dorm.h"`。
- `mainwidget.cpp` 的 `#include "./ui_mainwidget.h"` 是 AUTOUIC 生成头，无需创建。
- 注释默认使用中文，保持现有紧邻声明/实现的注释风格。
- 新增源文件必须同步加入 `CMakeLists.txt`。

## commit 风格

提交信息使用中文单行、聚焦单个能力或局部修复，通常只修改少量直接相关文件。句式以“XX类新增/实现/修正/迁移/移除 YY”开头，方法名和属性名后使用括号补充中文语义，例如：

```text
school类实现remove_student（退学籍）与correct_student_gender（性别纠错），同步闭环退宿删除与单人宿舍性别锁修正
```

错误码或返回值约定可在括号内简述；关联变更使用逗号连接，避免提交无关重构。

## 待规划能力

- UI 层与领域模型连接。
- 已入库对象改学号、宿舍号、楼号等主键事务。
- 整楼搬迁、按复杂条件批量编排、宿舍内随机换床等扩展业务入口。
- 持久化、导入导出、权限和操作日志。
- 自动化测试框架；在引入前仍遵守“不要自动构建验证”的项目要求。
