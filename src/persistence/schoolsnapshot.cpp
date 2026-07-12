#include "schoolsnapshot.h"

bool schoolsnapshot::data_equals(const schoolsnapshot& other) const
{
	return format == other.format
		&& version == other.version
		&& buildings == other.buildings
		&& dorms == other.dorms
		&& students == other.students;
}
