#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

void* ConcurrentAlloc(size_t size) {
	if (size > MaxBytes) {
		//线程要申请的内存大小大于threadcache freelist中的最大值64k时，需要找pagecache申请
		size_t npage = SizeClass::RoundUp(size) >> PAGESHIFT;   //根据内存大小转换为多少页
		Span* span = PageCache::GetInstance()->NewSpan(npage);                  //找pagecache要内存
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
	//根据idSpanMap找到ptr在哪个span里面，根据这个span可以获得这块内存的大小
	Span* span = PageCache::GetInstance()->ObjectToSpan(ptr);
	size_t size = span->objsize;
	if (size > MaxBytes) {
		//还给pagecache
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
	}
	else {
		TLSThreadCache->Deallocate(ptr, size);
	}
}