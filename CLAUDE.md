# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

山东石油化工学院宿舍管理系统（ShanShiDormManager）—— C++/Qt6 Widgets 桌面应用，为 2025 级程序设计实习课程作业。当前处于领域模型搭建阶段，UI 层仅占位，尚无业务逻辑接入。

## 构建与运行

工具链：CMake ≥ 3.16、Qt 6.10.x（MSVC2022 x64）、Ninja。Qt 安装路径通过 `QTDIR` 环境变量传入 `CMAKE_PREFIX_PATH`，在 `CMakeUserPresets.json` 中配置（此文件已被 `.gitignore` 忽略，机器本地路径不会进版本库）。

```bash
# 配置（二选一）
cmake --preset Qt-Debug
cmake --preset Qt-Release

# 编译
cmake --build out/build/debug

# 运行
out/build/debug/ShanShiDormManager.exe
```

> **重要**：修改代码后**不要**自动执行构建验证，应提示用户自行运行以上构建命令进行验证。

无测试框架、无 lint 配置。`AUTOMOC`/`AUTOUIC`/`AUTORCC` 均开启，新增 `Q_OBJECT` 类、`.ui`、`.qrc` 时无需手动跑 moc/uic/rcc。新增源文件**必须**同时加入 `CMakeLists.txt` 的 `PROJECT_SOURCES`，否则不会参与编译。

## 架构

源码组织为三层，均在 `src/` 下，且 `src/` 已加入 include 路径（`target_include_directories`）：

- `src/core/` —— 领域模型：`student`（学生）、`dorm`（宿舍）、`studentmanager`（管理全校学生本体，`QHash`）、`building`（宿舍楼）、`buildingmanager`（管理全校宿舍楼本体，`QMap`）、`dormmanager`（管理全校宿舍本体，`QMap`）。纯数据 + 校验封装，不依赖 Qt Widgets（仅用 `QString`/`QVector`/`QHash`/`QMap`/`QRandomGenerator`）。三个 manager 均为单例。
- `src/system/` —— `check` 静态工具类，集中所有字段合法性校验。
- `src/ui/` —— Qt Widgets 界面，目前只有空的 `MainWidget`。

### 学生本体归属与存储模型（重要）

全校学生对象只存一份，归 `studentmanager` 持有（单例，`instance()` 取全局唯一实例；`QHash<int, student>`，学号→学生本体）。`dorm` 不再存学生对象，只存学号：成员 `QVector<int> beds`，下标 = 床位号 - 1、值 = 学号、`0` = 空床、长度始终等于 `max_num`。因此 `dorm` 不依赖 `student.h`，只认学号做增删查。需要学生详细信息时，拿学号去 `studentmanager::instance().get` 换本体。

dorm↔studentmanager 之间的位置字段同步：`dorm::add_student` 成功时经 `studentmanager::instance().assign_dorm_info` 正向写入 student 的 `dorm_id`/`bed_id`/`building_id`/`floor`，`remove_student`/`clear_students` 经 `studentmanager::instance().clear_dorm_info` 反向清零这四字段；跨宿舍判重用 `studentmanager::instance().is_student_have_dorm`（查 student 本体 `dorm_id`/`bed_id` 是否非零）。床位占用的权威是 `dorm.beds`，位置四字段的权威是 student 本体。

**幽灵学号两端防护（本轮补上）**：曾经的妥协「dorm 只校验学号格式、不校验是否真实注册」已被推翻——因为 dorm→studentmanager 依赖边早已存在（add_student 本就在调 studentmanager），该妥协并未换来解耦，反而会制造「beds 记着某学号、studentmanager 查无此人」的幽灵占用。现两端已堵：**入住端** `dorm::add_student` 要求学号必须已注册于 studentmanager，且学生性别不能为 0（`studentmanager::get(id)==nullptr` 或 `student.gender==0` 则返回 `-6`；住宿语义中 `gender==0` 视为该学生记录不可用于入住/等同不存在）；**删除端** `studentmanager::remove` 发现学生仍住宿舍（`is_student_have_dorm==1`）则拒删返回 `false`，须先退宿再删人。两道防护都只查 student 本体自身字段，未引入 studentmanager→dormmanager 反向依赖，依赖图仍为单向 `dorm→studentmanager`。**仍留给协调层**：反向级联（studentmanager 删人时自动令其退宿，需 studentmanager→dormmanager，会成环，故不放 studentmanager）只「防」未「做」，完整的「一步退宿+删人」由未来 `school` 协调层承载。

