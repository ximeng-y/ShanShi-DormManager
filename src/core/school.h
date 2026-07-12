#ifndef SCHOOL_H
#define SCHOOL_H

#include <QVector>
#include <QPair>
#include <QString>
#include "sampledataplan.h"
#include "persistence/schoolsnapshot.h"
#include "persistence/schoolstorage.h"

class student;
class dorm;
class building;

enum class assignment_strategy
{
	random,
	fill_occupied_first
};

enum class reassignment_strategy
{
	random,
	fill_dorms_first,
	preserve_building_first
};

enum class clear_scope
{
	dorm,
	building,
	all
};

enum class dorm_adjustment_mode
{
	full_swap,
	overlap_swap,
	overlap_swap_and_evict,
	gender_dorm_swap
};

enum class persistence_start_status
{
	ready,
	fallback_ready,
	backup_restored,
	data_conflict,
	read_only_corrupt,
	read_only_unwritable,
	read_only_newer_version
};

struct accommodation_change
{
	int student_id = 0;
	int student_gender = 0;
	int old_building_id = 0;
	int old_dorm_id = 0;
	int old_bed_id = 0;
	int new_building_id = 0;
	int new_dorm_id = 0;
	int new_bed_id = 0;
};

struct accommodation_data_issue
{
	int student_id = 0;
	int building_id = 0;
	int dorm_id = 0;
	QString message;
};

struct batch_assignment_preview
{
	assignment_strategy strategy = assignment_strategy::fill_occupied_first;
	quint32 random_seed = 0;
	int candidate_count = 0;
	int available_bed_count = 0;
	quint64 resource_signature = 0;
	QVector<accommodation_change> changes;
	QVector<int> unassigned_student_ids;
	QVector<accommodation_data_issue> issues;

	bool can_apply() const { return issues.isEmpty() && !changes.isEmpty(); }
};

struct reassignment_preview
{
	reassignment_strategy strategy = reassignment_strategy::fill_dorms_first;
	quint32 random_seed = 0;
	int candidate_count = 0;
	int available_bed_count = 0;
	quint64 resource_signature = 0;
	QVector<accommodation_change> changes;
	QVector<int> unassigned_student_ids;
	QVector<accommodation_data_issue> issues;

	bool can_apply() const { return issues.isEmpty() && !changes.isEmpty(); }
};

struct dorm_gender_change
{
	int building_id = 0;
	int dorm_id = 0;
	int old_gender = 0;
	int new_gender = 0;
};

struct batch_clear_preview
{
	clear_scope scope = clear_scope::dorm;
	int building_id = 0;
	int dorm_id = 0;
	bool reset_gender = false;
	QVector<accommodation_change> changes;
	QVector<dorm_gender_change> dorm_changes;
	QVector<accommodation_data_issue> issues;

	bool can_apply() const { return issues.isEmpty() && !changes.isEmpty(); }
};

struct dorm_preview_state
{
	int building_id = 0;
	int dorm_id = 0;
	int gender = 0;
	QVector<int> beds;
};

struct dorm_adjustment_preview
{
	dorm_adjustment_mode mode = dorm_adjustment_mode::full_swap;
	bool available = false;
	QString unavailable_reason;
	dorm_preview_state before_a;
	dorm_preview_state before_b;
	dorm_preview_state after_a;
	dorm_preview_state after_b;
	QVector<accommodation_change> changes;
	QVector<accommodation_data_issue> issues;

	bool can_apply() const { return available && issues.isEmpty(); }
};

