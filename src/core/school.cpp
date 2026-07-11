#include "school.h"
#include "studentmanager.h"
#include "dormmanager.h"
#include "buildingmanager.h"
#include "system/check.h"
#include <QRandomGenerator>
#include <algorithm>

school& school::instance()
{
	static school s;
	return s;
}

const student* school::get_student(int student_id) const//按学号获取学生本体
{
	return studentmanager::instance().get(student_id);
}

const dorm* school::get_dorm(int building_id, int dorm_id) const//按楼号、宿舍号获取宿舍本体
{
	return dormmanager::instance().get(building_id, dorm_id);
}

const building* school::get_building(int building_id) const//按楼号获取宿舍楼本体
{
	return buildingmanager::instance().get(building_id);
}

int school::get_student_count() const//获取全校学生总数
{
	return studentmanager::instance().count();
}

int school::get_dorm_count() const//获取全校宿舍总数
{
	return dormmanager::instance().count();
}

int school::get_building_count() const//获取全校宿舍楼总数
{
	return buildingmanager::instance().count();
}

int school::get_assigned_student_count() const//获取已入住学生数
{
	return get_assigned_student_ids().size();
}

int school::get_unassigned_student_count() const//获取未入住学生数
{
	return get_unassigned_student_ids().size();
}

int school::get_total_bed_count() const//获取全校总床位数
{
	int total = 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d != nullptr)
			total += d->get_max_num();
	}
	return total;
}

int school::get_occupied_bed_count() const//获取全校已占床位数
{
	int total = 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d != nullptr)
			total += d->get_current_num();
	}
	return total;
}

int school::get_empty_bed_count() const//获取全校全部空床位数
{
	return dormmanager::instance().get_empty_bed_count();
}

QVector<int> school::get_all_student_ids() const//按学号升序列出全校学生
{
	QVector<int> ids = studentmanager::instance().all_ids();
	std::sort(ids.begin(), ids.end());
	return ids;
}

QVector<QPair<int, int>> school::get_all_dorm_keys() const//列出全校宿舍复合键
{
	return dormmanager::instance().all_dorm_keys();
}

QVector<int> school::get_all_building_ids() const//按楼号升序列出全校宿舍楼
{
	return buildingmanager::instance().all_ids();
}

QVector<int> school::get_student_ids_of_class(int class_num) const//列出指定班级学生
{
	if (!check::is_valid_class_num(class_num))
		return {};
	QVector<int> ids = studentmanager::instance().ids_of_class(class_num);
	std::sort(ids.begin(), ids.end());
	return ids;
}

QVector<int> school::get_assigned_student_ids() const//列出已入住学生
{
	QVector<int> ids;
	for (int student_id : get_all_student_ids())
		if (studentmanager::instance().is_student_have_dorm(student_id) == 1)
			ids.append(student_id);
	return ids;
}

QVector<int> school::get_unassigned_student_ids() const//列出未入住学生
{
	QVector<int> ids;
	for (int student_id : get_all_student_ids())
		if (studentmanager::instance().is_student_have_dorm(student_id) == 0)
			ids.append(student_id);
	return ids;
}

QVector<QPair<int, int>> school::get_dorm_keys_of_building(int building_id) const//列出指定楼全部宿舍
{
	QVector<QPair<int, int>> keys;
	if (!check::is_valid_building_id(building_id) || buildingmanager::instance().get(building_id) == nullptr)
		return keys;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
		if (key.first == building_id)
			keys.append(key);
	return keys;
}

QVector<int> school::get_student_ids_of_dorm(int building_id, int dorm_id) const//列出指定宿舍住客
{
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	return d == nullptr ? QVector<int>{} : d->get_student_id_list();
}

QVector<QPair<int, int>> school::get_available_dorm_keys(int gender) const//列出指定性别全部可用宿舍
{
	QVector<QPair<int, int>> keys;
	if (gender != 1 && gender != 2)
		return keys;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const building* b = buildingmanager::instance().get(key.first);
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (b != nullptr && d != nullptr && b->accepts_gender(gender) && d->accepts_gender(gender) && !d->is_full())
			keys.append(key);
	}
	return keys;
}

bool school::add_student(const student& student_to_add)//添加学生
{
	return studentmanager::instance().add(student_to_add);
}

int school::set_student_name(int student_id, const QString& name)//修改学生姓名
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	return studentmanager::instance().set_student_name(student_id, name);
}