### 关键约定（跨多文件才看得出的隐含规则）

**类名全小写**：`student`、`dorm`、`check` 均为小写，与 Qt 的 PascalCase 风格相反。新增领域类请沿用此约定。

**Setter 返回 bool、校验下沉到 `check`**：`student::set_*` 与 `dorm::set_*` 全部返回 `bool`，内部调用 `check::is_valid_*` 做边界校验，校验失败则不赋值并返回 `false`。新增字段需同时在 `check` 加校验函数、在对应类加 `set_`/`get_`，并保持这一返回值契约。校验范围（见 `check.cpp`）：
- 学号 10000000~99999999（8 位：前 2 位年级后两位、3~4 位班级、5~8 位序列号）
- 班级号 1~99、年级 2000~2999、宿舍号 1001~9999、宿舍楼号 1~99
- 床位号 1~max_num、楼层 1~max_floor、姓名 1~20 字符且禁含 `"error"`
- 学生性别 `is_valid_gender` 0~2（0=未设置、1=男、2=女）；宿舍楼适用性别 `is_valid_building_gender` 1~3（1=男、2=女、3=男女混宿，无 0）
- dorm 房间性别锁定复用 `is_valid_gender`（0=未锁定/1=男舍/2=女舍，无 3——单间不可能男女混住）

**管理/查询方法返回 int 错误码**：`dorm` 的 `add_student`、`remove_student`、`swap_student`、`is_bed_occupied`、`get_student_id` 返回 `int`，正值表示成功（同时携带语义信息如床位号、学号），负值和零区分不同失败原因。`add_student(int student_id)` 错误码约定：`>0=成功（即分配的床位号）`/`-1=student_id 非法`/`-3=学生已有宿舍`/`-4=满员`/`-5=宿舍未配置（id 非法或 max_num<1）`/`-6=学号未注册于 studentmanager 或学生性别为 0`/`-7=性别与房间锁定不符`；指定床位重载 `add_student(int student_id, int bed_id)` 额外有 `-1=bed_id 非法`、`-2=床位占用`。具体返回值约定见 `dorm.h` 注释。`is_student_exist(int student_id)`（按学号判在不在本宿舍）返回 `bool`。

**`add_student` 前置校验**：`dorm::add_student` 进入分配前依次做前置校验——dorm 自身必须已配置（`id` 合法且 `max_num<1` 不成立，否则 `-5`）；学号格式合法（`is_valid_student_id`，否则 `-1`）；**学号必须已注册于 studentmanager 且学生性别不能为 0**（`studentmanager::instance().get(id)==nullptr` 或 `student.gender==0` 则 `-6`，杜绝幽灵占用并避免未设置性别的学生参与住宿）；学生不得已入住任意宿舍（`is_student_have_dorm==1` 则 `-3`）；宿舍未满（否则 `-4`）；房间若已锁定性别，入住者性别须匹配（`for_gender!=0 && for_gender!=学生性别` 则 `-7`）。双参版顺序略有不同（先校验 bed_id 合法、床位占用），见 `dorm.cpp`。

**`add_student` 写 beds、锁定房间性别、正向同步 student 本体**：`dorm::add_student` 成功后把学号写进 `beds` 对应下标；若房间当时 `for_gender==0`（未锁定），把它**先到先得锁定**为首住客性别（`s->get_gender()`）；再经 `studentmanager::instance().assign_dorm_info(...)` 一次性写入 student 本体的位置四字段（绕过 setter 校验，因 dorm 字段已校验合法）。单参版自动找最小空床位并返回床位号；双参版按指定床位入住。房间性别一经锁定，`clear_students` 不解除（见下），只有 `clear_students_reset_gender` 或 `set_for_gender(0)` 能解锁。

**`is_bed_occupied` 与 `is_student_exist` 区分命名**：两个方法参数同为 `int`，语义不同，特意分名以免重载歧义。`is_bed_occupied(int bed_id)`（按床位号查）三态返回 `int`：`1=有人`/`0=空床`/`-1=bed_id 非法`；`is_student_exist(int student_id)`（按学号查该生是否在本宿舍）返回 `bool`。

**`remove_student(int student_id)` 按学号移除**：按学号在 `beds` 里找人，找到就把该下标置 `0`（腾出床位），并经 `studentmanager::instance().clear_dorm_info(student_id)` 清零 student 本体的位置四字段，返回被腾出的**床位号**（`>0`）；`0=该学生不在本宿舍`、`-1=student_id 非法`。