//学校顶层协调类。负责组合 studentmanager、dormmanager、buildingmanager
//完成涉及两个及以上同级 manager 的跨聚合业务，并向 UI 提供统一入口。
class school
{
public:
	static school& instance();//创建单例入口
	school(const school&) = delete;//禁止拷贝构造，维护单例唯一性
	school& operator=(const school&) = delete;//禁止拷贝赋值，维护单例唯一性
	persistence_start_status initialize_persistence();//启动时选择目录、加载数据或创建首次空文件
	bool resolve_persistence_conflict(bool use_executable_data);//双目录均有有效数据时由UI确认使用哪一份
	bool is_persistence_ready() const;
	bool is_read_only() const;
	QString active_data_directory() const;
	QString last_persistence_error() const;
	persistence_start_status persistence_status() const;
	QString persistence_candidate_summary(bool executable_data) const;//返回冲突候选的目录、保存时间及对象数量

	//基础只读查询入口。返回指针均指向对应 manager 容器内部本体，后续删除对应对象后会失效；
	//student 指针还可能在 studentmanager 后续 add/remove 触发 QHash rehash 后失效。
	//请就地使用，不要长期持有；需长期引用请保存各对象 id，用时重新查询。
	const student* get_student(int student_id) const;//按学号获取学生本体，不存在返回 nullptr
	const dorm* get_dorm(int building_id, int dorm_id) const;//按楼号、宿舍号获取宿舍本体，不存在返回 nullptr
	const building* get_building(int building_id) const;//按楼号获取宿舍楼本体，不存在返回 nullptr
	int get_student_count() const;//获取全校学生总数
	int get_dorm_count() const;//获取全校宿舍总数
	int get_building_count() const;//获取全校宿舍楼总数
	int get_assigned_student_count() const;//获取已入住学生数
	int get_unassigned_student_count() const;//获取未入住学生数
	int get_total_bed_count() const;//获取全校总床位数
	int get_occupied_bed_count() const;//获取全校已占床位数
	int get_empty_bed_count() const;//获取全校全部空床位数，不区分性别锁
	QVector<int> get_all_student_ids() const;//按学号升序列出全校学生
	QVector<QPair<int, int>> get_all_dorm_keys() const;//按楼号、宿舍号升序列出全校宿舍复合键
	QVector<int> get_all_building_ids() const;//按楼号升序列出全校宿舍楼
	QVector<int> get_student_ids_of_class(int class_num) const;//按学号升序列出指定班级学生，参数非法返回空表
	QVector<int> get_assigned_student_ids() const;//按学号升序列出已入住学生
	QVector<int> get_unassigned_student_ids() const;//按学号升序列出未入住学生
	QVector<QPair<int, int>> get_dorm_keys_of_building(int building_id) const;//列出指定楼全部宿舍，参数非法或楼不存在返回空表
	QVector<int> get_student_ids_of_dorm(int building_id, int dorm_id) const;//按床位顺序列出指定宿舍住客，参数非法/宿舍不存在/无住客均返回空表
	QVector<QPair<int, int>> get_available_dorm_keys(int gender) const;//列出指定性别全部可用宿舍，参数非法返回空表
	int suggest_dorm_id(int building_id, int floor) const;//建议指定楼层最小缺号宿舍，0=楼不存在/-1=参数非法/-9=本层用尽

	//学生基础资料管理
	bool add_student(const student& student_to_add);//添加学生，字段非法、携带住宿位置或学号重复时返回false
	int suggest_student_id(int grade, int class_num) const;//建议最小缺号完整学号，-1=参数或年级既有记录异常/-9=序号耗尽
	int add_student(const QString& name, int gender, int grade, int class_num, int sequence = 0);//成功返回实际学号，-1=参数非法/-2=序号占用/-3=学号冲突/-9=耗尽
	int set_student_name(int student_id, const QString& name);//修改学生姓名, 1=成功/0=学生不存在/-1=参数非法
	int change_student_academic_info(int old_student_id, int new_grade, int new_class_num);//成功返回新学号，0=不存在/-1=参数或旧记录异常/-2=冲突/-5=失败已恢复/-6=恢复不完整/-8=住宿不一致/-9=耗尽