int school::set_student_class_num(int student_id, int class_num)//修改学生班级
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	return studentmanager::instance().set_student_class_num(student_id, class_num);
}

int school::set_student_grade(int student_id, int grade)//修改学生年级
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	return studentmanager::instance().set_student_grade(student_id, grade);
}

const dorm* school::get_available_dorm(int gender) const//获取指定性别最小顺位可用宿舍
{
	QVector<QPair<int, int>> keys = get_available_dorm_keys(gender);
	return keys.isEmpty() ? nullptr : dormmanager::instance().get(keys.first().first, keys.first().second);
}

const dorm* school::get_available_dorm_random(int gender) const//随机获取指定性别可用宿舍
{
	QVector<QPair<int, int>> candidates = get_available_dorm_keys(gender);
	if (candidates.isEmpty())
		return nullptr;
	const auto& key = candidates[QRandomGenerator::global()->bounded(candidates.size())];
	return dormmanager::instance().get(key.first, key.second);
}

int school::get_empty_bed_count(int gender) const//获取指定性别全校可用空床数
{
	if (gender != 1 && gender != 2)
		return -1;
	int total = 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const building* b = buildingmanager::instance().get(key.first);
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (b != nullptr && d != nullptr && b->accepts_gender(gender) && d->accepts_gender(gender))
			total += d->get_empty_count();
	}
	return total;
}

int school::get_empty_bed_count_of_building(int building_id, int gender) const//获取指定楼指定性别可用空床数
{
	if (!check::is_valid_building_id(building_id) || (gender != 1 && gender != 2))
		return -1;
	const building* b = buildingmanager::instance().get(building_id);
	if (b == nullptr || !b->accepts_gender(gender))
		return 0;
	int total = 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		if (key.first != building_id)
			continue;
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d != nullptr && d->accepts_gender(gender))
			total += d->get_empty_count();
	}
	return total;
}

int school::add_building(int building_id, int gender, int max_floor)//添加宿舍楼
{
	return buildingmanager::instance().add_building(building_id, gender, max_floor);
}

bool school::add_dorm(const dorm& dorm_to_add)//添加宿舍并校验楼级约束
{
	if (!dorm_to_add.is_empty())
		return false;//禁止携带预填住客入库，所有入住必须经过school闭环
	const building* b = buildingmanager::instance().get(dorm_to_add.get_building_id());
	if (b == nullptr || !check::is_valid_dorm_floor(dorm_to_add.get_id(), b->get_max_floor()))
		return false;
	if (dorm_to_add.get_for_gender() != 0 && !b->accepts_gender(dorm_to_add.get_for_gender()))
		return false;//空宿舍预设性别锁必须被所在楼接纳
	return dormmanager::instance().add_dorm(dorm_to_add);
}

int school::set_dorm_max_num(int building_id, int dorm_id, int max_num)//修改宿舍最大床位数
{
	return dormmanager::instance().set_dorm_max_num(building_id, dorm_id, max_num);
}

bool school::is_dorm_consistent(int building_id, int dorm_id) const//双向核对床位与学生位置字段
{
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	if (d == nullptr)
		return false;
	for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
	{
		int student_id = d->get_student_id(bed_id);
		if (student_id < 1)
			continue;
		const student* s = studentmanager::instance().get(student_id);
		if (s == nullptr || s->get_building_id() != building_id || s->get_dorm_id() != dorm_id || s->get_bed_id() != bed_id || s->get_floor() != d->get_floor())
			return false;
	}
	for (int student_id : studentmanager::instance().all_ids())
	{
		const student* s = studentmanager::instance().get(student_id);
		if (s->get_building_id() == building_id && s->get_dorm_id() == dorm_id && d->get_student_id(s->get_bed_id()) != student_id)
			return false;
	}
	return true;
}

bool school::remove_dorm(int building_id, int dorm_id)//删除宿舍并同步清退住客
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id))
		return false;
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	if (d == nullptr)
		return false;
	if (!is_dorm_consistent(building_id, dorm_id))
		return false;
	QVector<int> student_ids;
	for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
	{
		int student_id = d->get_student_id(bed_id);
		if (student_id < 1)
			continue;
		student_ids.append(student_id);
	}
	if (!dormmanager::instance().remove_dorm(building_id, dorm_id))
		return false;
	for (int student_id : student_ids)
		studentmanager::instance().clear_dorm_info(student_id);
	return true;
}

