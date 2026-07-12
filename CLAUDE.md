> 声明：本项目的所有工作都不需要留档

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

山东石油化工学院宿舍管理系统（ShanShiDormManager）—— C++/Qt6 Widgets 桌面应用，为 2025 级程序设计实习课程作业。当前处于领域模型搭建阶段，UI 层仅占位，尚无业务逻辑接入。

> **重要**：修改代码后**不要**自动执行构建验证，应提示用户自行进行验证

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
- 学号按 `YYCCSSSS` 编码（前 2 位为年级后两位、3~4 位为班级、5~8 位为同一年级共享的唯一序列号）；自动分配使用该年级最小未占用序号
- 班级号 1~99、年级 2010~2099、宿舍号 101~9999 且末两位非 00（编码：末两位=房间号 01~99，百位及以上=楼层，`floor=dorm_id/100`，详见「dorm_id 编码与 floor 派生」）、宿舍楼号 1~99
- 床位号 1~max_num、楼层 1~max_floor（`is_valid_floor` 仍存，被 `is_valid_dorm_floor` 复用；dorm 的 floor 已从 dorm_id 派生，不独立存储）、姓名 1~20 字符且禁含 `"error"`
- 学生性别 `is_valid_gender` 0~2（0=未设置、1=男、2=女）；宿舍楼适用性别 `is_valid_building_gender` 1~3（1=男、2=女、3=男女混宿，无 0）
- dorm 房间性别锁定复用 `is_valid_gender`（0=未锁定/1=男舍/2=女舍，无 3——单间不可能男女混住）

**管理/查询方法返回 int 错误码**：`dorm` 的 `add_student`、`remove_student`、`swap_student`、`is_bed_occupied`、`get_student_id` 返回 `int`，正值表示成功（同时携带语义信息如床位号、学号），负值和零区分不同失败原因。`add_student(int student_id)` 错误码约定：`>0=成功（即分配的床位号）`/`-1=student_id 非法`/`-3=学生已有宿舍`/`-4=满员`/`-5=宿舍未配置（id 非法或 max_num<1）`/`-6=学号未注册于 studentmanager 或学生性别为 0`/`-7=性别与房间锁定不符`；指定床位重载 `add_student(int student_id, int bed_id)` 额外有 `-1=bed_id 非法`、`-2=床位占用`。具体返回值约定见 `dorm.h` 注释。`is_student_exist(int student_id)`（按学号判在不在本宿舍）返回 `bool`。

**const-only `get()` 与写入包装（本轮）**：三个 manager 对外公开的 `get()` 均只返回 `const` 指针，外部不能再通过 manager 指针调用任何 setter（包括仍为 public 的 setter）。外部写字段必须走 manager 包装接口；manager 内部需要修改本体时，经安全 `find` 或已由存在性守卫保护的容器直访拿到非 const 本体。`set_id` / `set_building_id` 仍保持 public，作为默认构造后的构造期入口；const-only `get()` 已堵住已入库对象的绕过修改，改主键/改楼这类跨聚合动作留给未来 `school` 用 remove+add 组合承载。

**`add_student` 前置校验**：`dorm::add_student` 进入分配前依次做前置校验——dorm 自身必须已配置（`id` 合法且 `max_num<1` 不成立，否则 `-5`）；学号格式合法（`is_valid_student_id`，否则 `-1`）；**学号必须已注册于 studentmanager 且学生性别不能为 0**（`studentmanager::instance().get(id)==nullptr` 或 `student.gender==0` 则 `-6`，杜绝幽灵占用并避免未设置性别的学生参与住宿）；学生不得已入住任意宿舍（`is_student_have_dorm==1` 则 `-3`）；宿舍未满（否则 `-4`）；房间若已锁定性别，入住者性别须匹配（调 `dorm::accepts_gender`，不接纳则 `-7`）。双参版顺序略有不同（先校验 bed_id 合法、床位占用），见 `dorm.cpp`。

**`add_student` 写 beds、锁定房间性别、正向同步 student 本体**：`dorm::add_student` 成功后把学号写进 `beds` 对应下标；若房间当时 `for_gender==0`（未锁定），把它**先到先得锁定**为首住客性别（`s->get_gender()`）；再经 `studentmanager::instance().assign_dorm_info(...)` 一次性写入 student 本体的位置四字段（绕过 setter 校验，因 dorm 字段已校验合法）。单参版自动找最小空床位并返回床位号；双参版按指定床位入住。房间性别一经锁定，`clear_students` 不解除（见下），只有 `clear_students_reset_gender` 或 `dormmanager::set_dorm_gender(_, _, 0)` 能解锁（`dorm::set_for_gender` 已收 private，见后）。

