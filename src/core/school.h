#ifndef SCHOOL_H
#define SCHOOL_H

#include <QVector>
#include <QPair>

class student;
class dorm;
class building;
class QString;

//学校顶层协调类。负责组合 studentmanager、dormmanager、buildingmanager
//完成涉及两个及以上同级 manager 的跨聚合业务，并向 UI 提供统一入口。
class school
{
public:
	static school& instance();//创建单例入口
	school(const school&) = delete;//禁止拷贝构造，维护单例唯一性
	school& operator=(const school&) = delete;//禁止拷贝赋值，维护单例唯一性

	//基础只读查询入口。返回指针均指向对应 manager 容器内部本体，后续删除对应对象后会失效；
	//student 指针还可能在 studentmanager 后续 add/remove 触发 QHash rehash 后失效。
	//请就地使用，不要长期持有；需长期引用请保存各对象 id，用时重新查询。
	const student* get_student(int student_id) const;//按学号获取学生本体，不存在返回 nullptr
	const dorm* get_dorm(int building_id, int dorm_id) const;//按楼号、宿舍号获取宿舍本体，不存在返回 nullptr
	const building* get_building(int building_id) const;//按楼号获取宿舍楼本体，不存在返回 nullptr
	int get_student_count() const;//获取全校学生总数
	int get_dorm_count() const;//获取全校宿舍总数
	int get_building_count() const;//获取全校宿舍楼总数
	QVector<int> get_all_student_ids() const;//按学号升序列出全校学生
	QVector<QPair<int, int>> get_all_dorm_keys() const;//按楼号、宿舍号升序列出全校宿舍复合键
	QVector<int> get_all_building_ids() const;//按楼号升序列出全校宿舍楼
	QVector<int> get_student_ids_of_class(int class_num) const;//按学号升序列出指定班级学生，参数非法返回空表
	QVector<int> get_assigned_student_ids() const;//按学号升序列出已入住学生
	QVector<int> get_unassigned_student_ids() const;//按学号升序列出未入住学生
	QVector<QPair<int, int>> get_dorm_keys_of_building(int building_id) const;//列出指定楼全部宿舍，参数非法或楼不存在返回空表
	QVector<int> get_student_ids_of_dorm(int building_id, int dorm_id) const;//按床位顺序列出指定宿舍住客，宿舍不存在返回空表
	QVector<QPair<int, int>> get_available_dorm_keys(int gender) const;//列出指定性别全部可用宿舍，参数非法返回空表

	//学生基础资料管理
	bool add_student(const student& student_to_add);//添加学生，字段非法、携带住宿位置或学号重复时返回false
	int set_student_name(int student_id, const QString& name);//修改学生姓名, 1=成功/0=学生不存在/-1=参数非法
	int set_student_class_num(int student_id, int class_num);//修改学生班级, 1=成功/0=学生不存在/-1=参数非法
	int set_student_grade(int student_id, int grade);//修改学生年级, 1=成功/0=学生不存在/-1=参数非法

	const dorm* get_available_dorm(int gender) const;//获取指定性别最小顺位可用宿舍
	const dorm* get_available_dorm_random(int gender) const;//随机获取指定性别可用宿舍
	int get_empty_bed_count(int gender) const;//获取指定性别全校可用空床数, -1=参数非法
	int get_empty_bed_count_of_building(int building_id, int gender) const;//获取指定楼指定性别可用空床数, -1=参数非法
	//返回值: 1=成功  0=楼号已存在  -1=参数非法
	int add_building(int building_id, int gender, int max_floor);//添加宿舍楼
	bool add_dorm(const dorm& dorm_to_add);//添加宿舍并校验所在楼存在及派生楼层不越界
	//返回值: 1=成功  0=宿舍不存在  -1=参数非法  -2=缩容会丢弃已有住客
	int set_dorm_max_num(int building_id, int dorm_id, int max_num);//修改宿舍最大床位数
	bool remove_dorm(int building_id, int dorm_id);//删除宿舍并同步清退住客
	//返回值: 1=成功  0=宿舍不存在  -1=参数非法  -2=楼不存在或不接纳该性别  -3=住客性别冲突
	int set_dorm_gender(int building_id, int dorm_id, int gender);//设置房间性别锁
	bool remove_building(int building_id);//删除宿舍楼并级联删除楼内宿舍、清退住客
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

	//学籍协调
	//返回值: 1=成功  0=学生不存在  -1=参数非法  -8=住宿记录不一致导致退宿失败
	int remove_student(int student_id);//退学籍，已入住时先退宿再删除学生本体
	//返回值: 1=成功  0=学生不存在  -1=参数非法  -7=所在楼/宿舍不接纳新性别或多人宿舍性别冲突  -8=住宿记录不一致
	int correct_student_gender(int student_id, int gender);//性别纠错，不自动迁宿

	//全校住宿分配。返回未能成功分配的人数(>=0, 0=全部安置)。
	int assign_all_students_random();//补分，只处理当前未入住学生
	int reassign_all_students_random();//重排，先清空全部宿舍并放开性别锁

	//宿舍集合协调
	int clear_all_dorms();//清空全部宿舍并保留房间性别锁，返回被清退学生数
	int clear_all_dorms_reset_gender();//清空全部宿舍并放开房间性别锁，返回被清退学生数
	//交换返回值通用: 1=成功/-1=参数非法或同一间/-2=宿舍不存在/-3=房间锁不符/-4=人数或男女舍前提不符/-5=住客或目标约束异常/-6=失败后快照恢复不完整。
	int swap_dorms(int b1, int d1, int b2, int d2);//同锁同人数宿舍整体互换
	int swap_dorms_overlap(int b1, int d1, int b2, int d2);//重叠人数互换，多余住客留原处；一方为空时无操作成功
	int swap_dorms_overlap_evict(int b1, int d1, int b2, int d2);//重叠人数互换，多余住客离宿；一方为空时无操作成功
	int swap_gender_dorms(int b1, int d1, int b2, int d2);//混宿楼男舍与女舍互换，多余住客离宿；一方为空时无操作成功

private:
	school() = default;//构造函数，单例模式禁止外部实例化
	bool fill_dorm(int building_id, int dorm_id, const QVector<int>& student_ids);//按顺序向宿舍回填学生
	bool is_dorm_consistent(int building_id, int dorm_id) const;//双向核对床位与学生位置字段
	QVector<int> snapshot_dorm(const dorm& d) const;//按床位保存宿舍快照，0表示空床
	bool restore_dorm(int building_id, int dorm_id, const QVector<int>& beds, int gender);//按床位恢复宿舍原住客与性别锁
	void reset_and_sync_students(const QVector<int>& original_ids, int b1, int d1, int b2, int d2);//按交换后两间宿舍现状同步学生位置
};

#endif // SCHOOL_H