bool school::remove_building(int building_id)//删除宿舍楼并级联处理楼内宿舍
{
	if (!check::is_valid_building_id(building_id) || buildingmanager::instance().get(building_id) == nullptr)
		return false;
	QVector<QPair<int, int>> keys = dormmanager::instance().all_dorm_keys();
	for (const auto& key : keys)
		if (key.first == building_id && !is_dorm_consistent(key.first, key.second))
			return false;//先全量预检，避免级联删除部分提交
	for (const auto& key : keys)
		if (key.first == building_id && !remove_dorm(key.first, key.second))
			return false;
	return buildingmanager::instance().remove_building(building_id);
}

int school::set_building_gender(int building_id, int gender)//修改楼适用性别
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_building_gender(gender))
		return -1;
	if (buildingmanager::instance().get(building_id) == nullptr)
		return 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		if (key.first != building_id)
			continue;
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d == nullptr)
			continue;
		if (d->get_for_gender() != 0 && gender != 3 && d->get_for_gender() != gender)
			return -2;//房间性别锁与新楼性别冲突
		for (int student_id : d->get_student_id_list())
		{
			const student* s = studentmanager::instance().get(student_id);
			if (s == nullptr || (gender != 3 && s->get_gender() != gender))
				return -2;//住客不存在或性别与新楼性别冲突
		}
	}
	return buildingmanager::instance().set_building_gender(building_id, gender);
}

int school::set_building_max_floor(int building_id, int max_floor)//修改楼最大楼层
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_max_floor(max_floor))
		return -1;
	if (buildingmanager::instance().get(building_id) == nullptr)
		return 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
		if (key.first == building_id && !check::is_valid_dorm_floor(key.second, max_floor))
			return -2;//既有宿舍派生楼层超出新上限
	return buildingmanager::instance().set_building_max_floor(building_id, max_floor);
}

int school::set_dorm_gender(int building_id, int dorm_id, int gender)//设置房间性别锁
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_gender(gender))
		return -1;
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	if (d == nullptr)
		return 0;
	if (gender == 0 && !d->is_empty())
		return -3;//有住客时禁止解锁，防止后续异性入住形成混住
	if (gender != 0)
	{
		const building* b = buildingmanager::instance().get(building_id);
		if (b == nullptr || !b->accepts_gender(gender))
			return -2;
		for (int student_id : d->get_student_id_list())
		{
			const student* s = studentmanager::instance().get(student_id);
			if (s == nullptr || s->get_gender() != gender)
				return -3;
		}
	}
	return dormmanager::instance().set_dorm_gender(building_id, dorm_id, gender);
}

int school::assign_student_to_dorm(int building_id, int dorm_id, int student_id)//入住指定宿舍并自动分配最小空床位
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id))
		return -1;//参数非法
	studentmanager& sm = studentmanager::instance();
	const student* s = sm.get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;//学生不存在或性别未设置
	if (sm.is_student_have_dorm(student_id) == 1)
		return -3;//学生已有宿舍
	const building* b = buildingmanager::instance().get(building_id);
	if (b == nullptr || !b->accepts_gender(s->get_gender()))
		return -7;//宿舍楼不存在或不接纳该学生性别

	int bed_id = dormmanager::instance().add_student_to_dorm(building_id, dorm_id, student_id, s->get_gender());
	if (bed_id > 0)
		sm.assign_dorm_info(student_id, bed_id, dorm_id, building_id, dorm_id / 100);
	return bed_id;
}

int school::assign_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id)//入住指定宿舍的指定床位
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id))
		return -1;//参数非法
	const dorm* target = dormmanager::instance().get(building_id, dorm_id);
	if (target == nullptr)
		return -8;//宿舍不存在
	if (!check::is_valid_bed_id(bed_id, target->get_max_num()))
		return -1;//bed_id非法
	if (target->is_bed_occupied(bed_id) == 1)
		return -2;//床位占用
	studentmanager& sm = studentmanager::instance();
	const student* s = sm.get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;//学生不存在或性别未设置
	if (sm.is_student_have_dorm(student_id) == 1)
		return -3;//学生已有宿舍
	const building* b = buildingmanager::instance().get(building_id);
	if (b == nullptr || !b->accepts_gender(s->get_gender()))
		return -7;//宿舍楼不存在或不接纳该学生性别

	int result = dormmanager::instance().add_student_to_dorm(building_id, dorm_id, student_id, s->get_gender(), bed_id);
	if (result > 0)
		sm.assign_dorm_info(student_id, bed_id, dorm_id, building_id, dorm_id / 100);
	return result;
}