**`is_bed_occupied` 与 `is_student_exist` 区分命名**：两个方法参数同为 `int`，语义不同，特意分名以免重载歧义。`is_bed_occupied(int bed_id)`（按床位号查）三态返回 `int`：`1=有人`/`0=空床`/`-1=bed_id 非法`；`is_student_exist(int student_id)`（按学号查该生是否在本宿舍）返回 `bool`。

**`remove_student(int student_id)` 按学号移除**：按学号在 `beds` 里找人，找到就把该下标置 `0`（腾出床位），并经 `studentmanager::instance().clear_dorm_info(student_id)` 清零 student 本体的位置四字段，返回被腾出的**床位号**（`>0`）；`0=该学生不在本宿舍`、`-1=student_id 非法`。

**`swap_student(from, to)` 交换两下标的值并同步本体 bed_id**：`from` 空返回 `0`；`from==to` 返回 `-2`；`bed_id` 非法返回 `-1`；否则直接交换 `beds[from-1]` 与 `beds[to-1]` 两个学号（`to` 空即移动、`to` 有人即互换），并经 `studentmanager::instance().assign_dorm_info` 把交换后两侧非空学生本体的 `bed_id` 同步为新床位号（`dorm_id`/`building_id`/`floor` 未变，沿用 dorm 字段），成功返回 `1`。

**`set_max_num` 会 resize beds 并防丢人**：`set_max_num` 成功时同步把 `beds` 调到新长度；扩容时新床位默认 `0`（空）；**缩容时若被丢弃的尾部床位仍有人，则拒绝并返回 `false`**，防止静默丢人。

**`is_full` / `is_empty` / `get_current_num` / 清空双函数**：`get_current_num()` 遍历统计 `beds` 中非零床位数；`is_full()` 判断 `get_current_num() >= max_num`；`is_empty()` 判断 `==0`。清空分两个命名函数（不用 bool 重载，避免 `clear(true)` 读不出语义）：`clear_students()` 把 `beds` 全部置 `0`（长度不变、不改 `max_num`），置零前遍历已入住学号逐一调 `studentmanager::instance().clear_dorm_info` 清零对应 student 本体位置字段，**保留房间性别锁定**（空男舍仍是男舍）；`clear_students_reset_gender()` 先调 `clear_students()` 再把 `for_gender` 归 0（彻底放开）。`dormmanager::remove_dorm` 删整间宿舍时调 `clear_students()` 即可（宿舍本体随即销毁，性别留否无所谓）。

**`studentmanager` 为单例**：`studentmanager::instance()` 返回全局唯一实例（静态局部变量），构造函数私有化（`= default`）、删除拷贝构造与赋值运算符，禁止外部实例化。`get(int student_id) const` 按学号返回指向 `QHash` 内部 `student` 本体的只读指针，不存在返回 `nullptr`；**指针在后续 `add`/`remove` 触发 rehash 后可能失效**，请就地用完即弃；需长期引用请存学号、用时再 `get`。`add` 会校验字段合法、学号与年级班级编码一致、同年级序号唯一及完整学号唯一；`remove(int student_id)` 删除前先查 `is_student_have_dorm==1`，学生仍住宿舍时拒绝删除。`clear_dorm_info` / `assign_dorm_info` 仅供 `school` 同步住宿位置；`set_student_gender` 仅供 `school` 完成住宿一致性校验后调用，姓名可通过 `set_student_name` 修改。年级和班级不再提供独立 setter，必须由 `school::change_student_academic_info()` 一次提交并同步迁移学号主键及床位引用。`is_student_have_dorm(int student_id)` 检查学生是否已入住任意宿舍；`ids_of_class(int)` 遍历全量列出某班所有学号。

