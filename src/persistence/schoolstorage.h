#ifndef SCHOOLSTORAGE_H
#define SCHOOLSTORAGE_H

#include "schoolsnapshot.h"

#include <QByteArray>
#include <QString>

enum class storage_load_status
{
	loaded,
	not_found,
	newer_version,
	invalid,
	io_error
};

//school 内部使用的文件持久化辅助类。本类不持有或修改任何业务对象。
class schoolstorage
{
public:
	static QString executable_data_directory();//exe同级userdata目录
	static QString fallback_data_directory();//用户LocalAppData下userdata目录
	static bool directory_is_writable(const QString& directory, QString* error = nullptr);//实际原子写入探测目录权限

	void set_data_directory(const QString& directory);//激活本次运行使用的数据目录
	QString data_directory() const;
	QString primary_file_path() const;
	QString backup_file_path() const;
	storage_load_status load_primary(schoolsnapshot& snapshot, QString* error = nullptr) const;
	storage_load_status load_backup(schoolsnapshot& snapshot, QString* error = nullptr) const;
	bool create_initial_file(const schoolsnapshot& empty_snapshot, QString* error = nullptr);//首次启动创建正式空文件
	bool save_snapshot(const schoolsnapshot& snapshot, QString* error = nullptr);//备份当前正式文件后原子保存新快照
	bool rebuild_primary(const schoolsnapshot& snapshot, QString* error = nullptr);//从已验证备份重建正式文件，不轮换备份
	bool archive_invalid_file(const QString& source_path, QString* archived_path = nullptr, QString* error = nullptr) const;

	static QByteArray encode_snapshot(const schoolsnapshot& snapshot, QString* error = nullptr);//编码并自检版本化JSON
	static bool decode_snapshot(const QByteArray& data, schoolsnapshot& snapshot, QString* error = nullptr,
		bool* newer_version = nullptr);//解析并完整校验JSON

private:
	storage_load_status load_file(const QString& path, schoolsnapshot& snapshot, QString* error) const;
	static bool write_atomic(const QString& path, const QByteArray& data, QString* error);
	static bool validate_snapshot(const schoolsnapshot& snapshot, QString* error);//校验字段、主键、引用及住宿双向一致性

	QString current_directory;
};

#endif // SCHOOLSTORAGE_H
