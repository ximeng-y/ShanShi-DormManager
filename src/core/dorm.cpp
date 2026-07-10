#include "dorm.h"
#include <QVector>
#include <QRandomGenerator>
#include "system/check.h"

dorm::dorm()//构造函数
{
	id = 0;
	max_num = 0;
	building_id = 0;
	//所在楼层使用get_floor()方法解析宿舍号获取
	for_gender = 0;//默认未锁定, 表达"未设置"
	beds.clear();
}

//获取信息
int dorm::get_id() const//获取宿舍号
{
	return id;
}
int dorm::get_max_num() const//获取最大人数
{
	return max_num;
}
int dorm::get_current_num() const//获取实际人数(统计 beds 中非零床位)
{
	int count = 0;
	for (int student_id : beds)
	{
		if (student_id != 0)
			++count;
	}
	return count;
}
int dorm::get_building_id() const//获取所在宿舍楼号
{
	return building_id;
}
int dorm::get_floor() const//获取所在楼层(派生自 id/100, 不独立存储)
{
	return id / 100;
}
int dorm::get_for_gender() const//获取房间性别锁定(0=未锁定/1=男舍/2=女舍)
{
	return for_gender;
}

bool dorm::accepts_gender(int gender) const//判断本房间性别锁定是否接纳学生性别: 未锁定(0)接纳任意, 否则要求相等
{
	return for_gender == 0 || for_gender == gender;
}

int dorm::get_student_id(int bed_id) const//获取宿舍内指定床位学生学号
{
	if (!check::is_valid_bed_id(bed_id, max_num))//检查床位号是否合法
		return -1;
	int student_id = beds[bed_id - 1];//下标=床位号-1
	if (student_id == 0)
		return -1;//该床位为空
	return student_id;
}

QVector<int> dorm::get_student_id_list() const//获取宿舍内所有已入住学生的学号表(不含空床)
{
	QVector<int> id_list;
	for (int student_id : beds)
	{
		if (student_id != 0)
			id_list.append(student_id);
	}
	return id_list;
}

//判断存在性
int dorm::is_bed_occupied(int bed_id) const//判断指定床位是否存在及其状态
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;//bed_id非法/宿舍不存在此处床位
	if (beds[bed_id - 1] == 0)
		return 0;//床位存在且为空
	return 1;//床位存在且有人
}
bool dorm::is_student_exist(int student_id) const//判断指定学号的学生是否在宿舍内
{
	for (int existing : beds)//在以床位为下标，存储学生学号的数组中查找
	{
		if (existing == student_id)
			return true;
	}
	return false;
}

//设置信息
bool dorm::set_id(int id)//设置宿舍号
{
	if (!check::is_valid_dorm_id(id))
		return false;
	this->id = id;
	return true;
}
bool dorm::set_max_num(int max_num)//设置最大人数(同步 resize beds)
{
	if (max_num < 1)//基本合法性
		return false;
	
	if (max_num < this->max_num)//缩容时: 检查将被丢弃的床位(新长度之后的部分)是否有人, 有人则拒绝, 防静默丢人（高层次的自动分配学生至其它宿舍或暂时令学生离宿需在dormmanager中实现）
	{
		for (int i = max_num; i < beds.size(); ++i)//此处不使用(max_num - 1)并非bug，而是max_num是新长度，i从新长度开始，正好是开始被丢弃的床位的下标
		{
			if (beds[i] != 0)
				return false;//被丢弃的床位仍有人, 拒绝缩容
		}
	}
	this->max_num = max_num;
	beds.resize(max_num);//扩容时新床位默认值为 0(空床)
	return true;
}
bool dorm::set_building_id(int building_id)//设置所在宿舍楼号
{
	if (!check::is_valid_building_id(building_id))
		return false;
	this->building_id = building_id;
	return true;
}
bool dorm::set_for_gender(int gender)//钦定房间性别(0=未锁定/1=男舍/2=女舍)
{
	if (!check::is_valid_gender(gender))//复用学生性别校验(0/1/2)
		return false;
	this->for_gender = gender;
	return true;
}

