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
	clear_all_dorms_reset_gender();
	//再以学生本体表为准统一清零，修复可能存在的“位置字段有值但 beds 无记录”异常状态。
	for (int student_id : studentmanager::instance().all_ids())
		studentmanager::instance().clear_dorm_info(student_id);
	return assign_all_students_random();
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
		for (int student_id : ids)
			studentmanager::instance().clear_dorm_info(student_id);
	}
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
		for (int student_id : ids)
			studentmanager::instance().clear_dorm_info(student_id);
	}
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

void school::restore_dorm(int building_id, int dorm_id, const QVector<int>& beds, int gender)//按床位恢复宿舍原状态
{
	dormmanager::instance().clear_dorm_students(building_id, dorm_id, true);
	for (int i = 0; i < beds.size(); ++i)
	{
		if (beds[i] == 0)
			continue;
		const student* s = studentmanager::instance().get(beds[i]);
		if (s != nullptr)
			dormmanager::instance().add_student_to_dorm(building_id, dorm_id, beds[i], s->get_gender(), i + 1);
	}
	dormmanager::instance().set_dorm_gender(building_id, dorm_id, gender);
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
		restore_dorm(b1, d1, bedsA, genderA);
		restore_dorm(b2, d2, bedsB, genderB);
		QVector<int> original_ids = listA;
		original_ids += listB;
		reset_and_sync_students(original_ids, b1, d1, b2, d2);
		return -5;
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
		restore_dorm(b1, d1, bedsA, genderA);
		restore_dorm(b2, d2, bedsB, genderB);
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return success ? 1 : -5;
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
	dormmanager::instance().clear_dorm_students(b1, d1, false);
	dormmanager::instance().clear_dorm_students(b2, d2, false);
	QVector<int> moveA = listA.mid(0, k);
	QVector<int> moveB = listB.mid(0, k);
	bool success = fill_dorm(b2, d2, moveA) && fill_dorm(b1, d1, moveB);
	if (!success)
	{
		restore_dorm(b1, d1, bedsA, genderA);
		restore_dorm(b2, d2, bedsB, genderB);
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return success ? 1 : -5;
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
	for (int i = 0; i < k; ++i)
		if (get_student(listA[i]) == nullptr || get_student(listB[i]) == nullptr)
			return -5;
	dormmanager::instance().clear_dorm_students(b1, d1, true);
	dormmanager::instance().clear_dorm_students(b2, d2, true);
	bool success = fill_dorm(b2, d2, listA.mid(0, k)) && fill_dorm(b1, d1, listB.mid(0, k));
	if (!success)
	{
		restore_dorm(b1, d1, bedsA, genderA);
		restore_dorm(b2, d2, bedsB, genderB);
	}
	QVector<int> original_ids = listA;
	original_ids += listB;
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	return success ? 1 : -5;
}
