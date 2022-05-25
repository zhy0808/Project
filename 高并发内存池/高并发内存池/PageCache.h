#pragma once
#include "Common.h"
#include "PageMap.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &sinst;
	}
	//��centralcache����һ��iҳ��span
	Span* NewSpan(size_t i);
	//��ϵͳҪ�ڴ�
	void* SystemAllocPage(size_t n);
	//�ͷ��ڴ�ʱ����Ҫ�ͷŵ��ڴ���׵�ַ��Ӧ������һҳ������ҳ����map���ҵ���Ӧ��Span
	Span* ObjectToSpan(void* ptr);
	//CentralCache�黹span��pagecache��
	void ReleaseSpanToPageCache(Span* span);

private:
	SpanList spanlist[NPAGES];
	//std::map<PageId, Span*> idSpanMap;
	TCMalloc_PageMap2<32 - PAGESHIFT> idSpanMap;

	//�ݹ���
	std::recursive_mutex _mtx;



private:
	PageCache() {

	}
	PageCache(const PageCache&) = delete;
	static PageCache sinst;
};