	const dorm* get_available_dorm(int gender) const;//获取指定性别最小顺位可用宿舍
	const dorm* get_available_dorm_random(int gender) const;//随机获取指定性别可用宿舍
	int get_empty_bed_count(int gender) const;//获取指定性别全校可用空床数, -1=参数非法
	int get_empty_bed_count_of_building(int building_id, int gender) const;//获取指定楼指定性别可用空床数, -1=参数非法
	//返回值: 1=成功  0=楼号已存在  -1=参数非法
	int add_building(int building_id, int gender, int max_floor);//添加宿舍楼
	bool add_dorm(const dorm& dorm_to_add);//添加宿舍，false=字段非法/楼不存在/楼层或性别冲突/携带住客/复合键重复
	int add_dorm(int building_id, int floor, int room_num, int max_num, int gender_lock);//统一新增，成功返回宿舍号，0=楼不存在/-1=参数非法/-2=已存在/-3=性别冲突/-5=失败已恢复/-6=恢复不完整/-9=本层用尽
	//返回值: 1=成功  0=宿舍不存在  -1=参数非法  -2=缩容会丢弃已有住客
	int set_dorm_max_num(int building_id, int dorm_id, int max_num);//修改宿舍最大床位数
	bool remove_dorm(int building_id, int dorm_id);//删除宿舍并同步清退住客，false=参数非法/宿舍不存在/住宿记录不一致/删除失败
	//返回值: 1=成功  0=宿舍不存在  -1=参数非法  -2=楼不存在或不接纳该性别  -3=住客性别冲突
	int set_dorm_gender(int building_id, int dorm_id, int gender);//设置房间性别锁
	bool remove_building(int building_id);//删除宿舍楼并级联清退，false=参数非法/楼不存在/楼内记录不一致/级联删除失败
	int set_building_gender(int building_id, int gender);//修改楼性别, 1=成功/0=楼不存在/-1=参数非法/-2=既有宿舍或住客冲突
	int set_building_max_floor(int building_id, int max_floor);//修改最大楼层, 1=成功/0=楼不存在/-1=参数非法/-2=既有宿舍楼层越界

