#include "schoolstorage.h"

#include "system/check.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
void set_error(QString* error, const QString& message)
{
	if (error != nullptr)
		*error = message;
}
}

QString schoolstorage::executable_data_directory()
{
	return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("userdata"));
}

QString schoolstorage::fallback_data_directory()
{
	return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(QStringLiteral("userdata"));
}

bool schoolstorage::directory_is_writable(const QString& directory, QString* error)
{
	QDir target;
	if (!target.mkpath(directory))
	{
		set_error(error, QStringLiteral("无法创建数据目录：%1").arg(directory));
		return false;
	}
	const QString probe_path = QDir(directory).filePath(QStringLiteral(".write-probe"));
	QSaveFile probe(probe_path);
	if (!probe.open(QIODevice::WriteOnly) || probe.write("ok") != 2 || !probe.commit())
	{
		set_error(error, QStringLiteral("数据目录不可写：%1").arg(directory));
		probe.cancelWriting();
		return false;
	}
	QFile::remove(probe_path);
	return true;
}

void schoolstorage::set_data_directory(const QString& directory)
{
	current_directory = QDir::cleanPath(directory);
}

QString schoolstorage::data_directory() const
{
	return current_directory;
}

QString schoolstorage::primary_file_path() const
{
	return QDir(current_directory).filePath(QStringLiteral("school-data.json"));
}

QString schoolstorage::backup_file_path() const
{
	return QDir(current_directory).filePath(QStringLiteral("school-data.backup.json"));
}

storage_load_status schoolstorage::load_primary(schoolsnapshot& snapshot, QString* error) const
{
	return load_file(primary_file_path(), snapshot, error);
}

storage_load_status schoolstorage::load_backup(schoolsnapshot& snapshot, QString* error) const
{
	return load_file(backup_file_path(), snapshot, error);
}

storage_load_status schoolstorage::load_file(const QString& path, schoolsnapshot& snapshot, QString* error) const
{
	QFile file(path);
	if (!file.exists())
		return storage_load_status::not_found;
	if (!file.open(QIODevice::ReadOnly))
	{
		set_error(error, QStringLiteral("无法读取数据文件：%1").arg(path));
		return storage_load_status::io_error;
	}
	constexpr qint64 maximum_json_bytes = 32 * 1024 * 1024;
	if (file.size() <= 0 || file.size() > maximum_json_bytes)
	{
		set_error(error, QStringLiteral("数据文件为空或超过32 MiB安全上限：%1").arg(path));
		return storage_load_status::invalid;
	}
	const QByteArray data = file.readAll();
	if (data.size() != file.size())
	{
		set_error(error, QStringLiteral("数据文件读取不完整：%1").arg(path));
		return storage_load_status::io_error;
	}
	bool newer_version = false;
	if (!decode_snapshot(data, snapshot, error, &newer_version))
		return newer_version ? storage_load_status::newer_version : storage_load_status::invalid;
	return storage_load_status::loaded;
}

bool schoolstorage::create_initial_file(const schoolsnapshot& empty_snapshot, QString* error)
{
	if (!QDir().mkpath(current_directory))
	{
		set_error(error, QStringLiteral("无法创建数据目录：%1").arg(current_directory));
		return false;
	}
	if (QFileInfo::exists(primary_file_path()))
	{
		set_error(error, QStringLiteral("正式数据文件已经存在，拒绝覆盖。"));
		return false;
	}
	const QByteArray data = encode_snapshot(empty_snapshot, error);
	return !data.isEmpty() && write_atomic(primary_file_path(), data, error);
}