int school::assign_student_to_available_dorm(int student_id)//入住最小顺位可用宿舍
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	const student* s = studentmanager::instance().get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;//学生不存在或性别未设置
	if (studentmanager::instance().is_student_have_dorm(student_id) == 1)
		return -3;//学生已有宿舍
	const dorm* available = get_available_dorm(s->get_gender());
	if (available == nullptr)
		return -9;//无可用宿舍
	return assign_student_to_dorm(available->get_building_id(), available->get_id(), student_id);
}

int school::assign_student_to_available_dorm_random(int student_id)//随机入住可用宿舍
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	const student* s = studentmanager::instance().get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;//学生不存在或性别未设置
	if (studentmanager::instance().is_student_have_dorm(student_id) == 1)
		return -3;//学生已有宿舍
	const dorm* available = get_available_dorm_random(s->get_gender());
	if (available == nullptr)
		return -9;//无可用宿舍
	return assign_student_to_dorm(available->get_building_id(), available->get_id(), student_id);
}

int school::remove_student_from_dorm(int student_id)//退宿但保留学籍
{
	if (!check::is_valid_student_id(student_id))
		return -1;//参数非法
	studentmanager& sm = studentmanager::instance();
	const student* s = sm.get(student_id);
	if (s == nullptr)
		return -6;//学生不存在
	if (sm.is_student_have_dorm(student_id) == 0)
		return 0;//学生未入住

	int building_id = s->get_building_id();
	int dorm_id = s->get_dorm_id();
	int recorded_bed_id = s->get_bed_id();
	const dorm* target = dormmanager::instance().get(building_id, dorm_id);
	if (target == nullptr || target->get_student_id(recorded_bed_id) != student_id)
		return -8;//宿舍不存在或床位记录不一致
	int bed_id = dormmanager::instance().remove_student_from_dorm(building_id, dorm_id, student_id);
	if (bed_id <= 0)
		return -8;//宿舍不存在或床位记录不一致
	sm.clear_dorm_info(student_id);
	return bed_id;
}

int school::move_student_to_dorm_impl(int building_id, int dorm_id, int student_id, int bed_id, bool specified_bed)//调宿与换床共享实现
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id))
		return -1;
	studentmanager& sm = studentmanager::instance();
	dormmanager& dm = dormmanager::instance();
	const student* s = sm.get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;
	if (sm.is_student_have_dorm(student_id) == 0)
		return 0;

	int old_building_id = s->get_building_id();
	int old_dorm_id = s->get_dorm_id();
	int old_bed_id = s->get_bed_id();
	const dorm* source = dm.get(old_building_id, old_dorm_id);
	const dorm* target = dm.get(building_id, dorm_id);
	if (source == nullptr || target == nullptr || !is_dorm_consistent(old_building_id, old_dorm_id) ||
		((old_building_id != building_id || old_dorm_id != dorm_id) && !is_dorm_consistent(building_id, dorm_id)))
		return -8;
	if (specified_bed && !check::is_valid_bed_id(bed_id, target->get_max_num()))
		return -1;

	bool same_dorm = old_building_id == building_id && old_dorm_id == dorm_id;
	if (same_dorm)
	{
		if (!specified_bed || bed_id == old_bed_id)
			return old_bed_id;
		if (target->is_bed_occupied(bed_id) == 1)
			return -2;
		if (dm.move_student_bed(building_id, dorm_id, old_bed_id, bed_id) != 1)
			return -8;
		return sm.assign_dorm_info(student_id, bed_id, dorm_id, building_id, target->get_floor()) == 1 ? bed_id : -8;
	}

	const building* target_building = buildingmanager::instance().get(building_id);
	if (target_building == nullptr || !target_building->accepts_gender(s->get_gender()) || !target->accepts_gender(s->get_gender()))
		return -7;
	if (specified_bed && target->is_bed_occupied(bed_id) == 1)
		return -2;
	if (!specified_bed && target->is_full())
		return -4;

	int old_target_gender = target->get_for_gender();
	if (dm.remove_student_from_dorm(old_building_id, old_dorm_id, student_id) != old_bed_id)
		return -8;
	int new_bed_id = specified_bed ?
		dm.add_student_to_dorm(building_id, dorm_id, student_id, s->get_gender(), bed_id) :
		dm.add_student_to_dorm(building_id, dorm_id, student_id, s->get_gender());
	if (new_bed_id <= 0)
	{
		int restored = dm.add_student_to_dorm(old_building_id, old_dorm_id, student_id, s->get_gender(), old_bed_id);
		return restored == old_bed_id ? new_bed_id : -10;
	}
	if (sm.assign_dorm_info(student_id, new_bed_id, dorm_id, building_id, target->get_floor()) != 1)
	{
		bool removed_target = dm.remove_student_from_dorm(building_id, dorm_id, student_id) == new_bed_id;
		bool restored_source = dm.add_student_to_dorm(old_building_id, old_dorm_id, student_id, s->get_gender(), old_bed_id) == old_bed_id;
		bool restored_gender = dm.set_dorm_gender(building_id, dorm_id, old_target_gender) == 1;
		return removed_target && restored_source && restored_gender ? -8 : -10;
	}
	return new_bed_id;
}

