#include "sampledatagenerator.h"

#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"
#include "system/check.h"

#include <QHash>
#include <QPair>
#include <QRandomGenerator>
#include <QSet>
#include <QVector>

#include <limits>

namespace {
using plannedbuilding = samplebuildingplan;
using planneddorm = sampledormplan;
using plannedstudent = samplestudentplan;

template<typename T>
void shuffle_items(QVector<T>& items, QRandomGenerator& random)
{
	for (int i = items.size() - 1; i > 0; --i) {
		const int target = random.bounded(i + 1);
		items.swapItemsAt(i, target);
	}
}

int dorm_capacity(const QVector<planneddorm>& dorms, const QVector<int>& indexes)
{
	int result = 0;
	for (int index : indexes) {
		result += dorms.at(index).max_num;
	}
	return result;
}

QVector<int> choose_male_shared_dorms(const QVector<planneddorm>& dorms, const QVector<int>& shared_indexes,
	int male_needed, int female_needed, bool& found)
{
	found = false;
	int total_capacity = 0;
	for (int index : shared_indexes) {
		total_capacity += dorms.at(index).max_num;
	}
	if (male_needed < 0 || female_needed < 0 || male_needed + female_needed > total_capacity) {
		return {};
	}

	QVector<bool> reachable(total_capacity + 1, false);
	QVector<int> previous_sum(total_capacity + 1, -1);
	QVector<int> previous_dorm(total_capacity + 1, -1);
	reachable[0] = true;
	int processed_capacity = 0;
	for (int dorm_index : shared_indexes) {
		const int capacity = dorms.at(dorm_index).max_num;
		for (int sum = processed_capacity; sum >= 0; --sum) {
			if (!reachable.at(sum) || reachable.at(sum + capacity)) {
				continue;
			}
			reachable[sum + capacity] = true;
			previous_sum[sum + capacity] = sum;
			previous_dorm[sum + capacity] = dorm_index;
		}
		processed_capacity += capacity;
	}

	int selected_sum = -1;
	const int maximum_male_capacity = total_capacity - female_needed;
	for (int sum = male_needed; sum <= maximum_male_capacity; ++sum) {
		if (reachable.at(sum)) {
			selected_sum = sum;
			break;
		}
	}
	if (selected_sum < 0) {
		return {};
	}
	found = true;

	QVector<int> selected;
	while (selected_sum > 0) {
		const int dorm_index = previous_dorm.at(selected_sum);
		if (dorm_index < 0) {
			return {};
		}
		selected.append(dorm_index);
		selected_sum = previous_sum.at(selected_sum);
	}
	return selected;
}

QString random_student_name(QRandomGenerator& random)
{
	static const QStringList surnames = {
		QStringLiteral("赵"), QStringLiteral("钱"), QStringLiteral("孙"), QStringLiteral("李"), QStringLiteral("周"),
		QStringLiteral("吴"), QStringLiteral("郑"), QStringLiteral("王"), QStringLiteral("冯"), QStringLiteral("陈"),
		QStringLiteral("褚"), QStringLiteral("卫"), QStringLiteral("蒋"), QStringLiteral("沈"), QStringLiteral("韩"),
		QStringLiteral("杨"), QStringLiteral("朱"), QStringLiteral("秦"), QStringLiteral("许"), QStringLiteral("何"),
		QStringLiteral("吕"), QStringLiteral("施"), QStringLiteral("张"), QStringLiteral("孔"), QStringLiteral("曹"),
		QStringLiteral("严"), QStringLiteral("华"), QStringLiteral("金"), QStringLiteral("魏"), QStringLiteral("陶")
	};
	static const QStringList given_characters = {
		QStringLiteral("安"), QStringLiteral("博"), QStringLiteral("晨"), QStringLiteral("辰"), QStringLiteral("川"),
		QStringLiteral("德"), QStringLiteral("凡"), QStringLiteral("航"), QStringLiteral("浩"), QStringLiteral("嘉"),
		QStringLiteral("杰"), QStringLiteral("锦"), QStringLiteral("景"), QStringLiteral("静"), QStringLiteral("君"),
		QStringLiteral("可"), QStringLiteral("乐"), QStringLiteral("林"), QStringLiteral("明"), QStringLiteral("宁"),
		QStringLiteral("琪"), QStringLiteral("清"), QStringLiteral("然"), QStringLiteral("睿"), QStringLiteral("思"),
		QStringLiteral("彤"), QStringLiteral("文"), QStringLiteral("欣"), QStringLiteral("轩"), QStringLiteral("妍"),
		QStringLiteral("阳"), QStringLiteral("怡"), QStringLiteral("宇"), QStringLiteral("悦"), QStringLiteral("泽")
	};
	QString result = surnames.at(random.bounded(static_cast<int>(surnames.size())));
	result += given_characters.at(random.bounded(static_cast<int>(given_characters.size())));
	if (random.bounded(100) < 65) {
		result += given_characters.at(random.bounded(static_cast<int>(given_characters.size())));
	}
	return result;
}

int bounded_preview_value(qint64 value)
{
	if (value > std::numeric_limits<int>::max()) {
		return std::numeric_limits<int>::max();
	}
	if (value < std::numeric_limits<int>::min()) {
		return std::numeric_limits<int>::min();
	}
	return static_cast<int>(value);
}

int bounded_preview_product(int first, int second)
{
	return bounded_preview_value(static_cast<qint64>(first) * second);
}

void rollback_created_data(school& current_school, const QVector<int>& student_ids,
	const QVector<QPair<int, int>>& dorm_keys, const QVector<int>& building_ids, sampledataresult& result)
{
	for (auto iterator = student_ids.crbegin(); iterator != student_ids.crend(); ++iterator) {
		if (current_school.get_student(*iterator) != nullptr && current_school.remove_student(*iterator) != 1) {
			result.rollback_complete = false;
		}
	}
	for (auto iterator = dorm_keys.crbegin(); iterator != dorm_keys.crend(); ++iterator) {
		if (current_school.get_dorm(iterator->first, iterator->second) != nullptr
			&& !current_school.remove_dorm(iterator->first, iterator->second)) {
			result.rollback_complete = false;
		}
	}
	for (auto iterator = building_ids.crbegin(); iterator != building_ids.crend(); ++iterator) {
		if (current_school.get_building(*iterator) != nullptr && !current_school.remove_building(*iterator)) {
			result.rollback_complete = false;
		}
	}

	for (int student_id : student_ids) {
		result.residual_student_count += current_school.get_student(student_id) != nullptr ? 1 : 0;
	}
	for (const QPair<int, int>& dorm_key : dorm_keys) {
		result.residual_dorm_count += current_school.get_dorm(dorm_key.first, dorm_key.second) != nullptr ? 1 : 0;
	}
	for (int building_id : building_ids) {
		result.residual_building_count += current_school.get_building(building_id) != nullptr ? 1 : 0;
	}
	result.rollback_complete = result.rollback_complete
		&& result.residual_student_count == 0
		&& result.residual_dorm_count == 0
		&& result.residual_building_count == 0;
}
}

