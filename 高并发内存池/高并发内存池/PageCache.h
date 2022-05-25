#pragma once
#include "Common.h"
#include "PageMap.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &sinst;
	}
	//给centralcache返回一个i页的span
	Span* NewSpan(size_t i);
	//向系统要内存
	void* SystemAllocPage(size_t n);
	//释放内存时计算要释放的内存的首地址对应的是那一页，根据页号在map中找到对应的Span
	Span* ObjectToSpan(void* ptr);
	//CentralCache归还span到pagecache中
	void ReleaseSpanToPageCache(Span* span);

private:
	SpanList spanlist[NPAGES];
	//std::map<PageId, Span*> idSpanMap;
	TCMalloc_PageMap2<32 - PAGESHIFT> idSpanMap;

	//递归锁
	std::recursive_mutex _mtx;



private:
	PageCache() {

	}
	PageCache(const PageCache&) = delete;
	static PageCache sinst;
};


