hashTableTemplate.h
	创建Hash表类模板，提供的功能有：
	（1）指定shmkey或内存地址创建Hash表；
	（2）获取指定key元素；
	（3）遍历指定范围的元素，进行指定操作；
demo.cpp
	给出使用示例。
	
备注：
	（1）采用小于hash表大小的大质数尽量减少冲突，因为模的因子最少，冲突最少。因子最少的就是素数了。具体解释参见：https://blog.csdn.net/zhishengqianjun/article/details/79087525

缺点：
	该hash表模板未实现动态扩展，hash表容量不足时，需要重新指定空间后初始化。