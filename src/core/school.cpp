#include "school.h"
#include "studentmanager.h"
#include "dormmanager.h"
#include "buildingmanager.h"
#include "persistence/schoolstorage.h"
#include "system/check.h"
#include <QRandomGenerator>
#include <QHash>
#include <QDir>
#include <QFileInfo>
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
	if (!begin_persistent_mutation()) return false;
	return finish_persistent_mutation(studentmanager::instance().add(student_to_add));
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
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
}

int school::set_student_name(int student_id, const QString& name)//修改学生姓名
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	return studentmanager::instance().set_student_name(student_id, name);
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
}

int school::change_student_academic_info(int old_student_id, int new_grade, int new_class_num)//原子迁移学生学号与学籍信息
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
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
	if (!begin_persistent_mutation()) return -102;
	const int result = buildingmanager::instance().add_building(building_id, gender, max_floor);
	return finish_persistent_mutation(result, result == 1);
}

bool school::add_dorm(const dorm& dorm_to_add)//添加宿舍并校验楼级约束
{
	if (!begin_persistent_mutation()) return false;
	const auto operation = [&]() -> bool {
	if (!dorm_to_add.is_empty())
		return false;//禁止携带预填住客入库，所有入住必须经过school闭环
	const building* b = buildingmanager::instance().get(dorm_to_add.get_building_id());
	if (b == nullptr || !check::is_valid_dorm_floor(dorm_to_add.get_id(), b->get_max_floor()))
		return false;
	if (dorm_to_add.get_for_gender() != 0 && !b->accepts_gender(dorm_to_add.get_for_gender()))
		return false;//空宿舍预设性别锁必须被所在楼接纳
	return dormmanager::instance().add_dorm(dorm_to_add);
	};
	return finish_persistent_mutation(operation());
}

int school::add_dorm(int building_id, int floor, int room_num, int max_num, int gender_lock)//扩层与新增宿舍统一事务
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
}

int school::set_dorm_max_num(int building_id, int dorm_id, int max_num)//修改宿舍最大床位数
{
	if (!begin_persistent_mutation()) return -102;
	const int result = dormmanager::instance().set_dorm_max_num(building_id, dorm_id, max_num);
	return finish_persistent_mutation(result, result == 1);
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
	if (!begin_persistent_mutation()) return false;
	const auto operation = [&]() -> bool {
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
	};
	return finish_persistent_mutation(operation());
}

bool school::remove_building(int building_id)//删除宿舍楼并级联处理楼内宿舍
{
	if (!begin_persistent_mutation()) return false;
	const auto operation = [&]() -> bool {
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
	};
	return finish_persistent_mutation(operation());
}

int school::set_building_gender(int building_id, int gender)//修改楼适用性别
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
}

int school::set_building_max_floor(int building_id, int max_floor)//修改楼最大楼层
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
	if (!check::is_valid_building_id(building_id) || !check::is_valid_max_floor(max_floor))
		return -1;
	if (buildingmanager::instance().get(building_id) == nullptr)
		return 0;
	for (const auto& key : dormmanager::instance().all_dorm_keys())
		if (key.first == building_id && !check::is_valid_dorm_floor(key.second, max_floor))
			return -2;//既有宿舍派生楼层超出新上限
	return buildingmanager::instance().set_building_max_floor(building_id, max_floor);
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
}

int school::set_dorm_gender(int building_id, int dorm_id, int gender)//设置房间性别锁
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
}

int school::assign_student_to_dorm(int building_id, int dorm_id, int student_id)//入住指定宿舍并自动分配最小空床位
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
}

int school::assign_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id)//入住指定宿舍的指定床位
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int final_result = operation();
	return finish_persistent_mutation(final_result, final_result > 0);
}

int school::assign_student_to_available_dorm(int student_id)//入住最小顺位可用宿舍
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
}

int school::assign_student_to_available_dorm_random(int student_id)//随机入住可用宿舍
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
}

int school::remove_student_from_dorm(int student_id)//退宿但保留学籍
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result > 0);
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
	if (!begin_persistent_mutation()) return -102;
	const int result = move_student_to_dorm_impl(building_id, dorm_id, student_id, 0, false);
	return finish_persistent_mutation(result, result > 0);
}

int school::move_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id)//调往指定宿舍的指定床位
{
	if (!begin_persistent_mutation()) return -102;
	const int result = move_student_to_dorm_impl(building_id, dorm_id, student_id, bed_id, true);
	return finish_persistent_mutation(result, result > 0);
}

int school::swap_students(int student_id1, int student_id2)//交换两名学生的床位
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
}

int school::remove_student(int student_id)//退学籍
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
}

int school::correct_student_gender(int student_id, int gender)//性别纠错
{
	if (!begin_persistent_mutation()) return -102;
	const auto operation = [&]() -> int {
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
	};
	const int result = operation();
	return finish_persistent_mutation(result, result == 1);
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
		if (d->get_current_num() > 0 && d->get_for_gender() == 0)
			issues.append({0, key.first, key.second, QStringLiteral("宿舍已有住客，但宿舍性别锁仍为未设置。")});
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
			if (s->get_gender() != 1 && s->get_gender() != 2)
				issues.append({student_id, key.first, key.second, QStringLiteral("已入住学生的性别尚未明确设置。")});
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

quint64 school::accommodation_resource_signature() const//生成住宿资源状态签名
{
	quint64 value = 1469598103934665603ULL;
	const auto mix = [&value](quint64 part) { value ^= part + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2); };
	for (int building_id : get_all_building_ids())
	{
		const building* b = get_building(building_id);
		if (b != nullptr) { mix(building_id); mix(b->get_for_gender()); mix(b->get_max_floor()); }
	}
	for (const auto& key : get_all_dorm_keys())
	{
		const dorm* d = get_dorm(key.first, key.second);
		if (d == nullptr) continue;
		mix(key.first); mix(key.second); mix(d->get_max_num()); mix(d->get_for_gender());
		for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id) mix(static_cast<quint64>(qMax(0, d->get_student_id(bed_id))));
	}
	for (int student_id : get_all_student_ids())
	{
		const student* s = get_student(student_id);
		if (s == nullptr) continue;
		mix(student_id); mix(s->get_gender()); mix(s->get_building_id()); mix(s->get_dorm_id()); mix(s->get_bed_id()); mix(s->get_floor());
	}
	return value;
}

school::accommodation_snapshot school::take_accommodation_snapshot() const//保存全校住宿快照
{
	accommodation_snapshot snapshot;
	for (const auto& key : get_all_dorm_keys())
	{
		const dorm* d = get_dorm(key.first, key.second);
		if (d != nullptr)
			snapshot.dorms.append({key.first, key.second, d->get_for_gender(), snapshot_dorm(*d)});
	}
	return snapshot;
}