int school::move_student_to_dorm(int building_id, int dorm_id, int student_id)//调往指定宿舍并自动分配床位
{
	return move_student_to_dorm_impl(building_id, dorm_id, student_id, 0, false);
}

int school::move_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id)//调往指定宿舍的指定床位
{
	return move_student_to_dorm_impl(building_id, dorm_id, student_id, bed_id, true);
}

int school::remove_student(int student_id)//退学籍
{
	if (!check::is_valid_student_id(student_id))
		return -1;//参数非法
	studentmanager& sm = studentmanager::instance();
	const student* s = sm.get(student_id);
	if (s == nullptr)
		return 0;//学生不存在
	bool all_empty = s->get_bed_id() == 0 && s->get_dorm_id() == 0 && s->get_building_id() == 0 && s->get_floor() == 0;
	bool all_assigned = s->get_bed_id() > 0 && s->get_dorm_id() > 0 && s->get_building_id() > 0 && s->get_floor() > 0;
	if (!all_empty && !all_assigned)
		return -8;//住宿位置字段部分缺失，禁止删除学生本体
	if (all_assigned)
	{
		int result = remove_student_from_dorm(student_id);
		if (result <= 0)
			return -8;//住宿记录不一致，禁止只删除学生本体
	}
	return sm.remove(student_id) ? 1 : 0;
}

int school::correct_student_gender(int student_id, int gender)//性别纠错
{
	if (!check::is_valid_student_id(student_id) || gender < 1 || gender > 2)
		return -1;//参数非法，纠错后的性别只接受1/2
	studentmanager& sm = studentmanager::instance();
	const student* s = sm.get(student_id);
	if (s == nullptr)
		return 0;//学生不存在
	if (s->get_gender() == gender)
		return 1;//已是目标性别，无需修改
	if (sm.is_student_have_dorm(student_id) == 0)
		return sm.set_student_gender(student_id, gender);//未入住可直接纠正

	int building_id = s->get_building_id();
	int dorm_id = s->get_dorm_id();
	int bed_id = s->get_bed_id();
	const building* b = buildingmanager::instance().get(building_id);
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	if (b == nullptr || d == nullptr || d->get_student_id(bed_id) != student_id)
		return -8;//住宿记录不一致
	if (!b->accepts_gender(gender))
		return -7;//所在楼不接纳新性别
	if (d->get_for_gender() == gender)
		return sm.set_student_gender(student_id, gender);//房间未锁定或已匹配新性别
	if (d->get_for_gender() == 0)
	{
		for (int occupant_id : d->get_student_id_list())
		{
			const student* occupant = sm.get(occupant_id);
			if (occupant == nullptr)
				return -8;//床位存在幽灵学号
			if (occupant_id != student_id && occupant->get_gender() != gender)
				return -7;//其他住客性别与纠错目标冲突
		}
		int old_gender = s->get_gender();
		if (sm.set_student_gender(student_id, gender) != 1)
			return -1;
		int result = set_dorm_gender(building_id, dorm_id, gender);
		if (result != 1)
		{
			sm.set_student_gender(student_id, old_gender);
			return result == 0 ? -8 : -7;
		}
		return 1;
	}
	if (d->get_current_num() != 1)
		return -7;//多人宿舍不能只纠正一人的性别并改变房间锁

	int old_gender = s->get_gender();
	if (sm.set_student_gender(student_id, gender) != 1)
		return -1;
	int result = set_dorm_gender(building_id, dorm_id, gender);
	if (result != 1)
	{
		sm.set_student_gender(student_id, old_gender);//房间锁修改失败，恢复学生原性别
		return result == 0 ? -8 : -7;
	}
	return 1;
}