**`dormmanager` 为单例**：`dormmanager::instance()` 返回全局唯一实例（静态局部变量），构造函数私有化（`= default`）、删除拷贝构造与赋值运算符。存储结构为 `QMap<int, QMap<int, dorm>> dorms`（外层 key=building_id，内层 key=dorm_id）。**选 `QMap` 而非 `QHash`**：dorm 数量少（几百~几千），O(1) 与 O(log n) 无感知差异，但 `QMap` 白送两个关键收益——遍历天然按 building_id/dorm_id 升序（`get_available_dorm` 首个命中即「最小顺位」，无需额外排序），且红黑树插入不使已有项引用失效（只有删除被指向项才失效），比 `QHash` 的 rehash 全失效更契合「引用稳定」哲学；studentmanager 仍用 `QHash`（无有序需求），此不对称是按真实需求驱动、有意为之。`get(int building_id, int dorm_id) const` 两层 `find` 查找并返回只读指针，不存在返回 `nullptr`。`get_available_dorm(int gender)` / `get_available_dorm_random(int gender)` 返回 `const dorm*`，按性别找可用宿舍；入参只接受 1/2（否则 `nullptr`），三重过滤——楼级调 `building::accepts_gender`（横向查 `buildingmanager::instance().get(building_id)`；混宿(3)接纳任意、否则要求相等；楼未注册则该楼所有宿舍跳过）+ 房间级调 `dorm::accepts_gender`（未锁定(0)接纳任意、否则要求相等）+ 未满；前者按 QMap 升序返回首个命中（最小顺位），后者收集全部候选用 `QRandomGenerator::global()->bounded()` 等概率抽取。所有按外部 `(building_id,dorm_id)` 定位的写路径使用 `find`，避免 `dorms[building_id]` 对不存在楼号默认插入空内层表。`count()` 遍历外层累加内层 `size()`；`get_empty_count()`/`get_occupied_count()` 统计空/已占用宿舍数。`is_dorm_exist(int building_id, int dorm_id)` 三态返回 `int`：`1=存在`/`0=不存在`/`-1=参数非法`。`add_dorm(const dorm& dorm_to_add)`（building_id 取自 dorm 内部 `get_building_id()`）六重前置校验——building_id 合法、dorm_id 合法、max_num≥1（dorm 已配置）、同楼 dorm_id 唯一、building 已注册于 buildingmanager、`check::is_valid_dorm_floor(dorm_id, building.max_floor)`（派生楼层不越界）——通过后整体拷贝插入双层表。`remove_dorm(int building_id, int dorm_id)` 校验通过后先调 `dorm::clear_students()` 自动清空宿舍内学生（同步清理 student 本体位置字段），再移除；若该楼内层 QMap 变空则顺手清理外层条目。两个写方法均返回 `bool`。`set_dorm_gender(int building_id, int dorm_id, int gender)` 经 friend 后门调 `dorm::set_for_gender`（已收 private），前置做楼-房一致性校验（G2）：gender=0 解锁时跳过楼级校验；gender=1/2 须所在 building 经 `buildingmanager::get` 取 `for_gender` 后 `accepts_gender(gender)` 通过（混宿(3)接纳任意、否则要求相等；楼未注册或拒绝则 `-2`），再由 `dorm::set_for_gender` 内部校验住客性别一致性（不符 `-3`）。返回 `int`：`1=成功`/`0=宿舍不存在`/`-1=参数非法`/`-2=楼未注册或不接纳该性别`/`-3=住客性别冲突`。`set_dorm_max_num` 返回 `1=成功`/`0=宿舍不存在`/`-1=参数非法`/`-2=缩容会丢人`；`add_student_to_dorm` 两个重载先校验 building_id/dorm_id/student_id（非法返回 `-1`），定位失败返回 `-8`，否则透传 `dorm::add_student`；`add_student_to_available_dorm` / `add_student_to_available_dorm_random` 先校验 student_id（非法返回 `-1`），再查 studentmanager（学生不存在或 gender=0 返回 `-6`），再按学生性别找可用宿舍（无可用返回 `-9`），否则透传 `dorm::add_student`。`dormmanager` 因可用宿舍包装新增单向依赖 `studentmanager`，依赖图仍无环。

