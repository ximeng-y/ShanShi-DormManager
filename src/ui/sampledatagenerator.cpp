#include "sampledatagenerator.h"

#include "core/school.h"

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
	return errors;
}
