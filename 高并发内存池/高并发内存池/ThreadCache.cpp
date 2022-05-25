#include "ThreadCache.h"


void* ThreadCache::FetchFromCentralCache(size_t size, size_t i) {
	//一次向centralcache申请一批内存，batch表示这一批的个数
	size_t batch = min(SizeClass::NumMoveSize(size), freelist[i].MaxSize());

	//去centralcache获取
	void* begin = nullptr;
	void* end = nullptr;
	//找centralcache获取内存时，需要使获取的内存对齐，否则centralcache切分从pagecache获取的新span时会出错
	size_t fetchCount = CentralCache::GetInstance()->FetchRangeObj(begin, end, batch, SizeClass::RoundUp(size));
	//从CentralCache获取到了多个内存块，把多余的放进freelist链表中，下次申请求不需要找centralcache，提高效率
	if (fetchCount > 1) {
		freelist[i].PushRange(NextObj(begin), end, fetchCount - 1);
	}
	//慢启动的增长
	if (batch == freelist[i].MaxSize()) {
		freelist[i].SetMaxSize(freelist[i].MaxSize() + 1);
	}
	return begin;
}


void* ThreadCache::Allocate(size_t size) {
	int i = SizeClass::Index(size);
	//申请内存大小对应的自由链表为空，找centralcache要
	if (freelist[i].Empty()) {
		return FetchFromCentralCache(size, i);
	}
	//自由链表不为空，直接把链表头的内存返回
	else {
		return freelist[i].Pop();
	}
}

void ThreadCache::Deallocate(void* obj, size_t size) {
	//用该内存块的大小算出他应属于哪个freelist
	size_t i = SizeClass::Index(size);
	//将这个内存块头插进对应的freelist
	freelist[i].Push(obj);
	//如果该freelist达到一定长度，将其释放回centralcache
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