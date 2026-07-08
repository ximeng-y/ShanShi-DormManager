#ifndef DORM_H
#define DORM_H

#include <QVector>

//存储模型: 宿舍仅持有床位到学号的映射。
//beds 为定长数组, 下标 = 床位号 - 1, 值 = 学号, 0 表示空床, 长度恒等于 max_num。
//学生本体的唯一归属是 studentmanager(QHash<int,student>), dorm 只认学号。
class dorm
{
public:
	dorm();//构造函数

	//获取信息
	int get_id() const;//获取宿舍号
	int get_max_num() const;//获取最大人数
	int get_current_num() const;//获取实际人数(统计 beds 中非零床位)
	int get_building_id() const;//获取所在宿舍楼号
	int get_floor() const;//获取所在楼层
	//返回值: >0=该床位学生学号  -1=bed_id非法或该床位为空
	int get_student_id(int bed_id) const;//获取宿舍内指定床位学生学号
	QVector<int> get_student_id_list() const;//获取宿舍内所有已入住学生的学号表(不含空床)

	//判断存在性(两个方法参数同为 int, 语义不同, 特意区分命名)
	//返回值: 1=该床位有人  0=该床位为空  -1=bed_id非法
	int is_bed_occupied(int bed_id) const;//判断指定床位是否有人(按床位号)
	bool is_student_exist(int student_id) const;//判断指定学号的学生是否在本宿舍(按学号)


	//设置信息
	bool set_id(int id);//设置宿舍号
	bool set_max_num(int max_num);//设置最大人数(会同步 resize beds; 缩容时若被丢弃床位有人则拒绝并返回 false)
	bool set_building_id(int building_id);//设置所在宿舍楼号
	bool set_floor(int floor);//设置所在楼层

	//管理学生: beds 是床位占用的权威, 同时经 studentmanager 正向/反向同步 student 本体的位置四字段。
	//add_student 成功时调 studentmanager::assign_dorm_info 写入 dorm_id/bed_id/building_id/floor;
	//remove_student/clear_students 调 studentmanager::clear_dorm_info 清零四字段。
	//注意: dorm 仍只认学号做增删查, 学号是否真实注册于 studentmanager 由调用方/未来协调层保证。
	//返回值: >0=成功(即分配的床位号)  -1=student_id非法  -3=学生已有宿舍  -4=宿舍已满  -5=宿舍未配置(id非法或max_num<1)
	int add_student(int student_id);//添加学生(自动分配最小空床位)

	//返回值: >0=成功(即指定床位号)  -1=student_id非法或bed_id非法  -2=床位已被占用  -3=学生已有宿舍  -5=宿舍未配置
	int add_student(int student_id, int bed_id);//添加学生(指定床位)

	//返回值: >0=成功(即被释放的床位号)  0=该学生不在本宿舍  -1=student_id非法
	int remove_student(int student_id);//移除学生(按学号)

	//swap 后 beds 内两床位的值互换; from 空则不操作。
	//交换后经 studentmanager::assign_dorm_info 同步两侧(非空)学生本体的 bed_id; dorm_id/building_id/floor 未变沿用 dorm 字段。
	//返回值: 1=成功(to空则移入, to有人则互换)  0=from床位为空  -1=bed_id非法  -2=from==to
	int swap_student(int from, int to);//调换/移动床位(from→to)

	bool is_full() const;//判断宿舍是否已满
	void clear_students();//清空所有床位(beds 全部置 0, 长度不变)

private:
	int id;//宿舍号
	int max_num;//最大人数
	int building_id;//所在宿舍楼号(1~99)
	int floor;//所在楼层(1~max_floor)
	QVector<int> beds;//床位->学号映射作为伪指针(下标=床位号-1, 值=学号, 0=空床, size==max_num)
};

#endif // DORM_H
