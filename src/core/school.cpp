#include "school.h"
#include "studentmanager.h"
#include "dormmanager.h"
#include "buildingmanager.h"
#include "system/check.h"
#include <QRandomGenerator>

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

const dorm* school::get_available_dorm(int gender) const//获取指定性别最小顺位可用宿舍
{
	if (gender != 1 && gender != 2)
		return nullptr;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const building* b = buildingmanager::instance().get(key.first);
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (b != nullptr && d != nullptr && b->accepts_gender(gender) && d->accepts_gender(gender) && !d->is_full())
			return d;
	}
	return nullptr;
}

const dorm* school::get_available_dorm_random(int gender) const//随机获取指定性别可用宿舍
{
	if (gender != 1 && gender != 2)
		return nullptr;
	QVector<QPair<int, int>> candidates;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const building* b = buildingmanager::instance().get(key.first);
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		if (b != nullptr && d != nullptr && b->accepts_gender(gender) && d->accepts_gender(gender) && !d->is_full())
			candidates.append(key);
	}
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

bool school::add_dorm(const dorm& dorm_to_add)//添加宿舍并校验楼级约束
{
	const building* b = buildingmanager::instance().get(dorm_to_add.get_building_id());
	if (b == nullptr || !check::is_valid_dorm_floor(dorm_to_add.get_id(), b->get_max_floor()))
		return false;
	return dormmanager::instance().add_dorm(dorm_to_add);
}

int school::set_dorm_gender(int building_id, int dorm_id, int gender)//设置房间性别锁
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_gender(gender))
		return -1;
	const dorm* d = dormmanager::instance().get(building_id, dorm_id);
	if (d == nullptr)
		return 0;
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
	dormmanager::instance().clear_all_dorms_reset_gender();
	//再以学生本体表为准统一清零，修复可能存在的“位置字段有值但 beds 无记录”异常状态。
	for (int student_id : studentmanager::instance().all_ids())
		studentmanager::instance().clear_dorm_info(student_id);
	return assign_all_students_random();
}