bool schoolstorage::save_snapshot(const schoolsnapshot& snapshot, QString* error)
{
	const QByteArray new_data = encode_snapshot(snapshot, error);
	if (new_data.isEmpty())
		return false;
	const QString primary_path = primary_file_path();
	if (QFileInfo::exists(primary_path))
	{
		QFile primary(primary_path);
		if (!primary.open(QIODevice::ReadOnly))
		{
			set_error(error, QStringLiteral("无法读取当前正式数据，未执行覆盖。"));
			return false;
		}
		const QByteArray previous_data = primary.readAll();
		schoolsnapshot previous_snapshot;
		QString validation_error;
		bool newer_version = false;
		if (!decode_snapshot(previous_data, previous_snapshot, &validation_error, &newer_version))
		{
			set_error(error, newer_version ? QStringLiteral("当前正式数据来自更高版本，拒绝覆盖。")
				: QStringLiteral("当前正式数据无效，拒绝覆盖：%1").arg(validation_error));
			return false;
		}
		if (!write_atomic(backup_file_path(), previous_data, error))
			return false;
	}
	return write_atomic(primary_path, new_data, error);
}

bool schoolstorage::rebuild_primary(const schoolsnapshot& snapshot, QString* error)
{
	const QByteArray data = encode_snapshot(snapshot, error);
	return !data.isEmpty() && write_atomic(primary_file_path(), data, error);
}

bool schoolstorage::archive_invalid_file(const QString& source_path, QString* archived_path, QString* error) const
{
	if (!QFileInfo::exists(source_path))
		return true;
	const QString recovery_directory = QDir(current_directory).filePath(QStringLiteral("recovery"));
	if (!QDir().mkpath(recovery_directory))
	{
		set_error(error, QStringLiteral("无法创建损坏文件保留目录。"));
		return false;
	}
	const QFileInfo source_info(source_path);
	const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
	const QString destination = QDir(recovery_directory).filePath(
		QStringLiteral("%1.corrupt-%2.%3").arg(source_info.completeBaseName(), timestamp, source_info.suffix()));
	QFile source(source_path);
	if (!source.open(QIODevice::ReadOnly))
	{
		set_error(error, QStringLiteral("无法读取待保留的损坏文件。"));
		return false;
	}
	if (!write_atomic(destination, source.readAll(), error))
		return false;
	if (archived_path != nullptr)
		*archived_path = destination;
	return true;
}

bool schoolstorage::write_atomic(const QString& path, const QByteArray& data, QString* error)
{
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly))
	{
		set_error(error, QStringLiteral("无法打开文件进行写入：%1").arg(path));
		return false;
	}
	if (file.write(data) != data.size())
	{
		file.cancelWriting();
		set_error(error, QStringLiteral("文件写入不完整：%1").arg(path));
		return false;
	}
	if (!file.commit())
	{
		set_error(error, QStringLiteral("无法原子提交文件：%1").arg(path));
		return false;
	}
	return true;
}

