#include "sampledatagenerator.h"

#include "core/school.h"

#include <QHash>
#include <QPair>
#include <QRandomGenerator>
#include <QSet>
#include <QVector>

namespace {
struct plannedbuilding
{
	int id = 0;
	int gender = 0;
	int max_floor = 0;
};

struct planneddorm
{
	int building_id = 0;
	int dorm_id = 0;
	int max_num = 0;
	int gender_lock = 0;
};

struct plannedstudent
{
	int id = 0;
	QString name;
	int gender = 0;
	int class_num = 0;
	int grade = 0;
	int building_id = 0;
	int dorm_id = 0;
};

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
	QString result = surnames.at(random.bounded(surnames.size()));
	result += given_characters.at(random.bounded(given_characters.size()));
	if (random.bounded(100) < 65) {
		result += given_characters.at(random.bounded(given_characters.size()));
	}
	return result;
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
	result.building_count = config.male_building_count
		+ config.female_building_count
		+ config.mixed_building_count;
	const int dorms_per_building = config.floors_per_building * config.dorms_per_floor;
	result.dorm_count = result.building_count * dorms_per_building;
	const int beds_per_building = config.four_bed_dorm_count * 4
		+ config.six_bed_dorm_count * 6;
	result.bed_count = result.building_count * beds_per_building;
	result.student_count = config.male_student_count + config.female_student_count;
	result.assigned_student_count = config.male_assigned_count + config.female_assigned_count;
	result.unassigned_student_count = result.student_count - result.assigned_student_count;
	result.reserved_unlocked_dorm_count = config.minimum_unlocked_empty_dorm_count;
	return result;
}

