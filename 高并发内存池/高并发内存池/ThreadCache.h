#pragma once
#include "Common.h"
#include "CentralCache.h"

class ThreadCache {
public:
	//����ThreadCache�е��ڴ�
	void* Allocate(size_t size);
	//���ThreadCache���ڴ治������CentralCacheҪ
	void* FetchFromCentralCache(size_t size, size_t i);


	//�ͷ��ڴ棬���ͷŵ��ڴ����ThreadCache��ϣ���е�ĳһ��freelist��
	void Deallocate(void* obj, size_t size);
	//freelist�����������ڴ�
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList freelist[NLISTS];
};

//�̱߳��ش洢��ÿ���̶߳���һ�ݣ�������ÿ���߳���һ���Լ���threadcache
static __declspec(thread) ThreadCache* TLSThreadCache = nullptr;