**`swap_student(from, to)` 交换两下标的值并同步本体 bed_id**：`from` 空返回 `0`；`from==to` 返回 `-2`；`bed_id` 非法返回 `-1`；否则直接交换 `beds[from-1]` 与 `beds[to-1]` 两个学号（`to` 空即移动、`to` 有人即互换），并经 `studentmanager::instance().assign_dorm_info` 把交换后两侧非空学生本体的 `bed_id` 同步为新床位号（`dorm_id`/`building_id`/`floor` 未变，沿用 dorm 字段），成功返回 `1`。

**`set_max_num` 会 resize beds 并防丢人**：`set_max_num` 成功时同步把 `beds` 调到新长度；扩容时新床位默认 `0`（空）；**缩容时若被丢弃的尾部床位仍有人，则拒绝并返回 `false`**，防止静默丢人。

**`is_full` / `is_empty` / `get_current_num` / 清空双函数**：`get_current_num()` 遍历统计 `beds` 中非零床位数；`is_full()` 判断 `get_current_num() >= max_num`；`is_empty()` 判断 `==0`。清空分两个命名函数（不用 bool 重载，避免 `clear(true)` 读不出语义）：`clear_students()` 把 `beds` 全部置 `0`（长度不变、不改 `max_num`），置零前遍历已入住学号逐一调 `studentmanager::instance().clear_dorm_info` 清零对应 student 本体位置字段，**保留房间性别锁定**（空男舍仍是男舍）；`clear_students_reset_gender()` 先调 `clear_students()` 再把 `for_gender` 归 0（彻底放开）。`dormmanager::remove_dorm` 删整间宿舍时调 `clear_students()` 即可（宿舍本体随即销毁，性别留否无所谓）。

**`studentmanager` 为单例**：`studentmanager::instance()` 返回全局唯一实例（静态局部变量），构造函数私有化（`= default`）、删除拷贝构造与赋值运算符，禁止外部实例化。`get(int student_id)` 按学号返回指向 `QHash` 内部 `student` 本体的指针（有 const 与非 const 两个重载），不存在返回 `nullptr`。**指针在后续 `add`/`remove` 触发 rehash 后可能失效**，请就地用完即弃；需长期引用请存学号、用时再 `get`。`add` 会校验字段合法（name/class_num/grade/gender/id）且学号唯一（重号返回 `false`）；`remove(int student_id)` 删除前先查 `is_student_have_dorm==1`，**学生仍住宿舍则拒删返回 `false`**（防止学号残留在 `dorm.beds` 成幽灵），`false` 现有两种原因：学号不存在或仍住宿舍；`clear_dorm_info(int student_id)` 使指定学号学生离宿（经 friend 后门调 `student::assign_dorm_info(0,0,0,0)` 清零位置四字段，`1=成功`/`-1=学号不存在`）；`assign_dorm_info(int student_id, int bed_id, int dorm_id, int building_id, int floor)` 使学生入住（正向写入四字段，与 `clear_dorm_info` 对称，供 `dorm::add_student` 成功分支调用）；`is_student_have_dorm(int student_id)` 检查学生是否已入住任意宿舍（查本体 `dorm_id`/`bed_id` 是否非零，`1=已分配`/`0=未分配`/`-1=学号不存在`）；`ids_of_class(int)` 遍历全量列出某班所有学号。