**`buildingmanager` 为单例**：`buildingmanager::instance()` 返回全局唯一实例，构造私有化、删拷贝。存储结构 `QMap<int, building> buildings`（key=building_id，按楼号升序），是 `building` 本体的唯一归属——`for_gender`/`max_floor` 等楼属性活在这里，dorm/dormmanager 只存 building_id、用时来换本体（与 studentmanager 存 student、dorm 只存学号同构）。`get(int building_id) const` 返回本体只读指针（楼不存在时返回 `nullptr`，QMap 插入不失效、删除被指向项才失效）。`add_building(int building_id, int gender, int max_floor)` 是 `building::set_for_gender` / `set_max_floor` 收 private 后的外部建楼入口，返回 `1=成功`/`0=楼号已存在`/`-1=字段非法`；`add_building(const building& b)` 保留用于已完整构造对象的接纳校验。`remove_building(int building_id)` 仅从表删除、**不级联清理楼内 dorm**（跨 manager 级联留待 school，避免 buildingmanager→dormmanager 反向依赖）；`set_building_gender` 返回 `1=成功`/`0=楼不存在`/`-1=gender非法`，只校验 building 自身并经 friend 调 `building::set_for_gender`，不级联检查楼内 dorm；`set_building_max_floor` 返回 `1=成功`/`0=楼不存在`/`-1=floor非法`，只校验格式并经 friend 调 `building::set_max_floor`，不级联检查既有宿舍派生楼层；`is_building_exist` 三态 `1/0/-1`；`count()` 返回楼总数。依赖方向 `dormmanager→buildingmanager` 单向（get_available_dorm 查楼性别），buildingmanager 不反向依赖 dormmanager，无环。

**哨兵值**：`dorm::get_student_id` 在 bed_id 非法或床位为空时返回 `-1`（整型）；`studentmanager::get` 用 `nullptr`。消费这些返回值时需显式判哨兵，不要当作正常数据处理。姓名字段仍禁含 `"error"`（历史约定，`check::is_valid_student_name` 强制）。

**默认构造表达"未设置"**：`student` 默认构造将所有字段置 0/空（`name=""`、`class_num=0`、`grade=0`、`id=0`、`bed_id=0`、`dorm_id=0`、`building_id=0`、`floor=0`、`gender=0`），统一表达"未设置"状态。`id=0` 是非法学号（`check::is_valid_student_id` 要求 10000000~99999999），配合 `studentmanager::add` 与 `dorm::add_student` 的校验，默认/未设置 id 的 student 无法被接纳，避免多个默认 student 因共享同一合法 id 而互相撞号。住宿语义中 `gender=0` 同样视为不可入住状态，即使该 student 已被加入 studentmanager，`dorm::add_student` 也会按 `-6` 拒绝。

**dorm 用定长 beds、按床位下标直接定位**：`dorm::beds` 是 `QVector<int>`，长度恒等于 `max_num`，下标 = 床位号 - 1、值 = 学号、`0` = 空床。按床位号取学号是 `beds[bed_id-1]` 直接下标访问，不再遍历。这是本轮重构的核心（旧模型是 `QVector<student>` 存对象副本 + 遍历查 bed_id）——不要改回存对象。

**dorm_id 编码与 floor 派生（本轮）**：dorm_id 编码规则——末两位=房间号(01~99)、百位及以上=楼层，故 `floor` 直接从 `id` 派生(`id/100`)，不再独立存储。3 位宿舍号(101~999)对应 1~9 楼、4 位(1001~9999)对应 10~99 楼；末两位为 00（房间号缺失）非法，由 `check::is_valid_dorm_id` 内联排除（范围 101~9999 且 `%100!=0`）。据此：dorm 删除 `floor` 字段与 `set_floor`，`get_floor()` 返回 `id/100`；`add_student`/`swap_student` 向 `studentmanager::assign_dorm_info` 传 floor 处改传 `get_floor()`。student 保留 `floor` 字段（作快照，由 `assign_dorm_info` 后门接收 dorm 派生值，默认构造仍为 0），但删除其 `set_floor` 死代码（原本就无调用者，`assign_dorm_info` 直接写 `this->floor`）。楼级 max_floor 校验下沉到 dormmanager：新增 `check::is_valid_dorm_floor(dorm_id, max_floor)`（复用 `is_valid_floor` 校验 `dorm_id/100 ∈ [1,max_floor]`，纯数值边界、不依赖聚合，与 `is_valid_bed_id(bed_id,max_num)` 同构），由 `dormmanager::add_dorm` 调用——须先从 buildingmanager 取 building 本体（`max_floor` 活在此），building 未注册或派生楼层越界则拒绝。原 `dorm::set_floor`/`student::set_floor` 硬编码 `max_floor=99` 的占位 TODO 随之消除。