	//住宿协调
	//返回值: >0=成功(床位号)  -1=参数非法  -2=床位占用  -3=学生已有宿舍  -4=宿舍已满  -5=宿舍未配置  -6=学生不存在或性别未设置  -7=性别冲突  -8=宿舍不存在
	int assign_student_to_dorm(int building_id, int dorm_id, int student_id);//入住指定宿舍并自动分配最小空床位
	int assign_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id);//入住指定宿舍的指定床位
	int assign_student_to_available_dorm(int student_id);//入住最小顺位可用宿舍, -9=无可用宿舍
	int assign_student_to_available_dorm_random(int student_id);//随机入住可用宿舍, -9=无可用宿舍
	//返回值: >0=成功(释放的床位号)  0=学生未入住  -1=参数非法  -6=学生不存在  -8=学生记录指向的宿舍不存在或床位记录不一致
	int remove_student_from_dorm(int student_id);//退宿但保留学籍
	//调宿返回值: >0=成功(新床位号)  0=学生未入住  -1=参数非法  -2=目标床位占用  -4=目标宿舍已满
	//-6=学生不存在或性别未设置  -7=目标楼/宿舍性别冲突  -8=源或目标宿舍不存在/住宿记录不一致  -10=失败后未能恢复原床位
	int move_student_to_dorm(int building_id, int dorm_id, int student_id);//调往指定宿舍并自动分配最小空床位
	int move_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id);//调往指定宿舍的指定空床位，同宿舍时用于换床
	//返回值: 1=成功  -1=参数非法或同一学生  -2=学生不存在或性别未设置  -3=学生未入住或位置字段不完整
	//-4=源宿舍不存在或住宿记录不一致  -5=目标约束异常或执行失败  -6=失败后未能完整恢复快照
	int swap_students(int student_id1, int student_id2);//交换两名学生的床位，支持同宿舍及跨宿舍

	//学籍协调
	//返回值: 1=成功  0=学生不存在  -1=参数非法  -8=住宿记录不一致导致退宿失败
	int remove_student(int student_id);//退学籍，已入住时先退宿再删除学生本体
	//返回值: 1=成功  0=学生不存在  -1=参数非法  -7=所在楼/宿舍不接纳新性别或多人宿舍性别冲突  -8=住宿记录不一致
	int correct_student_gender(int student_id, int gender);//性别纠错，不自动迁宿

	//全校住宿分配。返回未能成功分配的人数(>=0, 0=全部安置)。
	int assign_all_students_random();//补分，只处理当前未入住学生
	int reassign_all_students_random();//重排，先清空全部宿舍并放开性别锁
	batch_assignment_preview preview_assign_unassigned_students(assignment_strategy strategy, quint32 random_seed) const;//生成未入住学生补分预览；发现住宿异常时只返回 issues
	//返回值: >0=成功安排人数  0=没有可执行安排  -5=执行失败但已撤销本次安排  -6=撤销不完整  -7=预览涉及的数据已变化  -8=现有住宿数据异常
	int apply_batch_assignment(const batch_assignment_preview& preview);//按预览中的具体床位执行补分，不重新随机
	reassignment_preview preview_reassign_all_students(reassignment_strategy strategy, quint32 random_seed) const;//生成全校重新安排预览，现有住宿异常时只返回 issues
	//返回值: >=0=成功安排人数  -5=执行失败且已恢复原住宿  -6=原住宿恢复不完整  -7=预览已失效  -8=现有住宿数据异常
	int apply_reassignment(const reassignment_preview& preview);//按预览执行全校重新安排，不重新随机

	//宿舍集合协调
	//单间清退返回值: >=0=成功(清退人数)  -1=参数非法  -8=宿舍不存在或住宿记录不一致
	int clear_dorm(int building_id, int dorm_id);//清空指定宿舍并保留房间性别锁
	int clear_dorm_reset_gender(int building_id, int dorm_id);//清空指定宿舍并放开房间性别锁
	int clear_all_dorms();//清空全部宿舍并保留房间性别锁，返回被清退学生数
	int clear_all_dorms_reset_gender();//清空全部宿舍并放开房间性别锁，返回被清退学生数
	batch_clear_preview preview_clear_dorms(clear_scope scope, int building_id, int dorm_id, bool reset_gender) const;//生成指定宿舍、指定楼栋或全部宿舍清退预览
	//返回值: >=0=清退人数  -5=执行失败且已恢复  -6=恢复不完整  -7=预览失效  -8=住宿数据异常
	int apply_batch_clear(const batch_clear_preview& preview);//按固定预览执行批量清退
	int clear_building_dorms(int building_id);//清退指定宿舍楼并保留宿舍性别锁
	int clear_building_dorms_reset_gender(int building_id);//清退指定宿舍楼并解除宿舍性别锁
	//交换返回值通用: 1=成功/-1=参数非法或同一间/-2=宿舍不存在/-3=房间锁不符/-4=人数或男女舍前提不符/-5=住客或目标约束异常/-6=失败后快照恢复不完整。
	int swap_dorms(int b1, int d1, int b2, int d2);//同锁同人数宿舍整体互换
	int swap_dorms_overlap(int b1, int d1, int b2, int d2);//重叠人数互换，多余住客留原处；一方为空时无操作成功
	int swap_dorms_overlap_evict(int b1, int d1, int b2, int d2);//重叠人数互换，多余住客离宿；一方为空时无操作成功
	int swap_gender_dorms(int b1, int d1, int b2, int d2);//混宿楼男舍与女舍互换，多余住客离宿；一方为空时无操作成功
	dorm_adjustment_preview preview_dorm_adjustment(int b1, int d1, int b2, int d2, dorm_adjustment_mode mode) const;//生成两间宿舍整体调整的固定预览
	int apply_dorm_adjustment(const dorm_adjustment_preview& preview);//重新核对预览后分发至对应宿舍调整事务
	//返回值: 1=成功  -1=样例计划非法  -5=替换失败且原数据已恢复  -6=原数据恢复不完整  -8=替换前原数据异常或无法按当前规则恢复
	int replace_all_with_sample_data(const sampledataplan& plan);//清空全部现有数据后按固定计划生成样例数据

