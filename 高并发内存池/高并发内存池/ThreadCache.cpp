#include "ThreadCache.h"


void* ThreadCache::FetchFromCentralCache(size_t size, size_t i) {
	//һ����centralcache����һ���ڴ棬batch��ʾ��һ���ĸ���
	size_t batch = min(SizeClass::NumMoveSize(size), freelist[i].MaxSize());

	//ȥcentralcache��ȡ
	void* begin = nullptr;
	void* end = nullptr;
	//��centralcache��ȡ�ڴ�ʱ����Ҫʹ��ȡ���ڴ���룬����centralcache�зִ�pagecache��ȡ����spanʱ�����
	size_t fetchCount = CentralCache::GetInstance()->FetchRangeObj(begin, end, batch, SizeClass::RoundUp(size));
	//��CentralCache��ȡ���˶���ڴ�飬�Ѷ���ķŽ�freelist�����У��´���������Ҫ��centralcache�����Ч��
	if (fetchCount > 1) {
		freelist[i].PushRange(NextObj(begin), end, fetchCount - 1);
	}
	//������������
	if (batch == freelist[i].MaxSize()) {
		freelist[i].SetMaxSize(freelist[i].MaxSize() + 1);
	}
	return begin;
}


void* ThreadCache::Allocate(size_t size) {
	int i = SizeClass::Index(size);
	//�����ڴ��С��Ӧ����������Ϊ�գ���centralcacheҪ
	if (freelist[i].Empty()) {
		return FetchFromCentralCache(size, i);
	}
	//��������Ϊ�գ�ֱ�Ӱ�����ͷ���ڴ淵��
	else {
		return freelist[i].Pop();
	}
}

void ThreadCache::Deallocate(void* obj, size_t size) {
	//�ø��ڴ��Ĵ�С�����Ӧ�����ĸ�freelist
	size_t i = SizeClass::Index(size);
	//������ڴ��ͷ�����Ӧ��freelist
	freelist[i].Push(obj);
	//�����freelist�ﵽһ�����ȣ������ͷŻ�centralcache
	if (freelist[i].Size() >= freelist[i].MaxSize()) {
		ListTooLong(freelist[i], size);
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size) {
	size_t betch = list.MaxSize();
	void* begin = nullptr;
	void* end = nullptr;
	list.PopRange(begin, end, betch);

	CentralCache::GetInstance()->ReleaseListToCentralCache(begin, size);

}