int school::assign_all_students_random()//为当前未入住学生随机补分宿舍
{
	QVector<int> ids = studentmanager::instance().all_ids();
	for (int i = ids.size() - 1; i > 0; --i)
	{
		int j = QRandomGenerator::global()->bounded(i + 1);
		int tmp = ids[i];
		ids[i] = ids[j];
		ids[j] = tmp;
	}
	int failed = 0;
	for (int student_id : ids)
	{
		if (studentmanager::instance().is_student_have_dorm(student_id) == 1)
			continue;//已有宿舍者跳过
		if (assign_student_to_available_dorm_random(student_id) <= 0)
			++failed;
	}
	return failed;
}

int school::reassign_all_students_random()//清空后为全校学生随机重排宿舍
{
	clear_all_dorms_reset_gender();
	//再以学生本体表为准统一清零，修复可能存在的“位置字段有值但 beds 无记录”异常状态。
	for (int student_id : studentmanager::instance().all_ids())
		studentmanager::instance().clear_dorm_info(student_id);
	return assign_all_students_random();
}

int school::clear_dorm_impl(int building_id, int dorm_id, bool reset_gender)//清空单间宿舍的共享实现
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id))
		return -1;
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	if (d == nullptr || !is_dorm_consistent(building_id, dorm_id))
		return -8;
	QVector<int> student_ids = d->get_student_id_list();
	int cleared = dormmanager::instance().clear_dorm_students(building_id, dorm_id, reset_gender);
	if (cleared != student_ids.size())
		return -8;
	for (int student_id : student_ids)
		if (studentmanager::instance().clear_dorm_info(student_id) != 1)
			return -8;
	return cleared;
}

int school::clear_dorm(int building_id, int dorm_id)//清空指定宿舍并保留房间性别锁
{
	return clear_dorm_impl(building_id, dorm_id, false);
}

int school::clear_dorm_reset_gender(int building_id, int dorm_id)//清空指定宿舍并放开房间性别锁
{
	return clear_dorm_impl(building_id, dorm_id, true);
}

int school::clear_all_dorms()//清空全部宿舍并保留房间性别锁
{
	int cleared = 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d == nullptr)
			continue;
		QVector<int> ids = d->get_student_id_list();
		cleared += ids.size();
		dormmanager::instance().clear_dorm_students(key.first, key.second, false);
	}
	for (int student_id : studentmanager::instance().all_ids())
		studentmanager::instance().clear_dorm_info(student_id);//同时修复student指向空床的反向不一致
	return cleared;
}

int school::clear_all_dorms_reset_gender()//清空全部宿舍并放开房间性别锁
{
	int cleared = 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d == nullptr)
			continue;
		QVector<int> ids = d->get_student_id_list();
		cleared += ids.size();
		dormmanager::instance().clear_dorm_students(key.first, key.second, true);
	}
	for (int student_id : studentmanager::instance().all_ids())
		studentmanager::instance().clear_dorm_info(student_id);//同时修复student指向空床的反向不一致
	return cleared;
}

bool school::fill_dorm(int building_id, int dorm_id, const QVector<int>& student_ids)//按顺序向宿舍回填学生
{
	for (int student_id : student_ids)
	{
		const student* s = studentmanager::instance().get(student_id);
		if (s == nullptr || s->get_gender() == 0 ||
			dormmanager::instance().add_student_to_dorm(building_id, dorm_id, student_id, s->get_gender()) <= 0)
			return false;
	}
	return true;
}

QVector<int> school::snapshot_dorm(const dorm& d) const//按床位保存宿舍快照
{
	QVector<int> beds;
	for (int bed_id = 1; bed_id <= d.get_max_num(); ++bed_id)
	{
		int student_id = d.get_student_id(bed_id);
		beds.append(student_id > 0 ? student_id : 0);
	}
	return beds;
}

bool school::restore_dorm(int building_id, int dorm_id, const QVector<int>& beds, int gender)//按床位恢复宿舍原状态
{
	if (dormmanager::instance().clear_dorm_students(building_id, dorm_id, true) < 0)
		return false;
	for (int i = 0; i < beds.size(); ++i)
	{
		if (beds[i] == 0)
			continue;
		const student* s = studentmanager::instance().get(beds[i]);
		if (s == nullptr || s->get_gender() == 0 ||
			dormmanager::instance().add_student_to_dorm(building_id, dorm_id, beds[i], s->get_gender(), i + 1) <= 0)
			return false;
	}
	return dormmanager::instance().set_dorm_gender(building_id, dorm_id, gender) == 1;
}

