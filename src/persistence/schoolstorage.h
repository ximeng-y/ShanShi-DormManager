#ifndef SCHOOLSTORAGE_H
#define SCHOOLSTORAGE_H

#include "schoolsnapshot.h"

#include <QByteArray>
#include <QString>

//school 内部使用的文件持久化辅助类。本类不持有或修改任何业务对象。
class schoolstorage
{
public:
	static QByteArray encode_snapshot(const schoolsnapshot& snapshot, QString* error = nullptr);//编码并自检版本化JSON
	static bool decode_snapshot(const QByteArray& data, schoolsnapshot& snapshot, QString* error = nullptr,
		bool* newer_version = nullptr);//解析并完整校验JSON

private:
	static bool validate_snapshot(const schoolsnapshot& snapshot, QString* error);//校验字段、主键、引用及住宿双向一致性
};

#endif // SCHOOLSTORAGE_H
