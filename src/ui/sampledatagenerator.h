#ifndef SAMPLEDATAGENERATOR_H
#define SAMPLEDATAGENERATOR_H

#include <QString>
#include <QStringList>
#include <QtGlobal>
#include "core/sampledataplan.h"

class school;

enum class sampledatamode
{
	append,
	replace_reserved//仅预留接口；须等前端 Phase 2 收尾并具备原数据快照恢复后再实现
};

//样例数据生成参数。数量均描述本次新增数据，不修改既有对象。
struct sampledataconfig
{
	sampledatamode mode = sampledatamode::append;
	int male_building_count = 2;
	int female_building_count = 2;
	int mixed_building_count = 1;
	int floors_per_building = 3;
	int dorms_per_floor = 4;
	int four_bed_dorm_count = 8;
	int six_bed_dorm_count = 4;
	int mixed_male_dorm_count = 3;
	int mixed_female_dorm_count = 3;
	int mixed_unlocked_dorm_count = 6;
	int minimum_unlocked_empty_dorm_count = 2;
	int male_student_count = 56;
	int female_student_count = 56;
	int male_assigned_count = 40;
	int female_assigned_count = 40;
	int minimum_class_num = 1;
	int maximum_class_num = 6;
	int minimum_grade = 2024;
	int maximum_grade = 2026;
	quint32 random_seed = 0;
};

//根据参数直接计算的规模预览，不代表执行结果。
struct sampledatapreview
{
	int building_count = 0;
	int dorm_count = 0;
	int bed_count = 0;
	int student_count = 0;
	int assigned_student_count = 0;
	int unassigned_student_count = 0;
	int reserved_unlocked_dorm_count = 0;
};

//生成执行结果。失败时同时报告补偿恢复是否完整。
struct sampledataresult
{
	bool success = false;
	bool rollback_complete = true;
	QString error_message;
	quint32 random_seed = 0;
	int added_building_count = 0;
	int added_dorm_count = 0;
	int added_student_count = 0;
	int assigned_student_count = 0;
	int remaining_unlocked_dorm_count = 0;
	int residual_building_count = 0;
	int residual_dorm_count = 0;
	int residual_student_count = 0;
};

//前端样例数据生成器。只通过 school 公开入口写入，不直接访问 manager。
class sampledatagenerator
{
public:
	static constexpr int maximum_building_count = 20;
	static constexpr int maximum_dorm_count = 1000;
	static constexpr int maximum_student_count = 2000;

	static sampledatapreview preview(const sampledataconfig& config);//计算预计新增规模
	static QStringList validate_config(const sampledataconfig& config, const school& current_school);//返回全部参数问题，空表表示可继续规划
	static sampledataresult generate(const sampledataconfig& config, school& current_school);//生成并追加样例数据，失败时补偿本次写入
	static sampledataplan create_replace_plan(const sampledataconfig& config, const school& current_school, QString* error_message = nullptr);//为清空后生成建立固定计划

private:
	struct plan;
	static plan create_plan(const sampledataconfig& config, const school& current_school);//生成不修改业务数据的完整执行计划
};

#endif // SAMPLEDATAGENERATOR_H