struct sampledatagenerator::plan
{
	QString error_message;
	QVector<plannedbuilding> buildings;
	QVector<planneddorm> dorms;
	QVector<plannedstudent> students;
	int remaining_unlocked_dorm_count = 0;
};

sampledatapreview sampledatagenerator::preview(const sampledataconfig& config)//计算预计新增规模
{
	sampledatapreview result;
	result.building_count = bounded_preview_value(static_cast<qint64>(config.male_building_count)
		+ config.female_building_count + config.mixed_building_count);
	const int dorms_per_building = bounded_preview_product(config.floors_per_building, config.dorms_per_floor);
	result.dorm_count = bounded_preview_product(result.building_count, dorms_per_building);
	const int beds_per_building = bounded_preview_value(static_cast<qint64>(config.four_bed_dorm_count) * 4
		+ static_cast<qint64>(config.six_bed_dorm_count) * 6);
	result.bed_count = bounded_preview_product(result.building_count, beds_per_building);
	result.student_count = bounded_preview_value(static_cast<qint64>(config.male_student_count)
		+ config.female_student_count);
	result.assigned_student_count = bounded_preview_value(static_cast<qint64>(config.male_assigned_count)
		+ config.female_assigned_count);
	result.unassigned_student_count = bounded_preview_value(static_cast<qint64>(config.male_student_count)
		+ config.female_student_count - config.male_assigned_count - config.female_assigned_count);
	result.reserved_unlocked_dorm_count = config.minimum_unlocked_empty_dorm_count;
	return result;
}