**`student` 的 friend 关系已移交 `studentmanager`**：`student` 已摘除 `friend class dorm`，改为 `friend class studentmanager`。`assign_dorm_info`（后门直接写入位置四字段，绕过 setter 校验）是 student 的 private 方法，由 `studentmanager::assign_dorm_info` / `clear_dorm_info` 经 friend 调用，再由 `dorm::add_student`/`remove_student`/`clear_students` 间接调用完成位置字段同步。原本公开的 `student::clear_dorm_info` 已删除，清零职责整体移交 `studentmanager::clear_dorm_info`。**位置四字段 setter 已收 private**：`set_dorm_id`/`set_bed_id`/`set_building_id` 从 public 移入 private（`set_floor` 已删除而非收 private，floor 改由 `assign_dorm_info` 后门接收 dorm 派生值快照，见「dorm_id 编码与 floor 派生」），因为床位占用的权威是 `dorm.beds`，本体位置字段只应经 `assign_dorm_info` 后门同步；若这些 setter 公开，外部可让本体「自称住某处」却无对应 beds 记录，使 `is_student_have_dorm` 撒谎、`remove` 防护误判成删不掉的「僵尸学生」。`set_gender` 保持 public：默认构造后本地对象需要先设置性别再 `studentmanager::add`，已入库对象的外部绕过修改由 const-only `get()` 堵住；修改已入库学生性别请走 `studentmanager::set_student_gender`，其房间锁一致性闭环留给未来 `school`。现位置字段只有 friend 后门与构造函数两条写入路径。

**`dorm` 的 set_for_gender 已收 private**：与 student 位置四字段同构——`set_for_gender` 从 public 移入 private，因为钦定房间性别须校验所在 building 的 `for_gender`（楼-房一致性 G2），而 dorm 不依赖 buildingmanager；公开会允许在男生楼里钦定女舍，产出 `get_available_dorm` 永远选不中的「死间」（dorm::accepts_gender 拒绝同性别、building::accepts_gender 拒绝异性楼，两头堵死）。`dorm` 新增 `friend class dormmanager`，由 `dormmanager::set_dorm_gender` 经 friend 后门调用、在 dormmanager 层完成楼级校验（dormmanager 已单向依赖 buildingmanager，不引入新依赖）。for_gender 的两条类内写入路径（`add_student` 先到先得锁定、`clear_students_reset_gender` 归 0）直接写字段不走 setter，不受影响。`set_building_id` 保持 public，作为默认构造后加入 dormmanager 前设置主键关联的构造期入口；const-only `dormmanager::get` 已堵住外部经 manager 指针绕过修改。（`set_floor` 已删除，floor 改为从 id 派生，见「dorm_id 编码与 floor 派生」。）

**`building`**：`building` 类持有 `id`（宿舍楼号）、`max_floor`（最大楼层数）、`for_gender`（宿舍楼适用性别，1=男/2=女/3=男女混宿，经 `check::is_valid_building_gender` 校验）、`dorm_ids`（`QVector<QVector<int>>`，每层的宿舍号二维表，第一维楼层下标、第二维该层宿舍号列表）。getter 已全部 `const` 化（供 `buildingmanager::add_building(const building&)` 在 const 引用上读取）。`set_id` 保持 public，作为默认构造后设置楼号的构造期入口；`set_for_gender` / `set_max_floor` 已收 private，并新增 `friend class buildingmanager`，因为这两个字段会牵连楼内已存在 dorm 的性别锁定或派生楼层边界，building 自身无法自洽校验。外部新增楼应使用 `buildingmanager::add_building(building_id, gender, max_floor)`，由 manager 在内部构造合法本体；`buildingmanager::set_building_gender` / `set_building_max_floor` 只做字段格式校验并写入，不级联检查楼内 dorm，一致性闭环留给未来 `school`。`accepts_gender(int gender) const` 判断本楼适用性别是否接纳某学生性别（混宿(3)接纳任意，否则要求相等），供 `dormmanager::get_available_dorm` 做楼级性别过滤。原 `add_dorm`/`remove_dorm` 四个空壳方法已删除；`building.dorm_ids` 仍保留但暂无写入路径，谁是权威、要否收敛留待 school 协调层。dormmanager 两层键 (building_id, dorm_id) 是 dorm 的**复合主键**（dorm_id 仅楼内唯一），不是与 building 争夺归属职责。