bool school::restore_accommodation_snapshot(const accommodation_snapshot& snapshot)//恢复全校住宿快照
{
	for (const auto& key : get_all_dorm_keys())
		if (dormmanager::instance().clear_dorm_students(key.first, key.second, true) < 0)
			return false;
	for (int student_id : get_all_student_ids())
		if (studentmanager::instance().clear_dorm_info(student_id) != 1)
			return false;
	for (const dorm_accommodation_snapshot& dorm_snapshot : snapshot.dorms)
	{
		if (!restore_dorm(dorm_snapshot.building_id, dorm_snapshot.dorm_id, dorm_snapshot.beds, dorm_snapshot.gender))
			return false;
		const dorm* d = get_dorm(dorm_snapshot.building_id, dorm_snapshot.dorm_id);
		if (d == nullptr)
			return false;
		for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
		{
			const int student_id = d->get_student_id(bed_id);
			if (student_id > 0 && studentmanager::instance().assign_dorm_info(
				student_id, bed_id, d->get_id(), d->get_building_id(), d->get_floor()) != 1)
				return false;
		}
	}
	return collect_accommodation_issues().isEmpty();
}

school::school_data_snapshot school::take_school_data_snapshot() const//保存全校完整数据快照
{
	school_data_snapshot snapshot;
	for (int building_id : get_all_building_ids())
	{
		const building* b = get_building(building_id);
		if (b != nullptr) snapshot.data.buildings.append({b->get_id(), b->get_for_gender(), b->get_max_floor()});
	}
	for (const auto& key : get_all_dorm_keys())
	{
		const dorm* d = get_dorm(key.first, key.second);
		if (d != nullptr) snapshot.data.dorms.append({key.first, key.second, d->get_max_num(), d->get_for_gender()});
	}
	for (int student_id : get_all_student_ids())
	{
		const student* s = get_student(student_id);
		if (s != nullptr) snapshot.data.students.append({s->get_id(), s->get_name(), s->get_gender(), s->get_class_num(), s->get_grade(),
			s->get_building_id(), s->get_dorm_id(), s->get_bed_id()});
	}
	return snapshot;
}

bool school::validate_sample_data_plan(const sampledataplan& plan, bool snapshot_mode) const//校验样例数据计划或原数据快照
{
	QHash<int, samplebuildingplan> buildings;
	for (const samplebuildingplan& item : plan.buildings)
	{
		if (!check::is_valid_building_id(item.id) || !check::is_valid_building_gender(item.gender)
			|| !check::is_valid_max_floor(item.max_floor) || buildings.contains(item.id)) return false;
		buildings.insert(item.id, item);
	}
	QHash<QString, sampledormplan> dorms;
	QHash<QString, QSet<int>> occupied_beds;
	for (const sampledormplan& item : plan.dorms)
	{
		const QString key = QStringLiteral("%1/%2").arg(item.building_id).arg(item.dorm_id);
		if (!buildings.contains(item.building_id) || !check::is_valid_dorm_id(item.dorm_id)
			|| item.dorm_id / 100 > buildings.value(item.building_id).max_floor || item.max_num < 1 || (!snapshot_mode && item.max_num > 99)
			|| item.gender_lock < 0 || item.gender_lock > 2 || dorms.contains(key)
			|| (item.gender_lock != 0 && buildings.value(item.building_id).gender != 3
				&& buildings.value(item.building_id).gender != item.gender_lock)) return false;
		dorms.insert(key, item);
	}
	QSet<int> student_ids;
	QHash<int, QSet<int>> sequences;
	QHash<QString, int> planned_dorm_genders;
	for (const samplestudentplan& item : plan.students)
	{
		if (!check::is_valid_student_id(item.id) || !check::is_valid_student_name(item.name)
			|| (snapshot_mode ? !check::is_valid_gender(item.gender) : (item.gender != 1 && item.gender != 2)) || !check::is_valid_grade(item.grade)
			|| !check::is_valid_class_num(item.class_num) || !check::is_student_id_consistent(item.id, item.grade, item.class_num)
			|| student_ids.contains(item.id)) return false;
		const int sequence = check::student_id_sequence(item.id);
		if (sequences[item.grade].contains(sequence)) return false;
		sequences[item.grade].insert(sequence);
		student_ids.insert(item.id);
		const bool unassigned = item.building_id == 0 && item.dorm_id == 0 && item.bed_id == 0;
		if (unassigned) continue;
		const QString dorm_key = QStringLiteral("%1/%2").arg(item.building_id).arg(item.dorm_id);
		if (!dorms.contains(dorm_key) || item.bed_id < 1 || item.bed_id > dorms.value(dorm_key).max_num
			|| occupied_beds[dorm_key].contains(item.bed_id)
			|| (buildings.value(item.building_id).gender != 3 && buildings.value(item.building_id).gender != item.gender)
			|| (dorms.value(dorm_key).gender_lock != 0 && dorms.value(dorm_key).gender_lock != item.gender)) return false;
		const int planned_gender = planned_dorm_genders.value(dorm_key, dorms.value(dorm_key).gender_lock);
		if (planned_gender != 0 && planned_gender != item.gender) return false;
		planned_dorm_genders[dorm_key] = item.gender;
		occupied_beds[dorm_key].insert(item.bed_id);
	}
	return true;
}

bool school::purge_all_data()//事务内部清除全部对象
{
	bool success = true;
	for (const auto& key : get_all_dorm_keys())
		success = dormmanager::instance().clear_dorm_students(key.first, key.second, true) >= 0 && success;
	for (int student_id : get_all_student_ids())
	{
		studentmanager::instance().clear_dorm_info(student_id);
		success = studentmanager::instance().remove(student_id) && success;
	}
	for (const auto& key : get_all_dorm_keys())
		success = dormmanager::instance().remove_dorm(key.first, key.second) && success;
	for (int building_id : get_all_building_ids())
		success = buildingmanager::instance().remove_building(building_id) && success;
	return success && get_student_count() == 0 && get_dorm_count() == 0 && get_building_count() == 0;
}

bool school::write_sample_data_plan(const sampledataplan& plan)//按固定计划写入全校数据
{
	for (const samplebuildingplan& item : plan.buildings)
		if (add_building(item.id, item.gender, item.max_floor) != 1) return false;
	for (const sampledormplan& item : plan.dorms)
	{
		dorm new_dorm;
		if (!new_dorm.set_building_id(item.building_id) || !new_dorm.set_id(item.dorm_id)
			|| !new_dorm.set_max_num(item.max_num) || !add_dorm(new_dorm)) return false;
		if (item.gender_lock != 0 && set_dorm_gender(item.building_id, item.dorm_id, item.gender_lock) != 1) return false;
	}
	for (const samplestudentplan& item : plan.students)
	{
		student new_student;
		if (!new_student.set_id(item.id) || !new_student.set_name(item.name) || !new_student.set_gender(item.gender)
			|| !new_student.set_grade(item.grade) || !new_student.set_class_num(item.class_num) || !add_student(new_student)) return false;
	}
	for (const samplestudentplan& item : plan.students)
		if (item.building_id > 0 && assign_student_to_dorm(item.building_id, item.dorm_id, item.id, item.bed_id) != item.bed_id) return false;
	return collect_accommodation_issues().isEmpty();
}

bool school::restore_school_data_snapshot(const school_data_snapshot& snapshot)//恢复完整全校数据
{
	return purge_all_data() && write_sample_data_plan(snapshot.data);
}

