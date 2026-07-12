#include "school.h"
#include "studentmanager.h"
#include "dormmanager.h"
#include "buildingmanager.h"
#include "system/check.h"
#include <QRandomGenerator>
#include <QHash>
#include <QSet>
#include <QStringList>
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

int school::suggest_dorm_id(int building_id, int floor) const//建议指定楼层最小缺号宿舍
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_floor(floor, 99))
		return -1;
	const building* current_building = buildingmanager::instance().get(building_id);
	if (current_building == nullptr)
		return 0;
	if (floor > current_building->get_max_floor() + 1)
		return -1;
	QSet<int> used_room_numbers;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		if (key.first != building_id)
			continue;
		const dorm* existing_dorm = dormmanager::instance().get(key.first, key.second);
		if (!check::is_valid_dorm_id(key.second) || existing_dorm == nullptr || existing_dorm->get_id() != key.second)
			return -1;
		if (check::dorm_id_floor(key.second) != floor)
			continue;
		const int room_num = check::dorm_id_room_num(key.second);
		if (!check::is_valid_dorm_room_num(room_num))
			return -1;
		used_room_numbers.insert(room_num);
	}
	for (int room_num = 1; room_num <= 99; ++room_num)
	{
		if (!used_room_numbers.contains(room_num))
			return check::make_dorm_id(floor, room_num);
	}
	return -9;
}

int school::suggest_student_id(int grade, int class_num) const//建议指定年级班级的最小缺号学号
{
	if (!check::is_valid_grade(grade) || !check::is_valid_class_num(class_num))
		return -1;
	QSet<int> existing_sequences;
	for (int student_id : studentmanager::instance().all_ids())
	{
		const student* existing = studentmanager::instance().get(student_id);
		if (existing != nullptr && existing->get_grade() == grade)
		{
			if (student_id != existing->get_id()
				|| !check::is_student_id_consistent(existing->get_id(), existing->get_grade(), existing->get_class_num()))
				return -1;
			const int sequence = check::student_id_sequence(existing->get_id());
			if (existing_sequences.contains(sequence))
				return -1;
			existing_sequences.insert(sequence);
		}
	}
	const int sequence = studentmanager::instance().next_available_sequence(grade);
	if (sequence < 0)
		return -1;
	if (sequence == 0)
		return -9;
	return check::make_student_id(grade, class_num, sequence);
}

int school::add_student(const QString& name, int gender, int grade, int class_num, int sequence)//按统一学号规则添加学生
{
	if (!check::is_valid_student_name(name) || !check::is_valid_gender(gender)
		|| !check::is_valid_grade(grade) || !check::is_valid_class_num(class_num) || sequence < 0)
		return -1;
	if (sequence == 0)
	{
		const int suggested_id = suggest_student_id(grade, class_num);
		if (suggested_id < 0)
			return suggested_id;
		sequence = check::student_id_sequence(suggested_id);
	}
	if (!check::is_valid_student_sequence(sequence))
		return -1;
	if (studentmanager::instance().is_sequence_used(grade, sequence))
		return -2;
	const int student_id = check::make_student_id(grade, class_num, sequence);
	if (studentmanager::instance().is_exists(student_id))
		return -3;
	student new_student;
	const bool initialized = new_student.set_id(student_id) && new_student.set_name(name)
		&& new_student.set_gender(gender) && new_student.set_grade(grade) && new_student.set_class_num(class_num);
	return initialized && studentmanager::instance().add(new_student) ? student_id : -1;
}

int school::set_student_name(int student_id, const QString& name)//修改学生姓名
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	return studentmanager::instance().set_student_name(student_id, name);
}

