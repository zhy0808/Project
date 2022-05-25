#pragma once
#include "Common.h"
#include "CentralCache.h"

class ThreadCache {
public:
	//申请ThreadCache中的内存
	void* Allocate(size_t size);
	//如果ThreadCache中内存不够，向CentralCache要
	void* FetchFromCentralCache(size_t size, size_t i);


	//释放内存，将释放的内存放入ThreadCache哈希表中的某一个freelist中
	void Deallocate(void* obj, size_t size);
	//freelist过长，回收内存
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList freelist[NLISTS];
};

//线程本地存储，每个线程独有一份，这里让每个线程有一个自己的threadcache
static __declspec(thread) ThreadCache* TLSThreadCache = nullptr;