persistence_start_status school::initialize_persistence()//启动加载或创建持久化文件
{
	if (persistence_initialized)
		return current_persistence_status;
	persistence_initialized = true;
	const QString executable_directory = schoolstorage::executable_data_directory();
	const QString fallback_directory = schoolstorage::fallback_data_directory();
	executable_directory_writable = schoolstorage::directory_is_writable(executable_directory);
	fallback_directory_writable = schoolstorage::directory_is_writable(fallback_directory);
	const auto directory_has_data = [](const QString& directory) {
		const QDir data_directory(directory);
		return QFileInfo::exists(data_directory.filePath(QStringLiteral("school-data.json")))
			|| QFileInfo::exists(data_directory.filePath(QStringLiteral("school-data.backup.json")));
	};
	const auto load_candidate = [](const QString& directory, schoolsnapshot& candidate, QString& error) {
		schoolstorage candidate_storage;
		candidate_storage.set_data_directory(directory);
		storage_load_status status = candidate_storage.load_primary(candidate, &error);
		if (status == storage_load_status::not_found || status == storage_load_status::invalid || status == storage_load_status::io_error)
			status = candidate_storage.load_backup(candidate, &error);
		return status;
	};
	const bool executable_exists = directory_has_data(executable_directory);
	const bool fallback_exists = directory_has_data(fallback_directory);

	if (executable_exists && fallback_exists)
	{
		QString executable_error;
		QString fallback_error;
		const storage_load_status executable_status = load_candidate(executable_directory, executable_candidate, executable_error);
		const storage_load_status fallback_status = load_candidate(fallback_directory, fallback_candidate, fallback_error);
		executable_candidate_valid = executable_status == storage_load_status::loaded;
		fallback_candidate_valid = fallback_status == storage_load_status::loaded;
		if (executable_candidate_valid && fallback_candidate_valid)
		{
			read_only = true;
			persistence_error = QStringLiteral("程序目录和用户目录均存在有效数据，请选择本次使用的数据。 ");
			current_persistence_status = persistence_start_status::data_conflict;
			return current_persistence_status;
		}
		if (executable_candidate_valid)
		{
			load_storage_directory(executable_directory, executable_directory_writable, false, persistence_start_status::ready);
			return current_persistence_status;
		}
		if (fallback_candidate_valid)
		{
			load_storage_directory(fallback_directory, fallback_directory_writable, true, persistence_start_status::fallback_ready);
			return current_persistence_status;
		}
		if (executable_status == storage_load_status::newer_version || fallback_status == storage_load_status::newer_version)
		{
			persistence_error = executable_status == storage_load_status::newer_version ? executable_error : fallback_error;
			read_only = true;
			current_persistence_status = persistence_start_status::read_only_newer_version;
			return current_persistence_status;
		}
		persistence_error = QStringLiteral("程序目录和用户目录中的数据均无法读取。\n程序目录：%1\n用户目录：%2")
			.arg(executable_error, fallback_error);
		read_only = true;
		current_persistence_status = persistence_start_status::read_only_corrupt;
		return current_persistence_status;
	}

	if (executable_exists)
	{
		if (executable_directory_writable || !fallback_directory_writable)
		{
			load_storage_directory(executable_directory, executable_directory_writable, false, persistence_start_status::ready);
			return current_persistence_status;
		}
		schoolsnapshot source_snapshot;
		QString source_error;
		const storage_load_status source_status = load_candidate(executable_directory, source_snapshot, source_error);
		if (source_status == storage_load_status::loaded && apply_loaded_snapshot(source_snapshot))
		{
			storage.set_data_directory(fallback_directory);
			if (storage.rebuild_primary(source_snapshot, &persistence_error))
			{
				read_only = false;
				current_persistence_status = persistence_start_status::fallback_ready;
				return current_persistence_status;
			}
		}
		if (source_status == storage_load_status::loaded)
		{
			storage.set_data_directory(executable_directory);
			read_only = true;
			persistence_error = QStringLiteral("程序目录数据已只读打开，但无法迁移到用户目录。%1").arg(persistence_error);
			current_persistence_status = persistence_start_status::read_only_unwritable;
			return current_persistence_status;
		}
		read_only = true;
		persistence_error = source_error;
		current_persistence_status = source_status == storage_load_status::newer_version
			? persistence_start_status::read_only_newer_version : persistence_start_status::read_only_corrupt;
		return current_persistence_status;
	}

	if (fallback_exists)
	{
		load_storage_directory(fallback_directory, fallback_directory_writable, true, persistence_start_status::fallback_ready);
		return current_persistence_status;
	}

	schoolsnapshot empty_snapshot;
	empty_snapshot.saved_at = QDateTime::currentDateTime();
	if (executable_directory_writable)
	{
		storage.set_data_directory(executable_directory);
		if (storage.create_initial_file(empty_snapshot, &persistence_error))
		{
			read_only = false;
			current_persistence_status = persistence_start_status::ready;
			return current_persistence_status;
		}
	}
	if (fallback_directory_writable)
	{
		storage.set_data_directory(fallback_directory);
		if (storage.create_initial_file(empty_snapshot, &persistence_error))
		{
			read_only = false;
			current_persistence_status = persistence_start_status::fallback_ready;
			return current_persistence_status;
		}
	}
	read_only = true;
	current_persistence_status = persistence_start_status::read_only_unwritable;
	return current_persistence_status;
}

bool school::resolve_persistence_conflict(bool use_executable_data)//确认双目录冲突使用哪份数据
{
	if (current_persistence_status != persistence_start_status::data_conflict)
		return false;
	const schoolsnapshot& selected = use_executable_data ? executable_candidate : fallback_candidate;
	const bool candidate_valid = use_executable_data ? executable_candidate_valid : fallback_candidate_valid;
	const bool writable = use_executable_data ? executable_directory_writable : fallback_directory_writable;
	if (!candidate_valid || !apply_loaded_snapshot(selected))
		return false;
	storage.set_data_directory(use_executable_data ? schoolstorage::executable_data_directory() : schoolstorage::fallback_data_directory());
	read_only = !writable;
	current_persistence_status = use_executable_data
		? (writable ? persistence_start_status::ready : persistence_start_status::read_only_unwritable)
		: (writable ? persistence_start_status::fallback_ready : persistence_start_status::read_only_unwritable);
	return true;
	};
	return finish_persistent_mutation(operation());
}

bool school::is_persistence_ready() const
{
	return persistence_initialized && !storage.data_directory().isEmpty()
		&& current_persistence_status != persistence_start_status::data_conflict
		&& current_persistence_status != persistence_start_status::read_only_corrupt
		&& current_persistence_status != persistence_start_status::read_only_newer_version;
}
bool school::is_read_only() const { return read_only; }
QString school::active_data_directory() const { return storage.data_directory(); }
QString school::last_persistence_error() const { return persistence_error; }
int school::last_persistence_error_code() const { return persistence_error_code; }
persistence_start_status school::persistence_status() const { return current_persistence_status; }