**include 风格**：跨模块引用用目录前缀（`#include "system/check.h"`、`#include "core/student.h"`、`#include "ui/mainwidget.h"`），同目录文件用裸名（`#include "dorm.h"`）。`mainwidget.cpp` 中的 `#include "./ui_mainwidget.h"` 是 AUTOUIC 生成的头，无需手动创建。

**commit 风格**：中文、单行、逗号分隔。方法名和属性名后跟括号中文说明，如 `add_student（添加学生）`、`building_id（所在宿舍楼号）`。错误码或返回值约定在括号内简述，如 `int错误码返回值（>0成功/-1参数非法/-2床位占用/-3已存在/-4满员/0空床位）`。句式以"XX类新增/实现/修正YY"起头，用"同步ZZ"收尾关联变更。

## 待规划能力

- **住宿协调层（顶层 `school`，尚未实现）**：三个 manager（student/dorm/building）已各就位并实现 dorm↔studentmanager 位置同步、幽灵学号两端防护、dormmanager 基础 CRUD + 按性别找可用宿舍、buildingmanager 基础 CRUD。仍待 `school` 承载的跨聚合能力：
  - **反向级联**：`studentmanager::remove` 目前只「防」（住宿则拒删），未「做」自动退宿；`school::expel_student` 应一步完成「先退宿再删人」。放 school 而非 studentmanager，是避免 studentmanager→dormmanager 成环。
  - ~~**楼-房性别一致性（G2）**~~（已解决）：原缺口是 `dorm::set_for_gender` 只校验 0/1/2、不查 building，能在男生楼里钦定女舍造「死间」。现已由 `dormmanager::set_dorm_gender` 承载（`dorm::set_for_gender` 收 private + `friend class dormmanager`），校验放在 dormmanager 层——因 dormmanager 已单向依赖 buildingmanager，做楼级校验不引入新依赖、不成环，且与 `student→studentmanager` 的 friend 模式对称。原归 school 的理由「为保持 dorm 不依赖 buildingmanager」系归类错误：dorm 不依赖 buildingmanager 不等于校验须归 school，dormmanager 本就是 dorm 与 buildingmanager 间的顺向桥梁。
  - **`remove_building` 级联**：现仅删表不清理楼内 dorm，级联删楼归 school。
  - **`set_student_gender` 房间锁一致性**：`studentmanager::set_student_gender` 只校验学生字段并写入，不检查该生是否已入住、是否会与 `dorm.for_gender` 房间锁冲突；完整的「改性别前先退宿/迁宿/拒绝」由 school 承载。
  - **`set_building_gender` / `set_building_max_floor` 楼内 dorm 一致性**：`buildingmanager` 包装只校验 building 自身字段，不级联检查 dormmanager 中已存在 dorm 的房间性别锁或派生楼层（`dorm_id/100`）。若楼建好宿舍后改楼性别或把 `max_floor` 改小，会产生楼-房不一致；级联校验/迁移/拒绝归 school，避免 buildingmanager→dormmanager 反向依赖成环。
  - **改主键三件套**：`student::set_id`、`dorm::set_id`、`dorm::set_building_id`、`building::set_id` 保持 public 用于构造期设置；已入库对象改学号/宿舍号/楼号会牵连 manager key 与其它聚合引用，未来由 school 通过 remove+add 或专门事务接口承载。
  - 换宿舍、宿舍间换人、整楼搬迁等高级操作。
- ~~顶层管理类还需承载 `set_floor` 注释中提到的 per-building max_floor 查表~~（已解决）：floor 改为从 dorm_id 派生(`id/100`)、`set_floor` 已删，max_floor 校验由 `dormmanager::add_dorm` 经 `check::is_valid_dorm_floor` 承载（取 building 本体的 `max_floor`），不再需要 per-building 查表占位。详见「dorm_id 编码与 floor 派生」。
- **性别约束现状**：学生性别（`student.gender`）、楼适用性别（`building.for_gender` 1/2/3）、房间性别锁定（`dorm.for_gender` 0/1/2，先到先得 + 经 `dormmanager::set_dorm_gender` 钦定）三级已就位；`dorm::add_student` 已做房间级性别匹配、`dormmanager::get_available_dorm` 已做楼级+房间级双重过滤、`dormmanager::set_dorm_gender` 已做钦定时的楼-房一致性校验（G2 已补齐）。但已入住学生改性别、已建楼改楼性别/最大楼层仍是跨 manager 事务，留给 `school` 闭环。
- UI 层与领域模型的连接尚未开始。