int school::change_student_academic_info(int old_student_id, int new_grade, int new_class_num)//原子迁移学生学号与学籍信息
{
	if (!check::is_valid_student_id(old_student_id) || !check::is_valid_grade(new_grade)
		|| !check::is_valid_class_num(new_class_num))
		return -1;
	const student* current = studentmanager::instance().get(old_student_id);
	if (current == nullptr)
		return 0;
	const student snapshot = *current;
	if (old_student_id != snapshot.get_id()
		|| !check::is_student_id_consistent(snapshot.get_id(), snapshot.get_grade(), snapshot.get_class_num()))
		return -1;
	const int old_sequence = check::student_id_sequence(old_student_id);
	if (studentmanager::instance().is_sequence_used(snapshot.get_grade(), old_sequence, old_student_id))
		return -1;
	const bool unassigned = snapshot.get_bed_id() == 0 && snapshot.get_dorm_id() == 0
		&& snapshot.get_building_id() == 0 && snapshot.get_floor() == 0;
	const bool assigned = snapshot.get_bed_id() > 0 && check::is_valid_dorm_id(snapshot.get_dorm_id())
		&& check::is_valid_building_id(snapshot.get_building_id()) && snapshot.get_floor() == snapshot.get_dorm_id() / 100;
	if (!unassigned && !assigned)
		return -8;
	int bed_reference_count = 0;
	bool declared_bed_found = false;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const dorm* inspected_dorm = dormmanager::instance().get(key.first, key.second);
		if (inspected_dorm == nullptr)
			continue;
		for (int bed_id = 1; bed_id <= inspected_dorm->get_max_num(); ++bed_id)
		{
			if (inspected_dorm->get_student_id(bed_id) != old_student_id)
				continue;
			++bed_reference_count;
			declared_bed_found = assigned && key.first == snapshot.get_building_id()
				&& key.second == snapshot.get_dorm_id() && bed_id == snapshot.get_bed_id();
		}
	}
	if ((unassigned && bed_reference_count != 0)
		|| (assigned && (bed_reference_count != 1 || !declared_bed_found)))
		return -8;
	if (assigned)
	{
		const dorm* current_dorm = dormmanager::instance().get(snapshot.get_building_id(), snapshot.get_dorm_id());
		if (current_dorm == nullptr || current_dorm->get_student_id(snapshot.get_bed_id()) != old_student_id)
			return -8;
	}
	int sequence = old_sequence;
	if (new_grade != snapshot.get_grade())
	{
		const int suggested_id = suggest_student_id(new_grade, new_class_num);
		if (suggested_id < 0)
			return suggested_id;
		sequence = check::student_id_sequence(suggested_id);
	}
	const int new_student_id = check::make_student_id(new_grade, new_class_num, sequence);
	if (new_student_id == old_student_id)
		return old_student_id;
	if (studentmanager::instance().is_sequence_used(new_grade, sequence, old_student_id)
		|| studentmanager::instance().is_exists(new_student_id))
		return -2;
	if (studentmanager::instance().rekey_student(old_student_id, new_student_id, new_grade, new_class_num) != 1)
		return -5;
	if (assigned && dormmanager::instance().replace_student_id_at_bed(snapshot.get_building_id(), snapshot.get_dorm_id(),
		snapshot.get_bed_id(), old_student_id, new_student_id) != 1)
	{
		return studentmanager::instance().rekey_student(new_student_id, old_student_id,
			snapshot.get_grade(), snapshot.get_class_num()) == 1 ? -5 : -6;
	}
	const student* updated = studentmanager::instance().get(new_student_id);
	const dorm* updated_dorm = assigned ? dormmanager::instance().get(snapshot.get_building_id(), snapshot.get_dorm_id()) : nullptr;
	const bool consistent = updated != nullptr
		&& check::is_student_id_consistent(updated->get_id(), updated->get_grade(), updated->get_class_num())
		&& (!assigned || (updated_dorm != nullptr && updated_dorm->get_student_id(snapshot.get_bed_id()) == new_student_id));
	if (consistent)
		return new_student_id;
	bool bed_restored = true;
	if (assigned)
		bed_restored = dormmanager::instance().replace_student_id_at_bed(snapshot.get_building_id(), snapshot.get_dorm_id(),
			snapshot.get_bed_id(), new_student_id, old_student_id) == 1;
	const bool student_restored = studentmanager::instance().rekey_student(new_student_id, old_student_id,
		snapshot.get_grade(), snapshot.get_class_num()) == 1;
	return bed_restored && student_restored ? -5 : -6;
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

