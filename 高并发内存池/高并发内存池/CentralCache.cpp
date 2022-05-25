#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::sinst;

Span* GetOneSpan(SpanList& splist, size_t size) {
	//遍历该spanlist，找出一个还有内存的span
	Span* it = splist.Begin();
	while (it != splist.End()) {
		//找到了还有内存的span，将该span返回
		if (it->list != nullptr) {
			return it;
		}
		it = it->next;
	}

	//走到这里，表示该spanlist链表中没有可用的span，找PageCache要span
	size_t num = SizeClass::NumMovePage(size);        //计算要几页大小的span
	Span* newspan = PageCache::GetInstance()->NewSpan(num);
	//把从pagecache中获得的span切成小块连成链表,挂在splist中
	char* begin = (char*)(newspan->startID << PAGESHIFT); //新获取到的span的起始地址
	char* end = begin + (newspan->pagenum << PAGESHIFT);  //新获取到的span的末尾地址
	while (begin < end) {
		char* next = begin + size;  //下一个小内存块的起始地址
		NextObj(begin) = newspan->list;
		newspan->list = begin;
		begin = next;
	}
	newspan->objsize = size;
	splist.Push(newspan);
	return newspan;
}

size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t n, size_t size) {
	//获取要申请内存大小对应span在spanlist哈希表中的位置
	size_t i = SizeClass::Index(size);

	std::lock_guard<std::mutex> lock(spanlist[i].mtx);

	//从对应的spanlist中获取一个可用的span
	Span* span = GetOneSpan(spanlist[i], size);
	//在该span中取出n个小的内存块给threadcache使用
	
	size_t ret = 0;
	begin = span->list;
	void* cur = begin;
	void* prev = cur;
	while (ret < n && cur != nullptr) {
		prev = cur;
		cur = NextObj(cur);
		++ret;
		span->usecount++;
	}
	end = prev;
	span->list = cur;
	NextObj(prev) = nullptr;
	return ret;
}



void CentralCache::ReleaseListToCentralCache(void* begin, size_t size) {
	size_t i = SizeClass::Index(size);
	std::lock_guard<std::mutex> lock(spanlist[i].mtx);

	while (begin) {
		void* next = NextObj(begin);
		Span* span = PageCache::GetInstance()->ObjectToSpan(begin);
		NextObj(begin) = span->list;
		span->list = begin;
		span->usecount--;
		//某个span中的内存全还回来了，就将其释放回pagecache
		if (span->usecount == 0) {
			spanlist->Erase(span);
			span->list = nullptr;
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		}
		begin = next;
	}
}