QStringList sampledatagenerator::validate_config(const sampledataconfig& config, const school& current_school)//校验生成参数
{
	QStringList errors;
	const sampledatapreview scale = preview(config);
	if (config.male_building_count < 0 || config.female_building_count < 0 || config.mixed_building_count < 0
		|| scale.building_count <= 0) {
		errors.append(QStringLiteral("至少需要生成一栋宿舍楼，且各类楼栋数量不能为负数。"));
	}
	if (scale.building_count > maximum_building_count) {
		errors.append(QStringLiteral("单次生成的宿舍楼不能超过%1栋。").arg(maximum_building_count));
	}
	if (scale.building_count > 99 - (config.mode == sampledatamode::append ? current_school.get_building_count() : 0)) {
		errors.append(QStringLiteral("剩余楼号不足，无法追加指定数量的宿舍楼。"));
	}
	if (config.floors_per_building < 1 || config.floors_per_building > 99
		|| config.dorms_per_floor < 1 || config.dorms_per_floor > 99) {
		errors.append(QStringLiteral("每栋楼层数和每层宿舍数必须在1～99之间。"));
	}

	const int dorms_per_building = bounded_preview_product(config.floors_per_building, config.dorms_per_floor);
	const qint64 capacity_dorm_count = static_cast<qint64>(config.four_bed_dorm_count)
		+ config.six_bed_dorm_count;
	if (config.four_bed_dorm_count < 0 || config.six_bed_dorm_count < 0
		|| capacity_dorm_count != dorms_per_building) {
		errors.append(QStringLiteral("每栋4人间与6人间数量之和必须等于每栋宿舍总数。"));
	}
	if (scale.dorm_count > maximum_dorm_count) {
		errors.append(QStringLiteral("单次生成的宿舍不能超过%1间。").arg(maximum_dorm_count));
	}

	if (config.mixed_building_count > 0) {
		const qint64 mixed_dorm_count = static_cast<qint64>(config.mixed_male_dorm_count)
			+ config.mixed_female_dorm_count + config.mixed_unlocked_dorm_count;
		if (config.mixed_male_dorm_count < 0 || config.mixed_female_dorm_count < 0
			|| config.mixed_unlocked_dorm_count < 0
			|| mixed_dorm_count != dorms_per_building) {
			errors.append(QStringLiteral("混合楼男生宿舍、女生宿舍与宿舍性别锁未设置的宿舍数量之和必须等于每栋宿舍总数。"));
		}
		const qint64 total_unlocked = static_cast<qint64>(config.mixed_building_count)
			* config.mixed_unlocked_dorm_count;
		if (config.minimum_unlocked_empty_dorm_count < 0
			|| config.minimum_unlocked_empty_dorm_count > total_unlocked) {
			errors.append(QStringLiteral("保留的宿舍性别锁未设置空宿舍数量不能超过对应宿舍总数。"));
		}
	} else if (config.minimum_unlocked_empty_dorm_count != 0) {
		errors.append(QStringLiteral("未生成混合宿舍楼时，保留的宿舍性别锁未设置空宿舍数量必须为0。"));
	}

	if (config.male_student_count < 0 || config.female_student_count < 0
		|| scale.student_count <= 0) {
		errors.append(QStringLiteral("至少需要生成一名学生，且男女生数量不能为负数。"));
	}
	if (scale.student_count > maximum_student_count) {
		errors.append(QStringLiteral("单次生成的学生不能超过%1人。").arg(maximum_student_count));
	}
	if (config.male_assigned_count < 0 || config.male_assigned_count > config.male_student_count
		|| config.female_assigned_count < 0 || config.female_assigned_count > config.female_student_count) {
		errors.append(QStringLiteral("男女入住人数必须分别处于0到对应学生总数之间。"));
	}
	if (config.minimum_class_num < 1 || config.maximum_class_num > 99
		|| config.minimum_class_num > config.maximum_class_num) {
		errors.append(QStringLiteral("班级范围必须处于1～99，且起始值不能大于结束值。"));
	}
	if (config.minimum_grade < 2010 || config.maximum_grade > 2099
		|| config.minimum_grade > config.maximum_grade) {
		errors.append(QStringLiteral("年级范围必须处于2010～2099，且起始值不能大于结束值。"));
	}
	if (errors.isEmpty()) {
		const plan generated_plan = create_plan(config, current_school);
		if (!generated_plan.error_message.isEmpty()) {
			errors.append(generated_plan.error_message);
		}
	}
	return errors;
}