//添加学生: 只写入 beds，不查询或同步 student 本体
int dorm::add_student(int student_id, int gender)//自动分配最小空床位
{
	if (!check::is_valid_dorm_id(this->id) || max_num < 1)//前置校验1: dorm 自身必须已配置(id 合法且 max_num 已设置), 否则 beds 为空无处安放
		return -5;//宿舍未配置
	if (!check::is_valid_student_id(student_id) || gender < 1 || gender > 2)//前置校验2: 学号与学生性别格式
		return -1;//student_id或gender非法
	if (is_full())//前置校验3: 宿舍不能满员
		return -4;//宿舍已满
	if (!accepts_gender(gender))//前置校验4: 房间已锁定性别时要求匹配
		return -7;//性别与房间锁定不符

	for (int i = 0; i < beds.size(); ++i)//找最小空床位
	{
		if (beds[i] == 0)
		{
			beds[i] = student_id;
			if (for_gender == 0)//空房先到先得: 把房间性别锁定为首住客性别
				for_gender = gender;
			return i + 1;//返回床位号(自然数)
		}
	}
	return -4;//理论不可达(is_full 已挡), 兜底返回已满
}
int dorm::add_student(int student_id, int gender, int bed_id)//添加学生-指定床位
{
	if (!check::is_valid_dorm_id(this->id) || max_num < 1)//前置校验1: dorm 自身必须已配置
		return -5;//宿舍未配置
	if (!check::is_valid_student_id(student_id) || gender < 1 || gender > 2)//前置校验2: 学号与学生性别格式
		return -1;//student_id或gender非法
	if (!check::is_valid_bed_id(bed_id, max_num))//前置校验3: 床位号合法性
		return -1;//bed_id非法
	if (beds[bed_id - 1] != 0)
		return -2;//床位已被占用
	if (!accepts_gender(gender))//前置校验4: 房间已锁定性别时要求匹配
		return -7;//性别与房间锁定不符

	beds[bed_id - 1] = student_id;//指定床位
	if (for_gender == 0)//空房先到先得: 把房间性别锁定为首住客性别
		for_gender = gender;
	return bed_id;
}

//移除学生(按学号)
int dorm::remove_student(int student_id)
{
	if (!check::is_valid_student_id(student_id))//前置校验1: 学号合法性
		return -1;//student_id非法
	for (int i = 0; i < beds.size(); ++i)
	{
		if (beds[i] == student_id)
		{
			beds[i] = 0;//释放床位
			return i + 1;//返回被释放的床位号
		}
	}
	return 0;//该学生不在本宿舍
}

//调换/移动床位
int dorm::swap_student(int from, int to)
{
	if (!check::is_valid_bed_id(from, max_num) || !check::is_valid_bed_id(to, max_num))
		return -1;//bed_id非法
	if (from == to)
		return -2;//同一床位, 无需操作
	if (beds[from - 1] == 0)
		return 0;//from床位为空

	//to 为空则移动, to 有人则互换; 直接交换两下标的值即可覆盖两种情形
	int tmp = beds[from - 1];
	beds[from - 1] = beds[to - 1];
	beds[to - 1] = tmp;

	return 1;
}

bool dorm::is_full() const//判断宿舍是否已满
{
	return get_current_num() >= max_num;
}

bool dorm::is_empty() const//判断宿舍是否为空
{
	return get_current_num() == 0;
}

int dorm::get_empty_count() const//获取当前空床位数
{
	return max_num - get_current_num();
}

int dorm::get_occupied_count() const//获取当前已占用床位数
{
	return get_current_num();
}

void dorm::clear_students()//清空所有床位(保留房间性别锁定)
{
	beds.fill(0);//长度不变, 全部置 0
}

void dorm::clear_students_reset_gender()//清空住客并重置房间性别为未锁定
{
	clear_students();//先按常规清空住客(保留性别)
	for_gender = 0;//再彻底放开房间性别锁定
}

//随机打乱本宿舍内学生与床位的对应关系(Fisher-Yates 洗牌整个 beds 数组, 含空床)
int dorm::shuffle_beds()
{
	int current = get_current_num();
	if (current == 0)
		return 0;//空房无需打乱

	//Fisher-Yates: 从末尾往前, 每步与 [0, i] 内随机位置交换。
	for (int i = beds.size() - 1; i > 0; --i)
	{
		int j = QRandomGenerator::global()->bounded(i + 1);//[0, i] 等概率
		int tmp = beds[i];
		beds[i] = beds[j];
		beds[j] = tmp;
	}
	return current;
}