QStringList sampledatagenerator::validate_config(const sampledataconfig& config, const school& current_school)//校验生成参数
{
	QStringList errors;
	if (config.mode != sampledatamode::append) {
		errors.append(QStringLiteral("清空后生成将在前端 Phase 2 收尾后实现，当前只能追加样例数据。"));
	}

	const sampledatapreview scale = preview(config);
	if (config.male_building_count < 0 || config.female_building_count < 0 || config.mixed_building_count < 0
		|| scale.building_count <= 0) {
		errors.append(QStringLiteral("至少需要生成一栋宿舍楼，且各类楼栋数量不能为负数。"));
	}
	if (scale.building_count > maximum_building_count) {
		errors.append(QStringLiteral("单次生成的宿舍楼不能超过%1栋。").arg(maximum_building_count));
	}
	if (scale.building_count > 99 - current_school.get_building_count()) {
		errors.append(QStringLiteral("剩余楼号不足，无法追加指定数量的宿舍楼。"));
	}
	if (config.floors_per_building < 1 || config.floors_per_building > 99
		|| config.dorms_per_floor < 1 || config.dorms_per_floor > 99) {
		errors.append(QStringLiteral("每栋楼层数和每层宿舍数必须在1～99之间。"));
	}

	const int dorms_per_building = config.floors_per_building * config.dorms_per_floor;
	if (config.four_bed_dorm_count < 0 || config.six_bed_dorm_count < 0
		|| config.four_bed_dorm_count + config.six_bed_dorm_count != dorms_per_building) {
		errors.append(QStringLiteral("每栋4人间与6人间数量之和必须等于每栋宿舍总数。"));
	}
	if (scale.dorm_count > maximum_dorm_count) {
		errors.append(QStringLiteral("单次生成的宿舍不能超过%1间。").arg(maximum_dorm_count));
	}

	if (config.mixed_building_count > 0) {
		if (config.mixed_male_dorm_count < 0 || config.mixed_female_dorm_count < 0
			|| config.mixed_unlocked_dorm_count < 0
			|| config.mixed_male_dorm_count + config.mixed_female_dorm_count
				+ config.mixed_unlocked_dorm_count != dorms_per_building) {
			errors.append(QStringLiteral("混合楼男舍、女舍与未锁定宿舍数量之和必须等于每栋宿舍总数。"));
		}
		const int total_unlocked = config.mixed_building_count * config.mixed_unlocked_dorm_count;
		if (config.minimum_unlocked_empty_dorm_count < 0
			|| config.minimum_unlocked_empty_dorm_count > total_unlocked) {
			errors.append(QStringLiteral("保留的未锁定空宿舍数量不能超过混合楼未锁定宿舍总数。"));
		}
	} else if (config.minimum_unlocked_empty_dorm_count != 0) {
		errors.append(QStringLiteral("未生成混合楼时，保留的未锁定空宿舍数量必须为0。"));
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
	if (config.minimum_grade < 2000 || config.maximum_grade > 2999
		|| config.minimum_grade > config.maximum_grade) {
		errors.append(QStringLiteral("年级范围必须处于2000～2999，且起始值不能大于结束值。"));
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
	for (int building_id : current_school.get_all_building_ids()) {
		used_building_ids.insert(building_id);
	}
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

	QSet<int> used_student_ids;
	for (int student_id : current_school.get_all_student_ids()) {
		used_student_ids.insert(student_id);
	}
	QVector<int> male_student_indexes;
	QVector<int> female_student_indexes;
	const auto append_students = [&](int count, int gender, QVector<int>& indexes) {
		for (int i = 0; i < count; ++i) {
			int student_id = 0;
			do {
				student_id = 10000000 + random.bounded(90000000);
			} while (used_student_ids.contains(student_id));
			used_student_ids.insert(student_id);
			plannedstudent student_plan;
			student_plan.id = student_id;
			student_plan.name = random_student_name(random);
			student_plan.gender = gender;
			student_plan.class_num = config.minimum_class_num
				+ random.bounded(config.maximum_class_num - config.minimum_class_num + 1);
			student_plan.grade = config.minimum_grade
				+ random.bounded(config.maximum_grade - config.minimum_grade + 1);
			indexes.append(result.students.size());
			result.students.append(student_plan);
		}
	};
	append_students(config.male_student_count, 1, male_student_indexes);
	append_students(config.female_student_count, 2, female_student_indexes);
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
		result.error_message = QStringLiteral("可保留的未锁定宿舍不足，无法形成入住计划。");
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
		QVector<int> slots;
		const int coverage_count = qMin(assigned_count, coverage_dorms.size());
		QHash<int, int> covered_beds;
		for (int i = 0; i < coverage_count; ++i) {
			const int dorm_index = coverage_dorms.at(i);
			plannedstudent& student_plan = result.students[student_indexes.at(i)];
			student_plan.building_id = result.dorms.at(dorm_index).building_id;
			student_plan.dorm_id = result.dorms.at(dorm_index).dorm_id;
			occupied_dorm_indexes.insert(dorm_index);
			covered_beds[dorm_index] = covered_beds.value(dorm_index) + 1;
		}
		for (int dorm_index : dorm_indexes) {
			const int available_beds = result.dorms.at(dorm_index).max_num - covered_beds.value(dorm_index);
			for (int bed = 0; bed < available_beds; ++bed) {
				slots.append(dorm_index);
			}
		}
		shuffle_items(slots, random);
		for (int i = coverage_count; i < assigned_count; ++i) {
			const int dorm_index = slots.at(i - coverage_count);
			plannedstudent& student_plan = result.students[student_indexes.at(i)];
			student_plan.building_id = result.dorms.at(dorm_index).building_id;
			student_plan.dorm_id = result.dorms.at(dorm_index).dorm_id;
			occupied_dorm_indexes.insert(dorm_index);
		}
	};
	assign_students(male_student_indexes, config.male_assigned_count, male_dorm_indexes, male_coverage_dorms);
	assign_students(female_student_indexes, config.female_assigned_count, female_dorm_indexes, female_coverage_dorms);

	for (int dorm_index : shared_dorm_indexes) {
		if (!occupied_dorm_indexes.contains(dorm_index)) {
			++result.remaining_unlocked_dorm_count;
		}
	}
	if (result.remaining_unlocked_dorm_count < config.minimum_unlocked_empty_dorm_count) {
		result.error_message = QStringLiteral("计划未能保留指定数量的未锁定空宿舍。");
	}
	return result;
}