**`dormmanager` 为单例**：`dormmanager::instance()` 返回全局唯一实例（静态局部变量），构造函数私有化（`= default`）、删除拷贝构造与赋值运算符。存储结构为 `QMap<int, QMap<int, dorm>> dorms`（外层 key=building_id，内层 key=dorm_id）。**选 `QMap` 而非 `QHash`**：dorm 数量少（几百~几千），O(1) 与 O(log n) 无感知差异，但 `QMap` 白送两个关键收益——遍历天然按 building_id/dorm_id 升序（`get_available_dorm` 首个命中即「最小顺位」，无需额外排序），且红黑树插入不使已有项引用失效（只有删除被指向项才失效），比 `QHash` 的 rehash 全失效更契合「引用稳定」哲学；studentmanager 仍用 `QHash`（无有序需求），此不对称是按真实需求驱动、有意为之。`get(int building_id, int dorm_id)` 两层 `find` 查找，不存在返回 `nullptr`。`get_available_dorm(int gender)` / `get_available_dorm_random(int gender)` 按性别找可用宿舍：入参只接受 1/2（否则 `nullptr`），三重过滤——楼级 `building_accepts_gender`（横向查 `buildingmanager::instance().get(building_id)` 的 `for_gender`，3=混宿接纳任意、否则相等；楼未注册则该楼所有宿舍跳过）+ 房间级 `dorm_accepts_gender`（房间 `for_gender`，0=未锁定接纳任意）+ 未满；前者按 QMap 升序返回首个命中（最小顺位），后者收集全部候选用 `QRandomGenerator::global()->bounded()` 等概率抽取。`count()` 遍历外层累加内层 `size()`；`get_empty_count()`/`get_occupied_count()` 统计空/已占用宿舍数。`is_dorm_exist(int building_id, int dorm_id)` 三态返回 `int`：`1=存在`/`0=不存在`/`-1=参数非法`。`add_dorm(const dorm& dorm_to_add)`（building_id 取自 dorm 内部 `get_building_id()`）四重前置校验——building_id 合法、dorm_id 合法、max_num≥1（dorm 已配置）、同楼 dorm_id 唯一——通过后整体拷贝插入双层表。`remove_dorm(int building_id, int dorm_id)` 校验通过后先调 `dorm::clear_students()` 自动清空宿舍内学生（同步清理 student 本体位置字段），再移除；若该楼内层 QMap 变空则顺手清理外层条目。两个写方法均返回 `bool`。

**`buildingmanager` 为单例**：`buildingmanager::instance()` 返回全局唯一实例，构造私有化、删拷贝。存储结构 `QMap<int, building> buildings`（key=building_id，按楼号升序），是 `building` 本体的唯一归属——`for_gender`/`max_floor` 等楼属性活在这里，dorm/dormmanager 只存 building_id、用时来换本体（与 studentmanager 存 student、dorm 只存学号同构）。`get(int building_id)` 双重载返回本体指针（不存在 `nullptr`，QMap 插入不失效、删除被指向项才失效）。`add_building(const building& b)` 校验 id/for_gender/max_floor 合法且楼号唯一；`remove_building(int building_id)` 仅从表删除、**不级联清理楼内 dorm**（跨 manager 级联留待 school，避免 buildingmanager→dormmanager 反向依赖）；`is_building_exist` 三态 `1/0/-1`；`count()` 返回楼总数。依赖方向 `dormmanager→buildingmanager` 单向（get_available_dorm 查楼性别），buildingmanager 不反向依赖，无环。

**哨兵值**：`dorm::get_student_id` 在 bed_id 非法或床位为空时返回 `-1`（整型）；`studentmanager::get` 用 `nullptr`。消费这些返回值时需显式判哨兵，不要当作正常数据处理。姓名字段仍禁含 `"error"`（历史约定，`check::is_valid_student_name` 强制）。

**默认构造表达"未设置"**：`student` 默认构造将所有字段置 0/空（`name=""`、`class_num=0`、`grade=0`、`id=0`、`bed_id=0`、`dorm_id=0`、`building_id=0`、`floor=0`、`gender=0`），统一表达"未设置"状态。`id=0` 是非法学号（`check::is_valid_student_id` 要求 10000000~99999999），配合 `studentmanager::add` 与 `dorm::add_student` 的校验，默认/未设置 id 的 student 无法被接纳，避免多个默认 student 因共享同一合法 id 而互相撞号。住宿语义中 `gender=0` 同样视为不可入住状态，即使该 student 已被加入 studentmanager，`dorm::add_student` 也会按 `-6` 拒绝。

**dorm 用定长 beds、按床位下标直接定位**：`dorm::beds` 是 `QVector<int>`，长度恒等于 `max_num`，下标 = 床位号 - 1、值 = 学号、`0` = 空床。按床位号取学号是 `beds[bed_id-1]` 直接下标访问，不再遍历。这是本轮重构的核心（旧模型是 `QVector<student>` 存对象副本 + 遍历查 bed_id）——不要改回存对象。

**`dorm::set_floor` 的 max_floor 暂为占位**：当前硬编码 `max_floor = 99`，源码注释说明未来计划用顶层类管理一个 `vector`，按宿舍楼号查各自最大楼层。新增涉及楼层的逻辑时注意此约束尚未最终确定。