private:
	struct dorm_accommodation_snapshot
	{
		int building_id = 0;
		int dorm_id = 0;
		int gender = 0;
		QVector<int> beds;
	};
	struct accommodation_snapshot
	{
		QVector<dorm_accommodation_snapshot> dorms;
	};
	struct school_data_snapshot
	{
		sampledataplan data;
	};
	school() = default;//构造函数，单例模式禁止外部实例化
	int clear_dorm_impl(int building_id, int dorm_id, bool reset_gender);//清空单间宿舍的共享实现
	int move_student_to_dorm_impl(int building_id, int dorm_id, int student_id, int bed_id, bool specified_bed);//调宿与换床共享实现
	bool fill_dorm(int building_id, int dorm_id, const QVector<int>& student_ids);//按顺序向宿舍回填学生
	bool is_dorm_consistent(int building_id, int dorm_id) const;//双向核对床位与学生位置字段
	QVector<accommodation_data_issue> collect_accommodation_issues() const;//逐床核对全校住宿数据并返回面向UI的异常信息
	accommodation_snapshot take_accommodation_snapshot() const;//保存全校床位与宿舍性别锁，调用前应完成一致性检查
	bool restore_accommodation_snapshot(const accommodation_snapshot& snapshot);//清空当前住宿后按原床位恢复快照
	school_data_snapshot take_school_data_snapshot() const;//保存楼栋、宿舍、学生档案和具体床位
	bool validate_sample_data_plan(const sampledataplan& plan, bool snapshot_mode = false) const;//校验样例计划；快照模式兼容核心允许的性别0和大容量宿舍
	quint64 accommodation_resource_signature() const;//汇总楼栋、宿舍、床位和学生住宿字段，用于判定固定预览是否失效
	bool purge_all_data();//事务内部清除三个manager当前全部对象
	bool write_sample_data_plan(const sampledataplan& plan);//按楼栋、空宿舍、学生、指定床位顺序写入
	bool restore_school_data_snapshot(const school_data_snapshot& snapshot);//清除残留后恢复完整原数据
	schoolsnapshot create_persistence_snapshot() const;//导出完整学校持久化快照
	bool restore_persistence_snapshot(const schoolsnapshot& snapshot);//整体恢复持久化快照并核对结果
	bool load_storage_directory(const QString& directory, bool writable, bool fallback_directory,
		persistence_start_status normal_status);//加载指定目录正式文件或备份
	bool apply_loaded_snapshot(const schoolsnapshot& snapshot);//启动加载期间暂停持久化并恢复快照
	QVector<int> snapshot_dorm(const dorm& d) const;//按床位保存宿舍快照，0表示空床
	bool restore_dorm(int building_id, int dorm_id, const QVector<int>& beds, int gender);//按床位恢复宿舍原住客与性别锁
	void reset_and_sync_students(const QVector<int>& original_ids, int b1, int d1, int b2, int d2);//按交换后两间宿舍现状同步学生位置

	schoolstorage storage;
	persistence_start_status current_persistence_status = persistence_start_status::read_only_unwritable;
	bool persistence_initialized = false;
	bool persistence_suspended = false;
	bool read_only = true;
	QString persistence_error;
	schoolsnapshot executable_candidate;
	schoolsnapshot fallback_candidate;
	bool executable_candidate_valid = false;
	bool fallback_candidate_valid = false;
	bool executable_directory_writable = false;
	bool fallback_directory_writable = false;
};

#endif // SCHOOL_H