int school::add_dorm(int building_id, int floor, int room_num, int max_num, int gender_lock)//扩层与新增宿舍统一事务
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_floor(floor, 99)
		|| !check::is_valid_dorm_room_num(room_num) || max_num < 1 || max_num > 99
		|| !check::is_valid_gender(gender_lock))
		return -1;
	const building* current_building = buildingmanager::instance().get(building_id);
	if (current_building == nullptr)
		return 0;
	const int original_max_floor = current_building->get_max_floor();
	if (floor > original_max_floor + 1)
		return -1;
	if (gender_lock != 0 && !current_building->accepts_gender(gender_lock))
		return -3;
	const int suggestion_result = suggest_dorm_id(building_id, floor);
	if (suggestion_result == -9)
		return -9;
	if (suggestion_result <= 0)
		return suggestion_result == 0 ? 0 : -1;
	const int dorm_id = check::make_dorm_id(floor, room_num);
	if (dormmanager::instance().get(building_id, dorm_id) != nullptr)
		return -2;
	const bool expands_building = floor == original_max_floor + 1;
	if (expands_building && set_building_max_floor(building_id, floor) != 1)
		return -5;

	dorm new_dorm;
	const bool initialized = new_dorm.set_building_id(building_id)
		&& new_dorm.set_id(dorm_id) && new_dorm.set_max_num(max_num);
	bool dorm_added = initialized && add_dorm(new_dorm);
	bool gender_set = dorm_added && (gender_lock == 0 || set_dorm_gender(building_id, dorm_id, gender_lock) == 1);
	if (gender_set)
	{
		const building* updated_building = buildingmanager::instance().get(building_id);
		const dorm* added_dorm = dormmanager::instance().get(building_id, dorm_id);
		if (updated_building != nullptr && updated_building->get_max_floor() == (expands_building ? floor : original_max_floor)
			&& added_dorm != nullptr && added_dorm->get_building_id() == building_id
			&& added_dorm->get_id() == dorm_id && added_dorm->get_max_num() == max_num
			&& added_dorm->get_for_gender() == gender_lock)
			return dorm_id;
	}

	bool dorm_removed = true;
	if (dorm_added)
		dorm_removed = remove_dorm(building_id, dorm_id);
	bool floor_restored = true;
	if (expands_building)
		floor_restored = set_building_max_floor(building_id, original_max_floor) == 1;
	const building* restored_building = buildingmanager::instance().get(building_id);
	const bool restored = dorm_removed && floor_restored && restored_building != nullptr
		&& restored_building->get_max_floor() == original_max_floor
		&& dormmanager::instance().get(building_id, dorm_id) == nullptr;
	return restored ? -5 : -6;
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
	if (s == nullptr)
		return -6;
	const dorm* target = dm.get(building_id, dorm_id);
	if (target == nullptr)
		return -8;
	if (specified_bed && !check::is_valid_bed_id(bed_id, target->get_max_num()))
		return -1;
	bool all_empty = s->get_bed_id() == 0 && s->get_dorm_id() == 0 && s->get_building_id() == 0 && s->get_floor() == 0;
	bool all_assigned = s->get_bed_id() > 0 && s->get_dorm_id() > 0 && s->get_building_id() > 0 && s->get_floor() > 0;
	if (!all_empty && !all_assigned)
		return -8;
	if (all_empty)
		return 0;
	if (s->get_gender() == 0)
		return -6;

	int old_building_id = s->get_building_id();
	int old_dorm_id = s->get_dorm_id();
	int old_bed_id = s->get_bed_id();
	const dorm* source = dm.get(old_building_id, old_dorm_id);
	if (source == nullptr || !is_dorm_consistent(old_building_id, old_dorm_id) ||
		((old_building_id != building_id || old_dorm_id != dorm_id) && !is_dorm_consistent(building_id, dorm_id)))
		return -8;

	bool same_dorm = old_building_id == building_id && old_dorm_id == dorm_id;
	if (same_dorm)
	{
		if (!specified_bed || bed_id == old_bed_id)
			return old_bed_id;
		if (target->is_bed_occupied(bed_id) == 1)
			return -2;
		if (dm.move_student_bed(building_id, dorm_id, old_bed_id, bed_id) != 1)
			return -8;
		if (sm.assign_dorm_info(student_id, bed_id, dorm_id, building_id, target->get_floor()) == 1)
			return bed_id;
		bool restored_bed = dm.move_student_bed(building_id, dorm_id, bed_id, old_bed_id) == 1;
		bool restored_info = sm.assign_dorm_info(student_id, old_bed_id, old_dorm_id, old_building_id, source->get_floor()) == 1;
		return restored_bed && restored_info && is_dorm_consistent(old_building_id, old_dorm_id) ? -8 : -10;
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

int school::swap_students(int student_id1, int student_id2)//交换两名学生的床位
{
	if (!check::is_valid_student_id(student_id1) || !check::is_valid_student_id(student_id2) || student_id1 == student_id2)
		return -1;
	studentmanager& sm = studentmanager::instance();
	dormmanager& dm = dormmanager::instance();
	const student* s1 = sm.get(student_id1);
	const student* s2 = sm.get(student_id2);
	if (s1 == nullptr || s2 == nullptr || s1->get_gender() == 0 || s2->get_gender() == 0)
		return -2;
	auto has_complete_position = [](const student* s)
	{
		return s->get_bed_id() > 0 && s->get_dorm_id() > 0 && s->get_building_id() > 0 && s->get_floor() > 0;
	};
	if (!has_complete_position(s1) || !has_complete_position(s2))
		return -3;

	int b1 = s1->get_building_id();
	int d1 = s1->get_dorm_id();
	int bed1 = s1->get_bed_id();
	int b2 = s2->get_building_id();
	int d2 = s2->get_dorm_id();
	int bed2 = s2->get_bed_id();
	const dorm* dorm1 = dm.get(b1, d1);
	const dorm* dorm2 = dm.get(b2, d2);
	if (dorm1 == nullptr || dorm2 == nullptr || !is_dorm_consistent(b1, d1) ||
		((b1 != b2 || d1 != d2) && !is_dorm_consistent(b2, d2)))
		return -4;

	bool same_dorm = b1 == b2 && d1 == d2;
	if (same_dorm)
	{
		if (dm.move_student_bed(b1, d1, bed1, bed2) != 1)
			return -5;
		bool synced1 = sm.assign_dorm_info(student_id1, bed2, d1, b1, dorm1->get_floor()) == 1;
		bool synced2 = sm.assign_dorm_info(student_id2, bed1, d1, b1, dorm1->get_floor()) == 1;
		if (synced1 && synced2 && is_dorm_consistent(b1, d1))
			return 1;
		bool restored_beds = dm.move_student_bed(b1, d1, bed2, bed1) == 1;
		bool restored1 = sm.assign_dorm_info(student_id1, bed1, d1, b1, dorm1->get_floor()) == 1;
		bool restored2 = sm.assign_dorm_info(student_id2, bed2, d1, b1, dorm1->get_floor()) == 1;
		return restored_beds && restored1 && restored2 && is_dorm_consistent(b1, d1) ? -5 : -6;
	}

	const building* building1 = buildingmanager::instance().get(b1);
	const building* building2 = buildingmanager::instance().get(b2);
	if (building1 == nullptr || building2 == nullptr ||
		!building2->accepts_gender(s1->get_gender()) || !dorm2->accepts_gender(s1->get_gender()) ||
		!building1->accepts_gender(s2->get_gender()) || !dorm1->accepts_gender(s2->get_gender()))
		return -5;

	QVector<int> beds1 = snapshot_dorm(*dorm1);
	QVector<int> beds2 = snapshot_dorm(*dorm2);
	int gender1 = dorm1->get_for_gender();
	int gender2 = dorm2->get_for_gender();
	QVector<int> original_ids = dorm1->get_student_id_list();
	original_ids += dorm2->get_student_id_list();
	bool moved = dm.remove_student_from_dorm(b1, d1, student_id1) == bed1 &&
		dm.remove_student_from_dorm(b2, d2, student_id2) == bed2 &&
		dm.add_student_to_dorm(b2, d2, student_id1, s1->get_gender(), bed2) == bed2 &&
		dm.add_student_to_dorm(b1, d1, student_id2, s2->get_gender(), bed1) == bed1;
	if (moved)
	{
		reset_and_sync_students(original_ids, b1, d1, b2, d2);
		if (is_dorm_consistent(b1, d1) && is_dorm_consistent(b2, d2))
			return 1;
	}

	bool restored1 = restore_dorm(b1, d1, beds1, gender1);
	bool restored2 = restore_dorm(b2, d2, beds2, gender2);
	reset_and_sync_students(original_ids, b1, d1, b2, d2);
	bool restored = restored1 && restored2 && is_dorm_consistent(b1, d1) && is_dorm_consistent(b2, d2);
	return restored ? -5 : -6;
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

QVector<accommodation_data_issue> school::collect_accommodation_issues() const//逐床核对全校住宿数据
{
	QVector<accommodation_data_issue> issues;
	QSet<int> students_in_beds;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
	{
		const dorm* d = dormmanager::instance().get(key.first, key.second);
		const building* b = buildingmanager::instance().get(key.first);
		if (d == nullptr || b == nullptr)
		{
			issues.append({0, key.first, key.second, QStringLiteral("宿舍或所属宿舍楼不存在。")});
			continue;
		}
		if (d->get_for_gender() != 0 && !b->accepts_gender(d->get_for_gender()))
			issues.append({0, key.first, key.second, QStringLiteral("宿舍性别锁与所属宿舍楼的性别要求冲突。")});
		for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
		{
			const int student_id = d->get_student_id(bed_id);
			if (student_id <= 0)
				continue;
			if (students_in_beds.contains(student_id))
			{
				issues.append({student_id, key.first, key.second, QStringLiteral("同一名学生出现在多个床位中。")});
				continue;
			}
			students_in_beds.insert(student_id);
			const student* s = studentmanager::instance().get(student_id);
			if (s == nullptr)
			{
				issues.append({student_id, key.first, key.second, QStringLiteral("床位中的学生档案不存在。")});
				continue;
			}
			if (s->get_building_id() != key.first || s->get_dorm_id() != key.second
				|| s->get_bed_id() != bed_id || s->get_floor() != d->get_floor())
				issues.append({student_id, key.first, key.second, QStringLiteral("学生登记的住宿位置与实际床位不一致。")});
			if (!b->accepts_gender(s->get_gender()) || !d->accepts_gender(s->get_gender()))
				issues.append({student_id, key.first, key.second, QStringLiteral("学生性别不符合宿舍楼或宿舍性别锁要求。")});
		}
	}

	for (int student_id : studentmanager::instance().all_ids())
	{
		const student* s = studentmanager::instance().get(student_id);
		if (s == nullptr)
			continue;
		const bool all_empty = s->get_building_id() == 0 && s->get_dorm_id() == 0
			&& s->get_bed_id() == 0 && s->get_floor() == 0;
		const bool all_assigned = s->get_building_id() > 0 && s->get_dorm_id() > 0
			&& s->get_bed_id() > 0 && s->get_floor() > 0;
		if (!all_empty && !all_assigned)
		{
			issues.append({student_id, s->get_building_id(), s->get_dorm_id(), QStringLiteral("学生住宿位置字段不完整。")});
			continue;
		}
		if (all_empty && students_in_beds.contains(student_id))
			issues.append({student_id, 0, 0, QStringLiteral("学生登记为未入住，但实际床位仍有记录。")});
		else if (all_assigned && !students_in_beds.contains(student_id))
			issues.append({student_id, s->get_building_id(), s->get_dorm_id(), QStringLiteral("学生登记为已入住，但对应床位没有该学生。")});
	}
	return issues;
}

batch_assignment_preview school::preview_assign_unassigned_students(assignment_strategy strategy, quint32 random_seed) const//生成未入住学生补分预览基础信息
{
	batch_assignment_preview preview;
	preview.strategy = strategy;
	preview.random_seed = random_seed;
	preview.issues = collect_accommodation_issues();
	if (!preview.issues.isEmpty())
		return preview;
	preview.unassigned_student_ids = get_unassigned_student_ids();
	preview.candidate_count = preview.unassigned_student_ids.size();
	preview.available_bed_count = 0;

	struct planned_dorm
	{
		int building_id = 0;
		int dorm_id = 0;
		int gender = 0;
		QVector<int> beds;
	};
	QVector<planned_dorm> dorms;
	for (const auto& key : get_all_dorm_keys())
	{
		const dorm* d = get_dorm(key.first, key.second);
		if (d == nullptr || d->is_full())
			continue;
		dorms.append({key.first, key.second, d->get_for_gender(), snapshot_dorm(*d)});
	}

	QVector<int> candidates = preview.unassigned_student_ids;
	QRandomGenerator random(random_seed);
	if (strategy == assignment_strategy::random)
	{
		for (int i = candidates.size() - 1; i > 0; --i)
			candidates.swapItemsAt(i, random.bounded(i + 1));
	}
	preview.unassigned_student_ids.clear();
	for (int student_id : candidates)
	{
		const student* s = get_student(student_id);
		if (s == nullptr || (s->get_gender() != 1 && s->get_gender() != 2))
		{
			preview.unassigned_student_ids.append(student_id);
			continue;
		}
		QVector<int> compatible;
		for (int i = 0; i < dorms.size(); ++i)
		{
			const building* b = get_building(dorms[i].building_id);
			if (b == nullptr || !b->accepts_gender(s->get_gender())
				|| (dorms[i].gender != 0 && dorms[i].gender != s->get_gender()))
				continue;
			bool has_empty_bed = false;
			for (int occupant_id : dorms[i].beds)
				has_empty_bed = has_empty_bed || occupant_id == 0;
			if (has_empty_bed)
				compatible.append(i);
		}
		if (compatible.isEmpty())
		{
			preview.unassigned_student_ids.append(student_id);
			continue;
		}

		int selected = compatible.first();
		if (strategy == assignment_strategy::random)
			selected = compatible.at(random.bounded(compatible.size()));
		else
		{
			for (int index : compatible)
			{
				const planned_dorm& current = dorms[index];
				const planned_dorm& best = dorms[selected];
				const int current_occupied = current.beds.size() - std::count(current.beds.cbegin(), current.beds.cend(), 0);
				const int best_occupied = best.beds.size() - std::count(best.beds.cbegin(), best.beds.cend(), 0);
				const bool current_started = current_occupied > 0;
				const bool best_started = best_occupied > 0;
				const bool better = current_started != best_started ? current_started
					: current_occupied * best.beds.size() != best_occupied * current.beds.size()
						? current_occupied * best.beds.size() > best_occupied * current.beds.size()
						: current.building_id != best.building_id ? current.building_id < best.building_id
						: current.dorm_id < best.dorm_id;
				if (better)
					selected = index;
			}
		}

		planned_dorm& target = dorms[selected];
		QVector<int> empty_beds;
		for (int i = 0; i < target.beds.size(); ++i)
			if (target.beds[i] == 0)
				empty_beds.append(i + 1);
		const int bed_id = strategy == assignment_strategy::random
			? empty_beds.at(random.bounded(empty_beds.size())) : empty_beds.first();
		target.beds[bed_id - 1] = student_id;
		if (target.gender == 0)
			target.gender = s->get_gender();
		preview.changes.append({student_id, s->get_gender(), 0, 0, 0, target.building_id, target.dorm_id, bed_id});
	}
	preview.available_bed_count = preview.changes.size();
	return preview;
}

int school::apply_batch_assignment(const batch_assignment_preview& preview)//按固定预览执行补分
{
	if (preview.changes.isEmpty())
		return 0;
	if (!collect_accommodation_issues().isEmpty())
		return -8;
	QSet<int> student_ids;
	QSet<QString> target_beds;
	QHash<QString, int> simulated_genders;
	QHash<QString, int> original_genders;
	for (const accommodation_change& change : preview.changes)
	{
		if (change.student_id <= 0 || (change.student_gender != 1 && change.student_gender != 2)
			|| change.old_building_id != 0 || change.old_dorm_id != 0 || change.old_bed_id != 0
			|| change.new_building_id <= 0 || change.new_dorm_id <= 0 || change.new_bed_id <= 0
			|| student_ids.contains(change.student_id))
			return -7;
		const QString bed_key = QStringLiteral("%1/%2/%3").arg(change.new_building_id).arg(change.new_dorm_id).arg(change.new_bed_id);
		if (target_beds.contains(bed_key))
			return -7;
		student_ids.insert(change.student_id);
		target_beds.insert(bed_key);
		const student* s = get_student(change.student_id);
		const dorm* d = get_dorm(change.new_building_id, change.new_dorm_id);
		const building* b = get_building(change.new_building_id);
		const QString dorm_key = QStringLiteral("%1/%2").arg(change.new_building_id).arg(change.new_dorm_id);
		if (s == nullptr || d == nullptr || b == nullptr
			|| s->get_gender() != change.student_gender
			|| s->get_building_id() != 0 || s->get_dorm_id() != 0 || s->get_bed_id() != 0 || s->get_floor() != 0
			|| d->is_bed_occupied(change.new_bed_id) != 0
			|| !b->accepts_gender(s->get_gender()))
			return -7;
		if (!simulated_genders.contains(dorm_key))
		{
			simulated_genders.insert(dorm_key, d->get_for_gender());
			original_genders.insert(dorm_key, d->get_for_gender());
		}
		if (simulated_genders.value(dorm_key) != 0 && simulated_genders.value(dorm_key) != s->get_gender())
			return -7;
		if (simulated_genders.value(dorm_key) == 0)
			simulated_genders[dorm_key] = s->get_gender();
	}

	QVector<int> assigned_ids;
	for (const accommodation_change& change : preview.changes)
	{
		const int result = assign_student_to_dorm(change.new_building_id, change.new_dorm_id, change.student_id, change.new_bed_id);
		if (result == change.new_bed_id)
		{
			assigned_ids.append(change.student_id);
			continue;
		}
		bool restored = true;
		for (int i = assigned_ids.size() - 1; i >= 0; --i)
			restored = remove_student_from_dorm(assigned_ids[i]) > 0 && restored;
		for (auto it = original_genders.cbegin(); it != original_genders.cend(); ++it)
		{
			const QStringList parts = it.key().split(QLatin1Char('/'));
			if (parts.size() != 2)
			{
				restored = false;
				continue;
			}
			restored = dormmanager::instance().set_dorm_gender(parts[0].toInt(), parts[1].toInt(), it.value()) == 1 && restored;
		}
		return restored ? -5 : -6;
	}
	return assigned_ids.size();
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