**`student` 的 friend 关系已移交 `studentmanager`**：`student` 已摘除 `friend class dorm`，改为 `friend class studentmanager`。`assign_dorm_info`（后门直接写入位置四字段，绕过 setter 校验）是 student 的 private 方法，由 `studentmanager::assign_dorm_info` / `clear_dorm_info` 经 friend 调用，再由 `dorm::add_student`/`remove_student`/`clear_students` 间接调用完成位置字段同步。原本公开的 `student::clear_dorm_info` 已删除，清零职责整体移交 `studentmanager::clear_dorm_info`。**位置四字段 setter 已收 private（本轮）**：`set_dorm_id`/`set_bed_id`/`set_building_id`/`set_floor` 从 public 移入 private，因为床位占用的权威是 `dorm.beds`，本体位置字段只应经 `assign_dorm_info` 后门同步；若这些 setter 公开，外部可让本体「自称住某处」却无对应 beds 记录，使 `is_student_have_dorm` 撒谎、`remove` 防护误判成删不掉的「僵尸学生」。现位置字段只有 friend 后门与构造函数两条写入路径。

**`building`**：`building` 类持有 `id`（宿舍楼号）、`max_floor`（最大楼层数）、`for_gender`（宿舍楼适用性别，1=男/2=女/3=男女混宿，经 `check::is_valid_building_gender` 校验）、`dorm_ids`（`QVector<QVector<int>>`，每层的宿舍号二维表，第一维楼层下标、第二维该层宿舍号列表）。getter 已全部 `const` 化（供 `buildingmanager::add_building(const building&)` 在 const 引用上读取），setter（`set_id`/`set_max_floor`/`set_for_gender`）经 `check` 校验。`add_dorm`/`remove_dorm`（向指定层增删宿舍号）目前仅有空函数体占位，尚未实现。注意：`building.dorm_ids`（按楼层组织的宿舍号）与 `dormmanager` 外层 key 存在「哪些宿舍属于本楼」的**冗余**，但 `add_dorm` 空占位、`dorm_ids` 从未被真正写过，暂无不一致；谁是权威、要否收敛留待 school 协调层。dormmanager 两层键 (building_id, dorm_id) 是 dorm 的**复合主键**（dorm_id 仅楼内唯一），不是与 building 争夺归属职责。

**include 风格**：跨模块引用用目录前缀（`#include "system/check.h"`、`#include "core/student.h"`、`#include "ui/mainwidget.h"`），同目录文件用裸名（`#include "dorm.h"`）。`mainwidget.cpp` 中的 `#include "./ui_mainwidget.h"` 是 AUTOUIC 生成的头，无需手动创建。

**commit 风格**：中文、单行、逗号分隔。方法名和属性名后跟括号中文说明，如 `add_student（添加学生）`、`building_id（所在宿舍楼号）`。错误码或返回值约定在括号内简述，如 `int错误码返回值（>0成功/-1参数非法/-2床位占用/-3已存在/-4满员/0空床位）`。句式以"XX类新增/实现/修正YY"起头，用"同步ZZ"收尾关联变更。

## 待规划能力

- **住宿协调层（顶层 `school`，尚未实现）**：三个 manager（student/dorm/building）已各就位并实现 dorm↔studentmanager 位置同步、幽灵学号两端防护、dormmanager 基础 CRUD + 按性别找可用宿舍、buildingmanager 基础 CRUD。仍待 `school` 承载的跨聚合能力：
  - **反向级联**：`studentmanager::remove` 目前只「防」（住宿则拒删），未「做」自动退宿；`school::expel_student` 应一步完成「先退宿再删人」。放 school 而非 studentmanager，是避免 studentmanager→dormmanager 成环。
  - **楼-房性别一致性（G2）**：`dorm::set_for_gender` 只校验 0/1/2，不查所在 building 的 `for_gender`，故能在男生楼里钦定女舍——不崩，但会成为 `get_available_dorm` 永远选不中的「死间」。楼-房一致性校验需查 building，为保持 dorm 不依赖 buildingmanager，归 school。
  - **`remove_building` 级联**：现仅删表不清理楼内 dorm，级联删楼归 school。
  - 换宿舍、宿舍间换人、整楼搬迁等高级操作。
- 顶层管理类还需承载 `set_floor` 注释中提到的 per-building max_floor 查表（当前 `dorm::set_floor`/`student::set_floor` 均硬编码 `max_floor=99`）。
- **性别约束现状**：学生性别（`student.gender`）、楼适用性别（`building.for_gender` 1/2/3）、房间性别锁定（`dorm.for_gender` 0/1/2，先到先得 + 可钦定）三级已就位；`dorm::add_student` 已做房间级性别匹配、`dormmanager::get_available_dorm` 已做楼级+房间级双重过滤。**唯一缺口是上面的 G2（楼-房一致性），归 school**。
- UI 层与领域模型的连接尚未开始。
