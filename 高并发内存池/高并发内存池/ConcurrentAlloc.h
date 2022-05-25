#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

void* ConcurrentAlloc(size_t size) {
	if (size > MaxBytes) {
		//�߳�Ҫ������ڴ��С����threadcache freelist�е����ֵ64kʱ����Ҫ��pagecache����
		size_t npage = SizeClass::RoundUp(size) >> PAGESHIFT;   //�����ڴ��Сת��Ϊ����ҳ
		Span* span = PageCache::GetInstance()->NewSpan(npage);                  //��pagecacheҪ�ڴ�
		span->objsize = size;
		void* ptr = (void*)(span->startID << PAGESHIFT);
		return ptr;
	}
	else {
		if (TLSThreadCache == nullptr) {
			TLSThreadCache = new ThreadCache();
		}
		return TLSThreadCache->Allocate(size);
	}
	
}

void ConcurrentFree(void* ptr) {
	//����idSpanMap�ҵ�ptr���ĸ�span���棬�������span���Ի������ڴ�Ĵ�С
	Span* span = PageCache::GetInstance()->ObjectToSpan(ptr);
	size_t size = span->objsize;
	if (size > MaxBytes) {
		//����pagecache
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
	}
	else {
		TLSThreadCache->Deallocate(ptr, size);
	}
}