sampledatagenerator::plan sampledatagenerator::create_plan(const sampledataconfig& config, const school& current_school)//创建完整随机计划
{
	plan result;
	QRandomGenerator random(config.random_seed);

	QSet<int> used_building_ids;
	if (config.mode == sampledatamode::append)
		for (int building_id : current_school.get_all_building_ids()) used_building_ids.insert(building_id);
	QVector<int> available_building_ids;
	for (int building_id = 1; building_id <= 99; ++building_id) {
		if (!used_building_ids.contains(building_id)) {
			available_building_ids.append(building_id);
		}
	}

	QVector<int> building_genders;
	building_genders.fill(1, config.male_building_count);
	building_genders += QVector<int>(config.female_building_count, 2);
	building_genders += QVector<int>(config.mixed_building_count, 3);
	shuffle_items(building_genders, random);
	if (building_genders.size() > available_building_ids.size()) {
		result.error_message = QStringLiteral("剩余楼号不足，无法形成样例数据计划。");
		return result;
	}

	const int dorms_per_building = config.floors_per_building * config.dorms_per_floor;
	for (int i = 0; i < building_genders.size(); ++i) {
		const plannedbuilding building_plan = {
			available_building_ids.at(i), building_genders.at(i), config.floors_per_building
		};
		result.buildings.append(building_plan);

		QVector<int> capacities;
		capacities.fill(4, config.four_bed_dorm_count);
		capacities += QVector<int>(config.six_bed_dorm_count, 6);
		shuffle_items(capacities, random);

		QVector<int> locks(dorms_per_building, 0);
		if (building_plan.gender == 3) {
			locks.clear();
			locks.fill(1, config.mixed_male_dorm_count);
			locks += QVector<int>(config.mixed_female_dorm_count, 2);
			locks += QVector<int>(config.mixed_unlocked_dorm_count, 0);
			shuffle_items(locks, random);
		}

		int dorm_index = 0;
		for (int floor = 1; floor <= config.floors_per_building; ++floor) {
			for (int room = 1; room <= config.dorms_per_floor; ++room) {
				result.dorms.append({
					building_plan.id,
					floor * 100 + room,
					capacities.at(dorm_index),
					locks.at(dorm_index)
				});
				++dorm_index;
			}
		}
	}

	QHash<int, QSet<int>> used_sequences_by_grade;
	QSet<int> used_student_ids;
	if (config.mode == sampledatamode::append) for (int student_id : current_school.get_all_student_ids()) {
		used_student_ids.insert(student_id);
		const student* existing_student = current_school.get_student(student_id);
		if (existing_student == nullptr || existing_student->get_id() != student_id
			|| !check::is_student_id_consistent(existing_student->get_id(), existing_student->get_grade(), existing_student->get_class_num())) {
			result.error_message = QStringLiteral("现有学生中存在不符合统一学号规则的异常记录，请先核查后再追加样例数据。");
			return result;
		}
		const int sequence = check::student_id_sequence(existing_student->get_id());
		if (check::is_valid_student_sequence(sequence)) {
			used_sequences_by_grade[existing_student->get_grade()].insert(sequence);
		}
	}
	QVector<int> male_student_indexes;
	QVector<int> female_student_indexes;
	const int grade_count = config.maximum_grade - config.minimum_grade + 1;
	const int class_count = config.maximum_class_num - config.minimum_class_num + 1;
	const int profile_count = grade_count * class_count;
	const auto append_students = [&](int count, int gender, QVector<int>& indexes) -> bool {
		for (int i = 0; i < count; ++i) {
			int student_id = 0;
			int grade = 0;
			int class_num = 0;
			const int first_profile = random.bounded(profile_count);
			for (int profile_offset = 0; profile_offset < profile_count && student_id == 0; ++profile_offset) {
				const int profile_index = (first_profile + profile_offset) % profile_count;
				grade = config.minimum_grade + profile_index / class_count;
				class_num = config.minimum_class_num + profile_index % class_count;
				const QSet<int>& used_sequences = used_sequences_by_grade[grade];
				for (int sequence = 1; sequence <= 9999; ++sequence) {
					const int candidate = check::make_student_id(grade, class_num, sequence);
					if (!used_sequences.contains(sequence) && !used_student_ids.contains(candidate)) {
						student_id = candidate;
						break;
					}
				}
			}
			if (student_id == 0) {
				return false;
			}
			used_sequences_by_grade[grade].insert(check::student_id_sequence(student_id));
			used_student_ids.insert(student_id);
			plannedstudent student_plan;
			student_plan.id = student_id;
			student_plan.name = random_student_name(random);
			student_plan.gender = gender;
			student_plan.class_num = class_num;
			student_plan.grade = grade;
			indexes.append(result.students.size());
			result.students.append(student_plan);
		}
		return true;
	};
	if (!append_students(config.male_student_count, 1, male_student_indexes)
		|| !append_students(config.female_student_count, 2, female_student_indexes)) {
		result.error_message = QStringLiteral("所选年级与班级范围内没有足够的可用学号序列。");
		return result;
	}
	shuffle_items(male_student_indexes, random);
	shuffle_items(female_student_indexes, random);

	QHash<int, int> building_genders_by_id;
	for (const plannedbuilding& building_plan : result.buildings) {
		building_genders_by_id.insert(building_plan.id, building_plan.gender);
	}
	QVector<int> male_dedicated_indexes;
	QVector<int> female_dedicated_indexes;
	QVector<int> male_locked_indexes;
	QVector<int> female_locked_indexes;
	QVector<int> shared_dorm_indexes;
	for (int i = 0; i < result.dorms.size(); ++i) {
		const planneddorm& dorm_plan = result.dorms.at(i);
		const int building_gender = building_genders_by_id.value(dorm_plan.building_id);
		if (building_gender == 1) {
			male_dedicated_indexes.append(i);
		} else if (building_gender == 2) {
			female_dedicated_indexes.append(i);
		} else if (building_gender == 3 && dorm_plan.gender_lock == 1) {
			male_locked_indexes.append(i);
		} else if (building_gender == 3 && dorm_plan.gender_lock == 2) {
			female_locked_indexes.append(i);
		} else if (building_gender == 3 && dorm_plan.gender_lock == 0) {
			shared_dorm_indexes.append(i);
		}
	}
	shuffle_items(male_dedicated_indexes, random);
	shuffle_items(female_dedicated_indexes, random);
	shuffle_items(male_locked_indexes, random);
	shuffle_items(female_locked_indexes, random);
	shuffle_items(shared_dorm_indexes, random);
	QVector<int> male_dorm_indexes = male_dedicated_indexes + male_locked_indexes;
	QVector<int> female_dorm_indexes = female_dedicated_indexes + female_locked_indexes;
	QVector<int> male_coverage_dorms;
	QVector<int> female_coverage_dorms;

	if (config.minimum_unlocked_empty_dorm_count > shared_dorm_indexes.size()) {
		result.error_message = QStringLiteral("可保留的宿舍性别锁未设置宿舍不足，无法形成入住计划。");
		return result;
	}
	QVector<int> unreserved_shared = shared_dorm_indexes.mid(config.minimum_unlocked_empty_dorm_count);
	if (config.male_assigned_count > 0 && !unreserved_shared.isEmpty()) {
		const int male_coverage_dorm = unreserved_shared.takeFirst();
		male_dorm_indexes.append(male_coverage_dorm);
		male_coverage_dorms.append(male_coverage_dorm);
	}
	if (config.female_assigned_count > 0 && !unreserved_shared.isEmpty()) {
		const int female_coverage_dorm = unreserved_shared.takeFirst();
		female_dorm_indexes.append(female_coverage_dorm);
		female_coverage_dorms.append(female_coverage_dorm);
	}
	if (!male_dedicated_indexes.isEmpty()) {
		male_coverage_dorms.append(male_dedicated_indexes.first());
	}
	if (!male_locked_indexes.isEmpty()) {
		male_coverage_dorms.append(male_locked_indexes.first());
	}
	if (!female_dedicated_indexes.isEmpty()) {
		female_coverage_dorms.append(female_dedicated_indexes.first());
	}
	if (!female_locked_indexes.isEmpty()) {
		female_coverage_dorms.append(female_locked_indexes.first());
	}

	const int male_deficit = qMax(0, config.male_assigned_count - dorm_capacity(result.dorms, male_dorm_indexes));
	const int female_deficit = qMax(0, config.female_assigned_count - dorm_capacity(result.dorms, female_dorm_indexes));
	bool shared_distribution_found = false;
	const QVector<int> male_extra_shared = choose_male_shared_dorms(result.dorms, unreserved_shared,
		male_deficit, female_deficit, shared_distribution_found);
	if (!shared_distribution_found) {
		result.error_message = QStringLiteral("当前房型与性别分布无法容纳指定的男女入住人数。");
		return result;
	}
	QSet<int> male_extra_set;
	for (int dorm_index : male_extra_shared) {
		male_extra_set.insert(dorm_index);
	}
	for (int dorm_index : unreserved_shared) {
		if (male_extra_set.contains(dorm_index)) {
			male_dorm_indexes.append(dorm_index);
		} else {
			female_dorm_indexes.append(dorm_index);
		}
	}
	if (dorm_capacity(result.dorms, male_dorm_indexes) < config.male_assigned_count
		|| dorm_capacity(result.dorms, female_dorm_indexes) < config.female_assigned_count) {
		result.error_message = QStringLiteral("当前宿舍容量不足，无法满足指定的男女入住人数。");
		return result;
	}

	QSet<int> occupied_dorm_indexes;
	const auto assign_students = [&](const QVector<int>& student_indexes, int assigned_count,
		QVector<int> dorm_indexes, const QVector<int>& coverage_dorms) {
		QVector<int> unique_dorms;
		for (int dorm_index : dorm_indexes) if (!unique_dorms.contains(dorm_index)) unique_dorms.append(dorm_index);
		QVector<int> planned_slots;
		QHash<int, int> used_beds;
		if (unique_dorms.size() >= 2 && assigned_count >= result.dorms.at(unique_dorms[0]).max_num + 1) {
			for (int i = 0; i < result.dorms.at(unique_dorms[0]).max_num; ++i) planned_slots.append(unique_dorms[0]);
			planned_slots.append(unique_dorms[1]);
			used_beds[unique_dorms[0]] = result.dorms.at(unique_dorms[0]).max_num;
			used_beds[unique_dorms[1]] = 1;
		}
		for (int dorm_index : coverage_dorms) {
			if (planned_slots.size() >= assigned_count) break;
			if (used_beds.value(dorm_index) == 0) {
				planned_slots.append(dorm_index);
				used_beds[dorm_index] = 1;
			}
		}
		QVector<int> remaining_slots;
		for (int dorm_index : unique_dorms)
			for (int bed = used_beds.value(dorm_index); bed < result.dorms.at(dorm_index).max_num; ++bed)
				remaining_slots.append(dorm_index);
		shuffle_items(remaining_slots, random);
		while (planned_slots.size() < assigned_count) planned_slots.append(remaining_slots.takeLast());
		for (int i = 0; i < assigned_count; ++i) {
			const int dorm_index = planned_slots.at(i);
			plannedstudent& student_plan = result.students[student_indexes.at(i)];
			student_plan.building_id = result.dorms.at(dorm_index).building_id;
			student_plan.dorm_id = result.dorms.at(dorm_index).dorm_id;
			occupied_dorm_indexes.insert(dorm_index);
		}
	};
	assign_students(male_student_indexes, config.male_assigned_count, male_dorm_indexes, male_coverage_dorms);
	assign_students(female_student_indexes, config.female_assigned_count, female_dorm_indexes, female_coverage_dorms);
	QHash<QString, int> next_bed_by_dorm;
	for (plannedstudent& student_plan : result.students) {
		if (student_plan.building_id == 0) continue;
		const QString key = QStringLiteral("%1/%2").arg(student_plan.building_id).arg(student_plan.dorm_id);
		student_plan.bed_id = next_bed_by_dorm.value(key, 0) + 1;
		next_bed_by_dorm[key] = student_plan.bed_id;
	}

	for (int dorm_index : shared_dorm_indexes) {
		if (!occupied_dorm_indexes.contains(dorm_index)) {
			++result.remaining_unlocked_dorm_count;
		}
	}
	if (result.remaining_unlocked_dorm_count < config.minimum_unlocked_empty_dorm_count) {
		result.error_message = QStringLiteral("计划未能保留指定数量的宿舍性别锁未设置空宿舍。");
	}
	return result;
}