void school::reset_and_sync_students(const QVector<int>& original_ids, int b1, int d1, int b2, int d2)//按交换后状态同步学生位置
{
	studentmanager& sm = studentmanager::instance();
	for (int student_id : original_ids)
		sm.clear_dorm_info(student_id);
	for (const auto& key : QVector<QPair<int, int>>{qMakePair(b1, d1), qMakePair(b2, d2)})
	{
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (d == nullptr)
			continue;
		for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
		{
			int student_id = d->get_student_id(bed_id);
			if (student_id > 0)
				sm.assign_dorm_info(student_id, bed_id, d->get_id(), d->get_building_id(), d->get_floor());
		}
	}
}

int school::swap_dorms(int b1, int d1, int b2, int d2)//同锁同人数宿舍整体互换
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) || !check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2) || (b1 == b2 && d1 == d2))
		return -1;
	const dorm* A = get_dorm(b1, d1);
	const dorm* B = get_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;
	if (A->get_for_gender() != B->get_for_gender())
		return -3;
	if (A->get_current_num() != B->get_current_num())
		return -4;
	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	QVector<int> bedsA = snapshot_dorm(*A);
	QVector<int> bedsB = snapshot_dorm(*B);
	int genderA = A->get_for_gender();
	int genderB = B->get_for_gender();
	const building* buildingA = get_building(b1);
	const building* buildingB = get_building(b2);
	for (int id : listA)
	{
		const student* s = get_student(id);
		if (s == nullptr || buildingB == nullptr || !buildingB->accepts_gender(s->get_gender()) || !B->accepts_gender(s->get_gender()))
			return -5;
	}
	for (int id : listB)
	{
		const student* s = get_student(id);
		if (s == nullptr || buildingA == nullptr || !buildingA->accepts_gender(s->get_gender()) || !A->accepts_gender(s->get_gender()))
			return -5;
	}
	dormmanager::instance().clear_dorm_students(b1, d1, false);
	dormmanager::instance().clear_dorm_students(b2, d2, false);
	if (!fill_dorm(b2, d2, listA) || !fill_dorm(b1, d1, listB))
	{
		bool restoredA = restore_dorm(b1, d1, bedsA, genderA);
		bool restoredB = restore_dorm(b2, d2, bedsB, genderB);
		bool restored = restoredA && restoredB;
		QVector<int> original_ids = listA;
		original_ids += listB;
		reset_and_sync_students(original_ids, b1, d1, b2, d2);
		return restored ? -5 : -6;
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return 1;
}

int school::swap_dorms_overlap(int b1, int d1, int b2, int d2)//重叠人数互换，多余住客留原处
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) || !check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2) || (b1 == b2 && d1 == d2))
		return -1;
	const dorm* A = get_dorm(b1, d1);
	const dorm* B = get_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;
	if (A->get_for_gender() != B->get_for_gender())
		return -3;
	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	int k = qMin(listA.size(), listB.size());
	if (k == 0)
		return 1;
	QVector<int> bedsA = snapshot_dorm(*A);
	QVector<int> bedsB = snapshot_dorm(*B);
	int genderA = A->get_for_gender();
	int genderB = B->get_for_gender();
	for (int id : listA + listB)
	{
		const student* s = get_student(id);
		if (s == nullptr || s->get_gender() == 0)
			return -5;//快照住客异常，无法保证失败恢复
	}
	for (int i = 0; i < k; ++i)
	{
		const student* sA = get_student(listA[i]);
		const student* sB = get_student(listB[i]);
		const building* buildingA = get_building(b1);
		const building* buildingB = get_building(b2);
		if (sA == nullptr || sB == nullptr || buildingA == nullptr || buildingB == nullptr ||
			!buildingB->accepts_gender(sA->get_gender()) || !B->accepts_gender(sA->get_gender()) ||
			!buildingA->accepts_gender(sB->get_gender()) || !A->accepts_gender(sB->get_gender()))
			return -5;
	}
	for (int i = 0; i < k; ++i)
	{
		dormmanager::instance().remove_student_from_dorm(b1, d1, listA[i]);
		dormmanager::instance().remove_student_from_dorm(b2, d2, listB[i]);
	}
	bool success = true;
	for (int i = 0; i < k && success; ++i)
	{
		const student* sA = get_student(listA[i]);
		const student* sB = get_student(listB[i]);
		success = dormmanager::instance().add_student_to_dorm(b2, d2, listA[i], sA->get_gender()) > 0 &&
			dormmanager::instance().add_student_to_dorm(b1, d1, listB[i], sB->get_gender()) > 0;
	}
	if (!success)
	{
		bool restoredA = restore_dorm(b1, d1, bedsA, genderA);
		bool restoredB = restore_dorm(b2, d2, bedsB, genderB);
		bool restored = restoredA && restoredB;
		QVector<int> original_ids = listA;
		original_ids += listB;
		reset_and_sync_students(original_ids, b1, d1, b2, d2);
		return restored ? -5 : -6;
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return 1;
}