namespace {
bool read_int(const QJsonObject& object, const QString& key, int& value)
{
	const QJsonValue json_value = object.value(key);
	if (!json_value.isDouble())
		return false;
	const double number = json_value.toDouble();
	if (!std::isfinite(number) || std::floor(number) != number
		|| number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
		return false;
	value = static_cast<int>(number);
	return true;
}

QString dorm_key(int building_id, int dorm_id)
{
	return QStringLiteral("%1/%2").arg(building_id).arg(dorm_id);
}
}

QByteArray schoolstorage::encode_snapshot(const schoolsnapshot& snapshot, QString* error)
{
	if (!validate_snapshot(snapshot, error))
		return {};
	QJsonArray buildings;
	for (const building_snapshot& item : snapshot.buildings)
		buildings.append(QJsonObject{{QStringLiteral("id"), item.id}, {QStringLiteral("maxFloor"), item.max_floor},
			{QStringLiteral("gender"), item.gender}});
	QJsonArray dorms;
	for (const dorm_snapshot& item : snapshot.dorms)
	{
		QJsonArray beds;
		for (int student_id : item.beds)
			beds.append(student_id);
		dorms.append(QJsonObject{{QStringLiteral("buildingId"), item.building_id}, {QStringLiteral("dormId"), item.dorm_id},
			{QStringLiteral("maxBeds"), item.max_beds}, {QStringLiteral("genderLock"), item.gender_lock},
			{QStringLiteral("beds"), beds}});
	}
	QJsonArray students;
	for (const student_snapshot& item : snapshot.students)
		students.append(QJsonObject{{QStringLiteral("id"), item.id}, {QStringLiteral("name"), item.name},
			{QStringLiteral("gender"), item.gender}, {QStringLiteral("grade"), item.grade},
			{QStringLiteral("classNumber"), item.class_number}, {QStringLiteral("buildingId"), item.building_id},
			{QStringLiteral("dormId"), item.dorm_id}, {QStringLiteral("floor"), item.floor},
			{QStringLiteral("bedId"), item.bed_id}});
	const QJsonObject root{{QStringLiteral("format"), snapshot.format}, {QStringLiteral("version"), snapshot.version},
		{QStringLiteral("savedAt"), snapshot.saved_at.isValid() ? snapshot.saved_at.toString(Qt::ISODateWithMs) : QString()},
		{QStringLiteral("buildings"), buildings}, {QStringLiteral("dorms"), dorms}, {QStringLiteral("students"), students}};
	const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
	schoolsnapshot reparsed;
	QString parse_error;
	if (!decode_snapshot(data, reparsed, &parse_error) || !reparsed.data_equals(snapshot))
	{
		set_error(error, QStringLiteral("持久化JSON自检失败：%1").arg(parse_error));
		return {};
	}
	return data;
}

bool schoolstorage::decode_snapshot(const QByteArray& data, schoolsnapshot& snapshot, QString* error, bool* newer_version)
{
	constexpr qsizetype maximum_json_bytes = 32 * 1024 * 1024;
	if (newer_version != nullptr)
		*newer_version = false;
	if (data.isEmpty() || data.size() > maximum_json_bytes)
	{
		set_error(error, QStringLiteral("数据文件为空或超过32 MiB安全上限。"));
		return false;
	}
	QJsonParseError parse_error;
	const QJsonDocument document = QJsonDocument::fromJson(data, &parse_error);
	if (parse_error.error != QJsonParseError::NoError || !document.isObject())
	{
		set_error(error, QStringLiteral("JSON语法无效：%1").arg(parse_error.errorString()));
		return false;
	}
	const QJsonObject root = document.object();
	if (!root.value(QStringLiteral("format")).isString() || !root.value(QStringLiteral("version")).isDouble())
	{
		set_error(error, QStringLiteral("数据文件缺少格式标识或版本号。"));
		return false;
	}
	schoolsnapshot parsed;
	parsed.format = root.value(QStringLiteral("format")).toString();
	if (!read_int(root, QStringLiteral("version"), parsed.version))
	{
		set_error(error, QStringLiteral("数据文件版本号无效。"));
		return false;
	}
	if (parsed.format != QStringLiteral("DormManagerData"))
	{
		set_error(error, QStringLiteral("数据文件格式标识不受支持。"));
		return false;
	}
	if (parsed.version > schoolsnapshot::current_version)
	{
		if (newer_version != nullptr)
			*newer_version = true;
		set_error(error, QStringLiteral("数据文件由更高版本程序创建。"));
		return false;
	}
	if (parsed.version != schoolsnapshot::current_version)
	{
		set_error(error, QStringLiteral("数据文件版本不受支持。"));
		return false;
	}
	if (!root.value(QStringLiteral("savedAt")).isString() || !root.value(QStringLiteral("buildings")).isArray()
		|| !root.value(QStringLiteral("dorms")).isArray() || !root.value(QStringLiteral("students")).isArray())
	{
		set_error(error, QStringLiteral("数据文件缺少必要字段或字段类型错误。"));
		return false;
	}
	parsed.saved_at = QDateTime::fromString(root.value(QStringLiteral("savedAt")).toString(), Qt::ISODateWithMs);
	for (const QJsonValue& value : root.value(QStringLiteral("buildings")).toArray())
	{
		if (!value.isObject()) { set_error(error, QStringLiteral("楼栋数据项不是对象。")); return false; }
		building_snapshot item;
		const QJsonObject object = value.toObject();
		if (!read_int(object, QStringLiteral("id"), item.id) || !read_int(object, QStringLiteral("maxFloor"), item.max_floor)
			|| !read_int(object, QStringLiteral("gender"), item.gender))
		{ set_error(error, QStringLiteral("楼栋字段类型错误。")); return false; }
		parsed.buildings.append(item);
	}
	for (const QJsonValue& value : root.value(QStringLiteral("dorms")).toArray())
	{
		if (!value.isObject()) { set_error(error, QStringLiteral("宿舍数据项不是对象。")); return false; }
		dorm_snapshot item;
		const QJsonObject object = value.toObject();
		if (!read_int(object, QStringLiteral("buildingId"), item.building_id)
			|| !read_int(object, QStringLiteral("dormId"), item.dorm_id)
			|| !read_int(object, QStringLiteral("maxBeds"), item.max_beds)
			|| !read_int(object, QStringLiteral("genderLock"), item.gender_lock)
			|| !object.value(QStringLiteral("beds")).isArray())
		{ set_error(error, QStringLiteral("宿舍字段类型错误。")); return false; }
		for (const QJsonValue& bed_value : object.value(QStringLiteral("beds")).toArray())
		{
			if (!bed_value.isDouble()) { set_error(error, QStringLiteral("床位学号字段类型错误。")); return false; }
			const double number = bed_value.toDouble();
			if (!std::isfinite(number) || std::floor(number) != number || number < 0 || number > 99999999)
			{ set_error(error, QStringLiteral("床位学号数值无效。")); return false; }
			item.beds.append(static_cast<int>(number));
		}
		parsed.dorms.append(item);
	}
	for (const QJsonValue& value : root.value(QStringLiteral("students")).toArray())
	{
		if (!value.isObject()) { set_error(error, QStringLiteral("学生数据项不是对象。")); return false; }
		student_snapshot item;
		const QJsonObject object = value.toObject();
		if (!object.value(QStringLiteral("name")).isString()
			|| !read_int(object, QStringLiteral("id"), item.id) || !read_int(object, QStringLiteral("gender"), item.gender)
			|| !read_int(object, QStringLiteral("grade"), item.grade) || !read_int(object, QStringLiteral("classNumber"), item.class_number)
			|| !read_int(object, QStringLiteral("buildingId"), item.building_id) || !read_int(object, QStringLiteral("dormId"), item.dorm_id)
			|| !read_int(object, QStringLiteral("floor"), item.floor) || !read_int(object, QStringLiteral("bedId"), item.bed_id))
		{ set_error(error, QStringLiteral("学生字段类型错误。")); return false; }
		item.name = object.value(QStringLiteral("name")).toString();
		parsed.students.append(item);
	}
	std::sort(parsed.buildings.begin(), parsed.buildings.end(), [](const building_snapshot& left, const building_snapshot& right) {
		return left.id < right.id;
	});
	std::sort(parsed.dorms.begin(), parsed.dorms.end(), [](const dorm_snapshot& left, const dorm_snapshot& right) {
		return left.building_id == right.building_id ? left.dorm_id < right.dorm_id : left.building_id < right.building_id;
	});
	std::sort(parsed.students.begin(), parsed.students.end(), [](const student_snapshot& left, const student_snapshot& right) {
		return left.id < right.id;
	});
	if (!validate_snapshot(parsed, error))
		return false;
	snapshot = parsed;
	return true;
}

bool schoolstorage::validate_snapshot(const schoolsnapshot& snapshot, QString* error)
{
	if (snapshot.format != QStringLiteral("DormManagerData"))
	{ set_error(error, QStringLiteral("数据文件格式标识不受支持。")); return false; }
	if (snapshot.version != schoolsnapshot::current_version)
	{ set_error(error, snapshot.version > schoolsnapshot::current_version
		? QStringLiteral("数据文件由更高版本程序创建。") : QStringLiteral("数据文件版本不受支持。")); return false; }
	if (!snapshot.saved_at.isValid())
	{ set_error(error, QStringLiteral("保存时间无效。")); return false; }
	QHash<int, building_snapshot> buildings;
	for (const building_snapshot& item : snapshot.buildings)
	{
		if (!check::is_valid_building_id(item.id) || !check::is_valid_max_floor(item.max_floor)
			|| !check::is_valid_building_gender(item.gender) || buildings.contains(item.id))
		{ set_error(error, QStringLiteral("楼栋字段非法或楼号重复。")); return false; }
		buildings.insert(item.id, item);
	}
	QHash<QString, dorm_snapshot> dorms;
	QSet<int> students_in_beds;
	QHash<int, QPair<QString, int>> bed_positions;
	for (const dorm_snapshot& item : snapshot.dorms)
	{
		const QString key = dorm_key(item.building_id, item.dorm_id);
		if (!buildings.contains(item.building_id) || !check::is_valid_dorm_id(item.dorm_id)
			|| item.dorm_id / 100 > buildings.value(item.building_id).max_floor || item.max_beds < 1
			|| item.gender_lock < 0 || item.gender_lock > 2 || item.beds.size() != item.max_beds || dorms.contains(key)
			|| (item.gender_lock != 0 && buildings.value(item.building_id).gender != 3
				&& buildings.value(item.building_id).gender != item.gender_lock))
		{ set_error(error, QStringLiteral("宿舍字段非法、引用错误或复合主键重复。")); return false; }
		for (int bed_index = 0; bed_index < item.beds.size(); ++bed_index)
		{
			const int student_id = item.beds.at(bed_index);
			if (student_id == 0) continue;
			if (!check::is_valid_student_id(student_id) || students_in_beds.contains(student_id))
			{ set_error(error, QStringLiteral("床位引用了非法学号或同一学生占用多个床位。")); return false; }
			students_in_beds.insert(student_id);
			bed_positions.insert(student_id, {key, bed_index + 1});
		}
		dorms.insert(key, item);
	}
	QSet<int> student_ids;
	QHash<int, QSet<int>> grade_sequences;
	for (const student_snapshot& item : snapshot.students)
	{
		if (!check::is_valid_student_id(item.id) || !check::is_valid_student_name(item.name) || !check::is_valid_gender(item.gender)
			|| !check::is_valid_grade(item.grade) || !check::is_valid_class_num(item.class_number)
			|| !check::is_student_id_consistent(item.id, item.grade, item.class_number) || student_ids.contains(item.id))
		{ set_error(error, QStringLiteral("学生字段非法或学号重复。")); return false; }
		const int sequence = check::student_id_sequence(item.id);
		if (grade_sequences[item.grade].contains(sequence))
		{ set_error(error, QStringLiteral("同一年级存在重复学号序号。")); return false; }
		grade_sequences[item.grade].insert(sequence);
		student_ids.insert(item.id);
		const bool all_empty = item.building_id == 0 && item.dorm_id == 0 && item.floor == 0 && item.bed_id == 0;
		const bool all_assigned = item.building_id > 0 && item.dorm_id > 0 && item.floor > 0 && item.bed_id > 0;
		if (!all_empty && !all_assigned)
		{ set_error(error, QStringLiteral("学生住宿位置字段不完整。")); return false; }
		if (all_empty)
		{
			if (students_in_beds.contains(item.id))
			{ set_error(error, QStringLiteral("未入住学生仍存在床位记录。")); return false; }
			continue;
		}
		const QString key = dorm_key(item.building_id, item.dorm_id);
		if (!dorms.contains(key) || item.floor != item.dorm_id / 100 || !bed_positions.contains(item.id)
			|| bed_positions.value(item.id).first != key || bed_positions.value(item.id).second != item.bed_id)
		{ set_error(error, QStringLiteral("学生位置与宿舍床位记录不一致。")); return false; }
		const dorm_snapshot& current_dorm = dorms.value(key);
		const building_snapshot& current_building = buildings.value(item.building_id);
		if (item.gender != 1 && item.gender != 2)
		{ set_error(error, QStringLiteral("已入住学生必须设置男或女。")); return false; }
		if (current_building.gender != 3 && current_building.gender != item.gender)
		{ set_error(error, QStringLiteral("学生性别与宿舍楼用途冲突。")); return false; }
		if (current_dorm.gender_lock == 0 || current_dorm.gender_lock != item.gender)
		{ set_error(error, QStringLiteral("学生性别与宿舍性别锁冲突。")); return false; }
	}
	for (int student_id : students_in_beds)
		if (!student_ids.contains(student_id))
		{ set_error(error, QStringLiteral("床位引用的学生不存在。")); return false; }
	return true;
}
