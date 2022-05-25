#pragma once
#include "Common.h"

class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &sinst;
	}
	//给threadcache一批内存块
	size_t FetchRangeObj(void*& begin, void*& end, size_t n, size_t size);
	//从某个spanlist中获取一个还有小内存块span
	Span* GetOneSpane(SpanList list, size_t size);

	//threadcache归还一批小内存块到centralcache中
	void ReleaseListToCentralCache(void* start, size_t size);
private:
	SpanList spanlist[NLISTS];

private:
	//构造函数私有
	CentralCache() {

	}
	//C++11防拷贝
	CentralCache(const CentralCache&) = delete;

	//单例模式—饿汉模式，在类内声明静态对象，将类的构造和拷贝构造函数声明为私有，且拷贝构造delete
	//在.cpp文件中声明对象，保证该类只有一个单例对象在CentralCache.obj中，如果在.h中定义，别的文件包含该.h文件时，会拷贝这个对象
	static CentralCache sinst;
};