int school::swap_dorms_overlap_evict(int b1, int d1, int b2, int d2)//重叠人数互换，多余住客离宿
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) || !check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2) || (b1 == b2 && d1 == d2))
		return -1;
	const dorm* A = get_dorm(b1, d1);
	const dorm* B = get_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;
	if (A->get_for_gender() != B->get_for_gender())
		return -3;
	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	int k = qMin(listA.size(), listB.size());
	if (k == 0)
		return 1;
	QVector<int> bedsA = snapshot_dorm(*A);
	QVector<int> bedsB = snapshot_dorm(*B);
	int genderA = A->get_for_gender();
	int genderB = B->get_for_gender();
	for (int id : listA + listB)
	{
		const student* s = get_student(id);
		if (s == nullptr || s->get_gender() == 0)
			return -5;//快照住客异常，无法保证失败恢复
	}
	const building* buildingA = get_building(b1);
	const building* buildingB = get_building(b2);
	for (int i = 0; i < k; ++i)
	{
		const student* sA = get_student(listA[i]);
		const student* sB = get_student(listB[i]);
		if (buildingA == nullptr || buildingB == nullptr ||
			!buildingB->accepts_gender(sA->get_gender()) || !B->accepts_gender(sA->get_gender()) ||
			!buildingA->accepts_gender(sB->get_gender()) || !A->accepts_gender(sB->get_gender()))
			return -5;
	}
	dormmanager::instance().clear_dorm_students(b1, d1, false);
	dormmanager::instance().clear_dorm_students(b2, d2, false);
	QVector<int> moveA = listA.mid(0, k);
	QVector<int> moveB = listB.mid(0, k);
	bool success = fill_dorm(b2, d2, moveA) && fill_dorm(b1, d1, moveB);
	if (!success)
	{
		bool restoredA = restore_dorm(b1, d1, bedsA, genderA);
		bool restoredB = restore_dorm(b2, d2, bedsB, genderB);
		bool restored = restoredA && restoredB;
		QVector<int> original_ids = listA;
		original_ids += listB;
		reset_and_sync_students(original_ids, b1, d1, b2, d2);
		return restored ? -5 : -6;
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return 1;
}

int school::swap_gender_dorms(int b1, int d1, int b2, int d2)//混宿楼男舍与女舍互换
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) || !check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2) || (b1 == b2 && d1 == d2))
		return -1;
	const dorm* A = get_dorm(b1, d1);
	const dorm* B = get_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;
	const building* buildingA = get_building(b1);
	const building* buildingB = get_building(b2);
	if (buildingA == nullptr || buildingB == nullptr || buildingA->get_for_gender() != 3 || buildingB->get_for_gender() != 3)
		return -3;
	int genderA = A->get_for_gender();
	int genderB = B->get_for_gender();
	if (!((genderA == 1 && genderB == 2) || (genderA == 2 && genderB == 1)))
		return -4;
	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	int k = qMin(listA.size(), listB.size());
	if (k == 0)
		return 1;
	QVector<int> bedsA = snapshot_dorm(*A);
	QVector<int> bedsB = snapshot_dorm(*B);
	for (int id : listA + listB)
	{
		const student* s = get_student(id);
		if (s == nullptr || s->get_gender() == 0)
			return -5;
	}
	dormmanager::instance().clear_dorm_students(b1, d1, true);
	dormmanager::instance().clear_dorm_students(b2, d2, true);
	bool success = fill_dorm(b2, d2, listA.mid(0, k)) && fill_dorm(b1, d1, listB.mid(0, k));
	if (!success)
	{
		bool restoredA = restore_dorm(b1, d1, bedsA, genderA);
		bool restoredB = restore_dorm(b2, d2, bedsB, genderB);
		bool restored = restoredA && restoredB;
		QVector<int> original_ids = listA;
		original_ids += listB;
		reset_and_sync_students(original_ids, b1, d1, b2, d2);
		return restored ? -5 : -6;
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return 1;
}