QString school::persistence_candidate_summary(bool executable_data) const
{
	const schoolsnapshot& candidate = executable_data ? executable_candidate : fallback_candidate;
	const QString directory = executable_data ? schoolstorage::executable_data_directory() : schoolstorage::fallback_data_directory();
	const bool writable = executable_data ? executable_directory_writable : fallback_directory_writable;
	return QStringLiteral("目录：%1\n保存时间：%2\n学生：%3 人\n楼栋：%4 栋\n宿舍：%5 间\n使用方式：%6")
		.arg(directory, candidate.saved_at.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
		.arg(candidate.students.size()).arg(candidate.buildings.size()).arg(candidate.dorms.size())
		.arg(writable ? QStringLiteral("可正常读写") : QStringLiteral("目录不可写，只读打开"));
}

bool school::load_storage_directory(const QString& directory, bool writable, bool fallback_directory,
	persistence_start_status normal_status)
{
	storage.set_data_directory(directory);
	schoolsnapshot snapshot;
	QString load_error;
	const storage_load_status primary_status = storage.load_primary(snapshot, &load_error);
	if (primary_status == storage_load_status::loaded && apply_loaded_snapshot(snapshot))
	{
		read_only = !writable;
		current_persistence_status = writable ? normal_status : persistence_start_status::read_only_unwritable;
		return true;
	}
	if (persistence_restore_rollback_failed)
	{
		read_only = true;
		persistence_error = QStringLiteral("加载正式数据失败，且原内存状态未能完整恢复。请立即停止后续操作。");
		current_persistence_status = persistence_start_status::read_only_corrupt;
		return false;
	}
	if (primary_status == storage_load_status::newer_version)
	{
		read_only = true;
		persistence_error = load_error;
		current_persistence_status = persistence_start_status::read_only_newer_version;
		return false;
	}
	bool primary_safe_to_rebuild = primary_status == storage_load_status::not_found;
	if (primary_status == storage_load_status::invalid && writable)
	{
		QString archive_error;
		primary_safe_to_rebuild = storage.archive_invalid_file(storage.primary_file_path(), nullptr, &archive_error);
		if (!primary_safe_to_rebuild)
			load_error = QStringLiteral("%1\n损坏正式文件无法安全保留：%2").arg(load_error, archive_error);
	}
	schoolsnapshot backup_snapshot;
	QString backup_error;
	const storage_load_status backup_status = storage.load_backup(backup_snapshot, &backup_error);
	if (backup_status == storage_load_status::loaded && apply_loaded_snapshot(backup_snapshot))
	{
		if (writable && primary_safe_to_rebuild && !storage.rebuild_primary(backup_snapshot, &persistence_error))
		{
			read_only = true;
			current_persistence_status = persistence_start_status::read_only_unwritable;
			return false;
		}
		read_only = !writable || !primary_safe_to_rebuild;
		current_persistence_status = writable && primary_safe_to_rebuild ? persistence_start_status::backup_restored
			: persistence_start_status::read_only_unwritable;
		return true;
	}
	if (persistence_restore_rollback_failed)
	{
		read_only = true;
		persistence_error = QStringLiteral("加载备份数据失败，且原内存状态未能完整恢复。请立即停止后续操作。");
		current_persistence_status = persistence_start_status::read_only_corrupt;
		return false;
	}
	if (backup_status == storage_load_status::newer_version)
	{
		read_only = true;
		persistence_error = backup_error;
		current_persistence_status = persistence_start_status::read_only_newer_version;
		return false;
	}
	if (backup_status == storage_load_status::invalid && writable)
	{
		QString archive_error;
		if (!storage.archive_invalid_file(storage.backup_file_path(), nullptr, &archive_error))
			backup_error = QStringLiteral("%1\n损坏备份无法安全保留：%2").arg(backup_error, archive_error);
	}
	read_only = true;
	persistence_error = QStringLiteral("正式数据无法读取：%1\n备份数据无法读取：%2").arg(load_error, backup_error);
	current_persistence_status = writable || fallback_directory
		? persistence_start_status::read_only_corrupt : persistence_start_status::read_only_unwritable;
	return false;
}

bool school::apply_loaded_snapshot(const schoolsnapshot& snapshot)
{
	persistence_restore_rollback_failed = false;
	persistence_suspended = true;
	const bool restored = restore_persistence_snapshot(snapshot);
	persistence_suspended = false;
	return restored;
}

bool school::begin_persistent_mutation()//建立最外层写事务
{
	if (persistence_suspended || !persistence_initialized)
	{
		++mutation_depth;
		return true;
	}
	if (read_only)
	{
		persistence_error_code = -102;
		persistence_error = QStringLiteral("当前处于只读安全模式，不能修改数据。请先处理数据文件问题并重新启动程序。");
		return false;
	}
	if (mutation_depth == 0)
	{
		mutation_before = create_persistence_snapshot();
		mutation_before_valid = !schoolstorage::encode_snapshot(mutation_before, &persistence_error).isEmpty();
		mutation_business_failed = !mutation_before_valid;
		persistence_error_code = 0;
	}
	++mutation_depth;
	return mutation_before_valid;
}

int school::finish_persistent_mutation(int result, bool business_success)//提交最外层int写事务
{
	if (mutation_depth <= 0)
		return result;
	mutation_business_failed = mutation_business_failed || !business_success;
	--mutation_depth;
	if (mutation_depth > 0 || persistence_suspended || !persistence_initialized)
		return result;
	const schoolsnapshot after = create_persistence_snapshot();
	const bool changed = mutation_before_valid && !after.data_equals(mutation_before);
	if (mutation_business_failed)
	{
		if (changed)
		{
			persistence_suspended = true;
			const bool restored = restore_persistence_snapshot(mutation_before)
				&& create_persistence_snapshot().data_equals(mutation_before);
			persistence_suspended = false;
			if (!restored)
			{
				read_only = true;
				persistence_error_code = -101;
				persistence_error = QStringLiteral("业务失败后未能完整恢复操作前数据，请立即停止后续操作并重新启动程序。");
				return -101;
			}
		}
		mutation_before_valid = false;
		return result;
	}
	if (!changed)
	{
		mutation_before_valid = false;
		return result;
	}
	if (storage.save_snapshot(after, &persistence_error))
	{
		mutation_before_valid = false;
		persistence_error_code = 0;
		return result;
	}
	persistence_suspended = true;
	const bool restored = restore_persistence_snapshot(mutation_before)
		&& create_persistence_snapshot().data_equals(mutation_before);
	persistence_suspended = false;
	mutation_before_valid = false;
	if (!restored)
	{
		read_only = true;
		persistence_error_code = -101;
		persistence_error = QStringLiteral("保存失败且内存数据未能完整恢复，请立即停止后续操作并重新启动程序。");
		return -101;
	}
	persistence_error_code = -100;
	persistence_error = QStringLiteral("操作未保存，系统已恢复到操作前状态。请检查数据目录权限或磁盘空间后重试。");
	return -100;
}

bool school::finish_persistent_mutation(bool result)//提交兼容bool写事务
{
	const int finalized = finish_persistent_mutation(result ? 1 : 0, result);
	return finalized == 1;
}

schoolsnapshot school::create_persistence_snapshot() const//导出完整学校持久化快照
{
	schoolsnapshot snapshot;
	snapshot.saved_at = QDateTime::currentDateTime();
	for (int building_id : get_all_building_ids())
	{
		const building* current_building = get_building(building_id);
		if (current_building != nullptr)
			snapshot.buildings.append({current_building->get_id(), current_building->get_max_floor(), current_building->get_for_gender()});
	}
	for (const auto& key : get_all_dorm_keys())
	{
		const dorm* current_dorm = get_dorm(key.first, key.second);
		if (current_dorm != nullptr)
			snapshot.dorms.append({key.first, key.second, current_dorm->get_max_num(),
				current_dorm->get_for_gender(), snapshot_dorm(*current_dorm)});
	}
	for (int student_id : get_all_student_ids())
	{
		const student* current_student = get_student(student_id);
		if (current_student != nullptr)
			snapshot.students.append({current_student->get_id(), current_student->get_name(), current_student->get_gender(),
				current_student->get_grade(), current_student->get_class_num(), current_student->get_building_id(),
				current_student->get_dorm_id(), current_student->get_floor(), current_student->get_bed_id()});
	}
	return snapshot;
}

bool school::restore_persistence_snapshot(const schoolsnapshot& snapshot)//整体恢复持久化快照并核对结果
{
	persistence_restore_rollback_failed = false;
	QString validation_error;
	if (schoolstorage::encode_snapshot(snapshot, &validation_error).isEmpty())
		return false;
	sampledataplan plan;
	for (const building_snapshot& item : snapshot.buildings)
		plan.buildings.append({item.id, item.gender, item.max_floor});
	for (const dorm_snapshot& item : snapshot.dorms)
	{
		if (item.beds.size() != item.max_beds)
			return false;
		plan.dorms.append({item.building_id, item.dorm_id, item.max_beds, item.gender_lock});
	}
	QHash<int, const student_snapshot*> students;
	for (const student_snapshot& item : snapshot.students)
	{
		if (students.contains(item.id))
			return false;
		students.insert(item.id, &item);
		const bool all_empty = item.building_id == 0 && item.dorm_id == 0 && item.floor == 0 && item.bed_id == 0;
		const bool all_assigned = item.building_id > 0 && item.dorm_id > 0 && item.floor > 0 && item.bed_id > 0;
		if (!all_empty && !all_assigned)
			return false;
		plan.students.append({item.id, item.name, item.gender, item.class_number, item.grade,
			item.building_id, item.dorm_id, item.bed_id});
	}
	QSet<int> students_in_beds;
	for (const dorm_snapshot& item : snapshot.dorms)
	{
		for (int bed_index = 0; bed_index < item.beds.size(); ++bed_index)
		{
			const int student_id = item.beds.at(bed_index);
			if (student_id == 0)
				continue;
			if (students_in_beds.contains(student_id))
				return false;
			students_in_beds.insert(student_id);
			const student_snapshot* current_student = students.value(student_id, nullptr);
			if (current_student == nullptr || current_student->building_id != item.building_id
				|| current_student->dorm_id != item.dorm_id || current_student->bed_id != bed_index + 1
				|| current_student->floor != item.dorm_id / 100)
				return false;
		}
	}
	for (const student_snapshot& item : snapshot.students)
	{
		const bool assigned = item.building_id > 0;
		if (assigned != students_in_beds.contains(item.id))
			return false;
	}
	if (!validate_sample_data_plan(plan, true))
		return false;
	const school_data_snapshot before = take_school_data_snapshot();
	const schoolsnapshot before_persistence = create_persistence_snapshot();
	if (schoolstorage::encode_snapshot(before_persistence, &validation_error).isEmpty())
		return false;
	if (!restore_school_data_snapshot({plan}))
	{
		if (!restore_school_data_snapshot(before) || !create_persistence_snapshot().data_equals(before_persistence))
		{
			persistence_restore_rollback_failed = true;
			return false;
		}
		return false;
	}
	const schoolsnapshot restored = create_persistence_snapshot();
	if (!restored.data_equals(snapshot))
	{
		if (!restore_school_data_snapshot(before) || !create_persistence_snapshot().data_equals(before_persistence))
		{
			persistence_restore_rollback_failed = true;
			return false;
		}
		return false;
	}
	return true;
}

int school::replace_all_with_sample_data(const sampledataplan& plan)//清空后生成样例数据
{
	if (plan.buildings.isEmpty() || plan.dorms.isEmpty() || plan.students.isEmpty() || !validate_sample_data_plan(plan)) return -1;
	if (!collect_accommodation_issues().isEmpty()) return -8;
	const school_data_snapshot snapshot = take_school_data_snapshot();
	if (!validate_sample_data_plan(snapshot.data, true)) return -8;
	if (!purge_all_data()) return restore_school_data_snapshot(snapshot) ? -5 : -6;
	if (write_sample_data_plan(plan)) return 1;
	return restore_school_data_snapshot(snapshot) ? -5 : -6;
}

batch_assignment_preview school::preview_assign_unassigned_students(assignment_strategy strategy, quint32 random_seed) const//生成未入住学生补分预览基础信息
{
	batch_assignment_preview preview;
	preview.strategy = strategy;
	preview.random_seed = random_seed;
	preview.resource_signature = accommodation_resource_signature();
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

reassignment_preview school::preview_reassign_all_students(reassignment_strategy strategy, quint32 random_seed) const//生成全校重新安排预览
{
	reassignment_preview preview;
	preview.strategy = strategy;
	preview.random_seed = random_seed;
	preview.resource_signature = accommodation_resource_signature();
	preview.issues = collect_accommodation_issues();
	if (!preview.issues.isEmpty())
		return preview;

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
		if (d != nullptr)
			dorms.append({key.first, key.second, 0, QVector<int>(d->get_max_num(), 0)});
	}

	QVector<int> candidates = get_all_student_ids();
	preview.candidate_count = candidates.size();
	QRandomGenerator random(random_seed);
	if (strategy == reassignment_strategy::random)
	{
		for (int i = candidates.size() - 1; i > 0; --i)
			candidates.swapItemsAt(i, random.bounded(i + 1));
	}
	else if (strategy == reassignment_strategy::preserve_building_first)
	{
		std::stable_sort(candidates.begin(), candidates.end(), [this](int left_id, int right_id)
		{
			const student* left = get_student(left_id);
			const student* right = get_student(right_id);
			const bool left_assigned = left != nullptr && left->get_building_id() > 0;
			const bool right_assigned = right != nullptr && right->get_building_id() > 0;
			return left_assigned != right_assigned ? left_assigned : left_id < right_id;
		});
	}

	auto plan_student = [this, &dorms, &preview, &random, strategy](int student_id, bool original_building_only)
	{
		const student* s = get_student(student_id);
		if (s == nullptr || (s->get_gender() != 1 && s->get_gender() != 2))
			return false;
		QVector<int> compatible;
		for (int i = 0; i < dorms.size(); ++i)
		{
			const building* b = get_building(dorms[i].building_id);
			if (b == nullptr || !b->accepts_gender(s->get_gender())
				|| (dorms[i].gender != 0 && dorms[i].gender != s->get_gender()) || !dorms[i].beds.contains(0)
				|| (original_building_only && dorms[i].building_id != s->get_building_id()))
				continue;
			compatible.append(i);
		}
		if (compatible.isEmpty())
			return false;
		int selected = compatible.first();
		if (strategy == reassignment_strategy::random)
			selected = compatible.at(random.bounded(compatible.size()));
		else
		{
			for (int index : compatible)
			{
				const planned_dorm& current = dorms[index];
				const planned_dorm& best = dorms[selected];
				const int current_occupied = current.beds.size() - std::count(current.beds.cbegin(), current.beds.cend(), 0);
				const int best_occupied = best.beds.size() - std::count(best.beds.cbegin(), best.beds.cend(), 0);
				const bool better = current_occupied * best.beds.size() != best_occupied * current.beds.size()
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
		const int bed_id = strategy == reassignment_strategy::random
			? empty_beds.at(random.bounded(empty_beds.size())) : empty_beds.first();
		target.beds[bed_id - 1] = student_id;
		if (target.gender == 0)
			target.gender = s->get_gender();
		preview.changes.append({student_id, s->get_gender(), s->get_building_id(), s->get_dorm_id(), s->get_bed_id(),
			target.building_id, target.dorm_id, bed_id});
		return true;
	};

	QVector<int> deferred;
	QVector<int> originally_unassigned;
	if (strategy == reassignment_strategy::preserve_building_first)
	{
		for (int student_id : candidates)
		{
			const student* s = get_student(student_id);
			if (s != nullptr && s->get_building_id() > 0)
			{
				if (!plan_student(student_id, true))
					deferred.append(student_id);
			}
			else
				originally_unassigned.append(student_id);
		}
	}
	else
		deferred = candidates;

	for (int student_id : deferred)
		if (!plan_student(student_id, false))
		{
			const student* s = get_student(student_id);
			preview.unassigned_student_ids.append(student_id);
			if (s != nullptr)
				preview.changes.append({student_id, s->get_gender(), s->get_building_id(), s->get_dorm_id(), s->get_bed_id(), 0, 0, 0});
		}
	for (int student_id : originally_unassigned)
		if (!plan_student(student_id, false))
		{
			const student* s = get_student(student_id);
			preview.unassigned_student_ids.append(student_id);
			if (s != nullptr)
				preview.changes.append({student_id, s->get_gender(), 0, 0, 0, 0, 0, 0});
		}
	preview.available_bed_count = preview.changes.size() - preview.unassigned_student_ids.size();
	return preview;
}

int school::apply_batch_assignment(const batch_assignment_preview& preview)//按固定预览执行补分
{
	if (preview.changes.isEmpty())
		return 0;
	if (!collect_accommodation_issues().isEmpty())
		return -8;
	if (preview.resource_signature != accommodation_resource_signature())
		return -7;
	QSet<int> preview_candidates;
	for (const accommodation_change& change : preview.changes) preview_candidates.insert(change.student_id);
	for (int student_id : preview.unassigned_student_ids) preview_candidates.insert(student_id);
	const QVector<int> current_candidate_ids = get_unassigned_student_ids();
	const QSet<int> current_candidates(current_candidate_ids.cbegin(), current_candidate_ids.cend());
	if (preview.candidate_count != current_candidates.size() || preview_candidates != current_candidates)
		return -7;
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

int school::apply_reassignment(const reassignment_preview& preview)//按固定预览执行全校重新安排
{
	if (!collect_accommodation_issues().isEmpty())
		return -8;
	if (preview.resource_signature != accommodation_resource_signature())
		return -7;
	QSet<int> all_preview_students;
	QSet<QString> target_beds;
	QHash<QString, int> simulated_genders;
	for (const accommodation_change& change : preview.changes)
	{
		const student* s = get_student(change.student_id);
		const bool remains_unassigned = change.new_building_id == 0 && change.new_dorm_id == 0 && change.new_bed_id == 0;
		const dorm* d = remains_unassigned ? nullptr : get_dorm(change.new_building_id, change.new_dorm_id);
		const building* b = remains_unassigned ? nullptr : get_building(change.new_building_id);
		if (s == nullptr || all_preview_students.contains(change.student_id)
			|| s->get_gender() != change.student_gender
			|| s->get_building_id() != change.old_building_id || s->get_dorm_id() != change.old_dorm_id
			|| s->get_bed_id() != change.old_bed_id
			|| (change.old_dorm_id > 0 && s->get_floor() != change.old_dorm_id / 100)
			|| (change.old_dorm_id == 0 && s->get_floor() != 0))
			return -7;
		if (remains_unassigned)
			continue;
		if (d == nullptr || b == nullptr || change.new_bed_id <= 0
			|| change.new_bed_id > d->get_max_num() || !b->accepts_gender(s->get_gender()))
			return -7;
		const QString bed_key = QStringLiteral("%1/%2/%3").arg(change.new_building_id).arg(change.new_dorm_id).arg(change.new_bed_id);
		if (target_beds.contains(bed_key))
			return -7;
		target_beds.insert(bed_key);
		all_preview_students.insert(change.student_id);
		const QString dorm_key = QStringLiteral("%1/%2").arg(change.new_building_id).arg(change.new_dorm_id);
		const int planned_gender = simulated_genders.value(dorm_key, 0);
		if (planned_gender != 0 && planned_gender != s->get_gender())
			return -7;
		simulated_genders[dorm_key] = s->get_gender();
	}
	for (int student_id : preview.unassigned_student_ids)
	{
		const student* s = get_student(student_id);
		auto change_it = std::find_if(preview.changes.cbegin(), preview.changes.cend(), [student_id](const accommodation_change& change)
		{
			return change.student_id == student_id;
		});
		if (s == nullptr || change_it == preview.changes.cend() || all_preview_students.contains(student_id)
			|| change_it->new_building_id != 0 || change_it->new_dorm_id != 0 || change_it->new_bed_id != 0
			|| s->get_gender() != change_it->student_gender || s->get_building_id() != change_it->old_building_id
			|| s->get_dorm_id() != change_it->old_dorm_id || s->get_bed_id() != change_it->old_bed_id)
			return -7;
		all_preview_students.insert(student_id);
	}
	if (all_preview_students.size() != get_student_count())
		return -7;

	const accommodation_snapshot snapshot = take_accommodation_snapshot();
	for (const auto& key : get_all_dorm_keys())
		if (dormmanager::instance().clear_dorm_students(key.first, key.second, true) < 0)
			return restore_accommodation_snapshot(snapshot) ? -5 : -6;
	for (int student_id : get_all_student_ids())
		if (studentmanager::instance().clear_dorm_info(student_id) != 1)
			return restore_accommodation_snapshot(snapshot) ? -5 : -6;
	for (const accommodation_change& change : preview.changes)
	{
		if (change.new_building_id == 0)
			continue;
		if (assign_student_to_dorm(change.new_building_id, change.new_dorm_id, change.student_id, change.new_bed_id) != change.new_bed_id)
			return restore_accommodation_snapshot(snapshot) ? -5 : -6;
	}
	return preview.changes.size() - preview.unassigned_student_ids.size();
}

int school::reassign_all_students_random()//清空后为全校学生随机重排宿舍
{
	const reassignment_preview preview = preview_reassign_all_students(reassignment_strategy::random, QRandomGenerator::global()->generate());
	if (!preview.issues.isEmpty())
		return -8;
	const int result = apply_reassignment(preview);
	return result >= 0 ? preview.unassigned_student_ids.size() : result;
}

batch_clear_preview school::preview_clear_dorms(clear_scope scope, int building_id, int dorm_id, bool reset_gender) const//生成批量清退预览
{
	batch_clear_preview preview;
	preview.scope = scope;
	preview.building_id = building_id;
	preview.dorm_id = dorm_id;
	preview.reset_gender = reset_gender;
	preview.issues = collect_accommodation_issues();
	if (!preview.issues.isEmpty())
		return preview;

	QVector<QPair<int, int>> keys;
	if (scope == clear_scope::dorm)
	{
		if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || get_dorm(building_id, dorm_id) == nullptr)
		{
			preview.issues.append({0, building_id, dorm_id, QStringLiteral("指定宿舍不存在。")});
			return preview;
		}
		keys.append(qMakePair(building_id, dorm_id));
	}
	else if (scope == clear_scope::building)
	{
		if (!check::is_valid_building_id(building_id) || get_building(building_id) == nullptr)
		{
			preview.issues.append({0, building_id, 0, QStringLiteral("指定宿舍楼不存在。")});
			return preview;
		}
		keys = get_dorm_keys_of_building(building_id);
	}
	else
		keys = get_all_dorm_keys();

	for (const auto& key : keys)
	{
		const dorm* d = get_dorm(key.first, key.second);
		if (d == nullptr)
			continue;
		preview.dorm_changes.append({key.first, key.second, d->get_for_gender(), reset_gender ? 0 : d->get_for_gender()});
		for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
		{
			const int student_id = d->get_student_id(bed_id);
			if (student_id <= 0)
				continue;
			const student* s = get_student(student_id);
			preview.changes.append({student_id, s == nullptr ? 0 : s->get_gender(), key.first, key.second, bed_id, 0, 0, 0});
		}
	}
	return preview;
}

int school::apply_batch_clear(const batch_clear_preview& preview)//按固定预览执行批量清退
{
	if (!collect_accommodation_issues().isEmpty())
		return -8;
	QVector<QPair<int, int>> current_keys;
	if (preview.scope == clear_scope::dorm)
		current_keys.append(qMakePair(preview.building_id, preview.dorm_id));
	else if (preview.scope == clear_scope::building)
		current_keys = get_dorm_keys_of_building(preview.building_id);
	else
		current_keys = get_all_dorm_keys();
	QSet<QString> preview_keys;
	for (const dorm_gender_change& change : preview.dorm_changes)
		preview_keys.insert(QStringLiteral("%1/%2").arg(change.building_id).arg(change.dorm_id));
	QSet<QString> actual_keys;
	for (const auto& key : current_keys) actual_keys.insert(QStringLiteral("%1/%2").arg(key.first).arg(key.second));
	if (preview_keys != actual_keys)
		return -7;
	QVector<dorm_accommodation_snapshot> snapshots;
	QSet<int> affected_students;
	int current_occupants = 0;
	for (const dorm_gender_change& dorm_change : preview.dorm_changes)
	{
		const dorm* d = get_dorm(dorm_change.building_id, dorm_change.dorm_id);
		if (d == nullptr || d->get_for_gender() != dorm_change.old_gender
			|| dorm_change.new_gender != (preview.reset_gender ? 0 : dorm_change.old_gender))
			return -7;
		snapshots.append({dorm_change.building_id, dorm_change.dorm_id, d->get_for_gender(), snapshot_dorm(*d)});
		current_occupants += d->get_current_num();
		for (int student_id : d->get_student_id_list())
			affected_students.insert(student_id);
	}
	if (current_occupants != preview.changes.size() || affected_students.size() != preview.changes.size())
		return -7;
	for (const accommodation_change& change : preview.changes)
	{
		const student* s = get_student(change.student_id);
		const dorm* d = get_dorm(change.old_building_id, change.old_dorm_id);
		if (s == nullptr || d == nullptr || !affected_students.contains(change.student_id)
			|| s->get_gender() != change.student_gender || s->get_building_id() != change.old_building_id
			|| s->get_dorm_id() != change.old_dorm_id || s->get_bed_id() != change.old_bed_id
			|| d->get_student_id(change.old_bed_id) != change.student_id)
			return -7;
	}

	auto restore = [this, &snapshots, &affected_students]()
	{
		bool restored = true;
		for (int student_id : affected_students)
			restored = studentmanager::instance().clear_dorm_info(student_id) == 1 && restored;
		for (const dorm_accommodation_snapshot& snapshot : snapshots)
		{
			restored = restore_dorm(snapshot.building_id, snapshot.dorm_id, snapshot.beds, snapshot.gender) && restored;
			const dorm* d = get_dorm(snapshot.building_id, snapshot.dorm_id);
			if (d == nullptr)
			{
				restored = false;
				continue;
			}
			for (int bed_id = 1; bed_id <= d->get_max_num(); ++bed_id)
			{
				const int student_id = d->get_student_id(bed_id);
				if (student_id > 0)
					restored = studentmanager::instance().assign_dorm_info(student_id, bed_id, d->get_id(), d->get_building_id(), d->get_floor()) == 1 && restored;
			}
		}
		return restored;
	};

	for (const dorm_accommodation_snapshot& snapshot : snapshots)
		if (dormmanager::instance().clear_dorm_students(snapshot.building_id, snapshot.dorm_id, preview.reset_gender) < 0)
			return restore() ? -5 : -6;
	for (int student_id : affected_students)
		if (studentmanager::instance().clear_dorm_info(student_id) != 1)
			return restore() ? -5 : -6;
	return affected_students.size();
}

int school::clear_building_dorms(int building_id)//清退指定楼并保留宿舍性别锁
{
	return apply_batch_clear(preview_clear_dorms(clear_scope::building, building_id, 0, false));
}

int school::clear_building_dorms_reset_gender(int building_id)//清退指定楼并解除宿舍性别锁
{
	return apply_batch_clear(preview_clear_dorms(clear_scope::building, building_id, 0, true));
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

dorm_adjustment_preview school::preview_dorm_adjustment(int b1, int d1, int b2, int d2, dorm_adjustment_mode mode) const//生成宿舍整体调整预览
{
	dorm_adjustment_preview preview;
	preview.mode = mode;
	preview.issues = collect_accommodation_issues();
	if (!preview.issues.isEmpty())
		return preview;
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1)
		|| !check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2) || (b1 == b2 && d1 == d2))
	{
		preview.unavailable_reason = QStringLiteral("请选择两间不同的有效宿舍。");
		return preview;
	}
	const dorm* a = get_dorm(b1, d1);
	const dorm* b = get_dorm(b2, d2);
	const building* building_a = get_building(b1);
	const building* building_b = get_building(b2);
	if (a == nullptr || b == nullptr || building_a == nullptr || building_b == nullptr)
	{
		preview.unavailable_reason = QStringLiteral("所选宿舍或所属宿舍楼不存在。");
		return preview;
	}
	preview.before_a = {b1, d1, a->get_for_gender(), snapshot_dorm(*a)};
	preview.before_b = {b2, d2, b->get_for_gender(), snapshot_dorm(*b)};
	preview.after_a = preview.before_a;
	preview.after_b = preview.before_b;
	const QVector<int> list_a = a->get_student_id_list();
	const QVector<int> list_b = b->get_student_id_list();
	const int overlap = qMin(list_a.size(), list_b.size());

	auto accepts = [this](int building_id, const dorm* target, int student_id)
	{
		const student* s = get_student(student_id);
		const building* target_building = get_building(building_id);
		return s != nullptr && target_building != nullptr && target_building->accepts_gender(s->get_gender())
			&& target->accepts_gender(s->get_gender());
	};
	auto clear_beds = [](dorm_preview_state& state)
	{
		std::fill(state.beds.begin(), state.beds.end(), 0);
	};
	auto remove_id = [](dorm_preview_state& state, int student_id)
	{
		for (int& occupant : state.beds)
			if (occupant == student_id)
			{
				occupant = 0;
				return;
			}
	};
	auto add_lowest = [this](dorm_preview_state& state, int student_id)
	{
		for (int& occupant : state.beds)
			if (occupant == 0)
			{
				occupant = student_id;
				const student* s = get_student(student_id);
				if (state.gender == 0 && s != nullptr)
					state.gender = s->get_gender();
				return true;
			}
		return false;
	};

	if (mode == dorm_adjustment_mode::full_swap)
	{
		if (a->get_for_gender() != b->get_for_gender())
			preview.unavailable_reason = QStringLiteral("两间宿舍的宿舍性别锁不同，不能完整交换。");
		else if (list_a.size() != list_b.size())
			preview.unavailable_reason = QStringLiteral("两间宿舍住客人数不同，不能完整交换。");
		else
		{
			for (int id : list_a)
				if (!accepts(b2, b, id)) preview.unavailable_reason = QStringLiteral("宿舍A中至少一名学生不符合宿舍B的性别要求。");
			for (int id : list_b)
				if (!accepts(b1, a, id)) preview.unavailable_reason = QStringLiteral("宿舍B中至少一名学生不符合宿舍A的性别要求。");
			if (preview.unavailable_reason.isEmpty())
			{
				clear_beds(preview.after_a);
				clear_beds(preview.after_b);
				for (int id : list_b) add_lowest(preview.after_a, id);
				for (int id : list_a) add_lowest(preview.after_b, id);
			}
		}
	}
	else if (mode == dorm_adjustment_mode::overlap_swap || mode == dorm_adjustment_mode::overlap_swap_and_evict)
	{
		if (a->get_for_gender() != b->get_for_gender())
			preview.unavailable_reason = QStringLiteral("两间宿舍的宿舍性别锁不同，不能使用此调整方式。");
		else
		{
			for (int i = 0; i < overlap; ++i)
				if (!accepts(b2, b, list_a[i]) || !accepts(b1, a, list_b[i]))
					preview.unavailable_reason = QStringLiteral("至少一名学生不符合目标宿舍的性别要求。");
			if (preview.unavailable_reason.isEmpty() && overlap > 0)
			{
				if (mode == dorm_adjustment_mode::overlap_swap_and_evict)
				{
					clear_beds(preview.after_a);
					clear_beds(preview.after_b);
				}
				else
					for (int i = 0; i < overlap; ++i)
					{
						remove_id(preview.after_a, list_a[i]);
						remove_id(preview.after_b, list_b[i]);
					}
				for (int i = 0; i < overlap; ++i)
				{
					add_lowest(preview.after_a, list_b[i]);
					add_lowest(preview.after_b, list_a[i]);
				}
			}
		}
	}
	else
	{
		if (building_a->get_for_gender() != 3 || building_b->get_for_gender() != 3)
			preview.unavailable_reason = QStringLiteral("男女宿舍交换只适用于混合宿舍楼中的宿舍。");
		else if (!((a->get_for_gender() == 1 && b->get_for_gender() == 2) || (a->get_for_gender() == 2 && b->get_for_gender() == 1)))
			preview.unavailable_reason = QStringLiteral("请选择一间男生宿舍和一间女生宿舍。");
		else if (overlap > 0)
		{
			clear_beds(preview.after_a);
			clear_beds(preview.after_b);
			preview.after_a.gender = 0;
			preview.after_b.gender = 0;
			for (int i = 0; i < overlap; ++i)
			{
				add_lowest(preview.after_a, list_b[i]);
				add_lowest(preview.after_b, list_a[i]);
			}
		}
	}
	if (!preview.unavailable_reason.isEmpty())
		return preview;

	QSet<int> affected;
	for (int id : list_a) affected.insert(id);
	for (int id : list_b) affected.insert(id);
	auto find_bed = [](const dorm_preview_state& state, int student_id)
	{
		for (int i = 0; i < state.beds.size(); ++i)
			if (state.beds[i] == student_id) return i + 1;
		return 0;
	};
	for (int student_id : affected)
	{
		const student* s = get_student(student_id);
		const int new_bed_a = find_bed(preview.after_a, student_id);
		const int new_bed_b = find_bed(preview.after_b, student_id);
		preview.changes.append({student_id, s == nullptr ? 0 : s->get_gender(),
			s == nullptr ? 0 : s->get_building_id(), s == nullptr ? 0 : s->get_dorm_id(), s == nullptr ? 0 : s->get_bed_id(),
			new_bed_a > 0 ? b1 : new_bed_b > 0 ? b2 : 0,
			new_bed_a > 0 ? d1 : new_bed_b > 0 ? d2 : 0,
			new_bed_a > 0 ? new_bed_a : new_bed_b});
	}
	preview.available = true;
	return preview;
}

int school::apply_dorm_adjustment(const dorm_adjustment_preview& preview)//核对并执行宿舍整体调整
{
	const dorm_adjustment_preview current = preview_dorm_adjustment(
		preview.before_a.building_id, preview.before_a.dorm_id, preview.before_b.building_id, preview.before_b.dorm_id, preview.mode);
	if (!current.issues.isEmpty())
		return -5;
	if (!current.available || current.before_a.beds != preview.before_a.beds || current.before_b.beds != preview.before_b.beds
		|| current.before_a.gender != preview.before_a.gender || current.before_b.gender != preview.before_b.gender)
		return -5;
	switch (preview.mode)
	{
	case dorm_adjustment_mode::full_swap:
		return swap_dorms(preview.before_a.building_id, preview.before_a.dorm_id, preview.before_b.building_id, preview.before_b.dorm_id);
	case dorm_adjustment_mode::overlap_swap:
		return swap_dorms_overlap(preview.before_a.building_id, preview.before_a.dorm_id, preview.before_b.building_id, preview.before_b.dorm_id);
	case dorm_adjustment_mode::overlap_swap_and_evict:
		return swap_dorms_overlap_evict(preview.before_a.building_id, preview.before_a.dorm_id, preview.before_b.building_id, preview.before_b.dorm_id);
	case dorm_adjustment_mode::gender_dorm_swap:
		return swap_gender_dorms(preview.before_a.building_id, preview.before_a.dorm_id, preview.before_b.building_id, preview.before_b.dorm_id);
	}
	return -1;
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