sampledataplan sampledatagenerator::create_replace_plan(const sampledataconfig& config, const school& current_school, QString* error_message)//创建清空后生成固定计划
{
	sampledataconfig replace_config = config;
	replace_config.mode = sampledatamode::replace_reserved;
	const QStringList errors = validate_config(replace_config, current_school);
	if (!errors.isEmpty()) {
		if (error_message != nullptr) *error_message = errors.join(QLatin1Char('\n'));
		return {};
	}
	const plan generated = create_plan(replace_config, current_school);
	if (error_message != nullptr) *error_message = generated.error_message;
	if (!generated.error_message.isEmpty()) return {};
	sampledataplan result;
	result.buildings = generated.buildings;
	result.dorms = generated.dorms;
	result.students = generated.students;
	return result;
}

sampledataresult sampledatagenerator::generate(const sampledataconfig& config, school& current_school)//执行追加生成并在失败时补偿
{
	sampledataresult result;
	result.random_seed = config.random_seed;
	const QStringList validation_errors = validate_config(config, current_school);
	if (!validation_errors.isEmpty()) {
		result.error_message = validation_errors.join(QLatin1Char('\n'));
		return result;
	}
	const plan generated_plan = create_plan(config, current_school);
	if (!generated_plan.error_message.isEmpty()) {
		result.error_message = generated_plan.error_message;
		return result;
	}

	QVector<int> created_building_ids;
	QVector<QPair<int, int>> created_dorm_keys;
	QVector<int> created_student_ids;
	const auto fail = [&](const QString& message) {
		result.error_message = message;
		rollback_created_data(current_school, created_student_ids, created_dorm_keys, created_building_ids, result);
		return result;
	};

	for (const plannedbuilding& building_plan : generated_plan.buildings) {
		const int add_result = current_school.add_building(building_plan.id, building_plan.gender, building_plan.max_floor);
		if (add_result != 1) {
			return fail(QStringLiteral("添加%1号样例宿舍楼失败，返回码：%2。").arg(building_plan.id).arg(add_result));
		}
		created_building_ids.append(building_plan.id);
		++result.added_building_count;
	}

	for (const planneddorm& dorm_plan : generated_plan.dorms) {
		dorm dorm_to_add;
		const bool initialized = dorm_to_add.set_building_id(dorm_plan.building_id)
			&& dorm_to_add.set_id(dorm_plan.dorm_id)
			&& dorm_to_add.set_max_num(dorm_plan.max_num);
		if (!initialized || !current_school.add_dorm(dorm_to_add)) {
			return fail(QStringLiteral("添加%1号楼%2宿舍失败。").arg(dorm_plan.building_id).arg(dorm_plan.dorm_id));
		}
		created_dorm_keys.append(qMakePair(dorm_plan.building_id, dorm_plan.dorm_id));
		++result.added_dorm_count;
		if (dorm_plan.gender_lock != 0) {
			const int lock_result = current_school.set_dorm_gender(
				dorm_plan.building_id, dorm_plan.dorm_id, dorm_plan.gender_lock);
			if (lock_result != 1) {
				return fail(QStringLiteral("设置%1号楼%2宿舍性别锁失败，返回码：%3。")
					.arg(dorm_plan.building_id).arg(dorm_plan.dorm_id).arg(lock_result));
			}
		}
	}

	for (const plannedstudent& student_plan : generated_plan.students) {
		student student_to_add;
		const bool initialized = student_to_add.set_id(student_plan.id)
			&& student_to_add.set_name(student_plan.name)
			&& student_to_add.set_gender(student_plan.gender)
			&& student_to_add.set_class_num(student_plan.class_num)
			&& student_to_add.set_grade(student_plan.grade);
		if (!initialized || !current_school.add_student(student_to_add)) {
			return fail(QStringLiteral("添加学号为%1的样例学生失败。").arg(student_plan.id));
		}
		created_student_ids.append(student_plan.id);
		++result.added_student_count;
	}

	for (const plannedstudent& student_plan : generated_plan.students) {
		if (student_plan.building_id == 0 || student_plan.dorm_id == 0) {
			continue;
		}
		const int assign_result = current_school.assign_student_to_dorm(
			student_plan.building_id, student_plan.dorm_id, student_plan.id);
		if (assign_result <= 0) {
			return fail(QStringLiteral("安排学号为%1的样例学生入住%2号楼%3宿舍失败，返回码：%4。")
				.arg(student_plan.id).arg(student_plan.building_id).arg(student_plan.dorm_id).arg(assign_result));
		}
		++result.assigned_student_count;
	}

	result.success = true;
	result.remaining_unlocked_dorm_count = generated_plan.remaining_unlocked_dorm_count;
	return result;
}
