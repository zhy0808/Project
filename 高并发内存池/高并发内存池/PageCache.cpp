#include "PageCache.h"

PageCache PageCache::sinst;

void* PageCache::SystemAllocPage(size_t n) {
	return SystemAlloc(n);
}

//根据centralcache需要的span大小（含i页），返回对应的span
Span* PageCache::NewSpan(size_t i) {

	std::lock_guard<std::recursive_mutex> lock(_mtx);

	//conurrentalloc要申请的内存大于pagecache哈希表中最大的span了，需要向系统申请
	if (i >= NPAGES) {
		void* ptr = SystemAlloc(i);
		Span* span = new Span;
		span->startID = (ADDRES)ptr >> PAGESHIFT;
		span->pagenum = i;
		
		idSpanMap[span->startID] = span;
		return span;
	}
	if (!spanlist[i].Empty()) {
		//有对应大小的span，直接返回
		//Span* span = spanlist[i].Begin();
		//spanlist[i].Erase(span);
		Span* span = spanlist[i].Pop();
		return span;
	}
	//没有对应大小的span，遍历spanlist哈希表，找有没有比他大的span，如果有，切分
	for (int j = i + 1; j < NPAGES; ++j) {
		//从大span的头部切出需要的span
		if (!spanlist[j].Empty()) {
			/*Span* span = spanlist[j].Begin();
			spanlist[j].Erase(span);*/
			Span* span = spanlist[j].Pop();
			//要从大span中切出i页，剩下的用split表示
			Span* split = new Span;
			split->startID = span->startID + i;
			split->pagenum = span->pagenum - i;
			//将切分后的剩下部分的页与新的span对应起来
			for (PageId k = 0; k < split->pagenum; ++k) {
				idSpanMap[split->startID + k] = split;
			}
			spanlist[split->pagenum].Push(split);
			span->pagenum = i;
			return span;
		}

	}
	//整个spanlist哈希表中没有比需要的span大的，向系统获取
	Span* bigspan = new Span;
	void* mem = SystemAllocPage(NPAGES - 1);
	//4k是1页，页号可由内存的起始地址除以页的大小得到
	bigspan->startID = (size_t)mem >> PAGESHIFT;
	bigspan->pagenum = NPAGES - 1;
	//将申请到的内存的每一页和span用map对应起来
	for (PageId k = 0; k < bigspan->pagenum; ++k) {
		idSpanMap[bigspan->startID + k] = bigspan;
	}
	spanlist[NPAGES - 1].Push(bigspan);
	return NewSpan(i);
}

Span* PageCache::ObjectToSpan(void* ptr) {

	//std::lock_guard<std::recursive_mutex> lock(_mtx);
	PageId i = (ADDRES)ptr >> PAGESHIFT;
	/*auto it = idSpanMap.find(i);
	if (it != idSpanMap.end()) {
		return it->second;
	}
	else {
		assert(false);
		return nullptr;
	}*/

	//基数树
	Span* span = idSpanMap.get(i);
	if (span != nullptr)
	{
		return span;
	}
	else
	{
		assert(false);
		return nullptr;
	}

}

void PageCache::ReleaseSpanToPageCache(Span* span) {

	if (span->pagenum >= NPAGES) {
		idSpanMap.erase(span->startID);
		void* ptr = (void*)(span->startID << PAGESHIFT);
		SystemFree(ptr);
		delete span;
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(_mtx);

	//向前合并
	while (1) {
		PageId preid = span->startID - 1;
		//找前一页对应的span
		/*auto it = idSpanMap.find(preid);
		if (it == idSpanMap.end()) {
			break;
		}*/

		Span* prespan = idSpanMap.get(preid);
		if (prespan == nullptr)
		{
			break;
		}

		//Span* prespan = it->second;
		//如果前一个span的usecount为0，说明前一个span未被使用，可以合并
		if (prespan->usecount != 0) {
			break;
		}

		// 超过128页，不需要合并
		if (prespan->pagenum + span->pagenum >= NPAGES)
		{
			break;
		}


		//将前一个span从spanlist哈希表中删除
		spanlist[prespan->pagenum].Erase(prespan);
		//将前一个span与当前span合并
		span->startID = prespan->startID;
		span->pagenum += prespan->pagenum;
		//最关键的，将前一个span中页和span映射关系更改，与当前span映射起来
		for (PageId i = 0; i < prespan->pagenum; ++i) {
			idSpanMap[prespan->startID + i] = span;
		}
		delete prespan;
	}

	//向后合并
	while (1) {
		PageId nextid = span->startID + span->pagenum;
		/*auto it = idSpanMap.find(nextid);
		if (it == idSpanMap.end()) {
			break;
		}
		Span* nextspan = it->second;*/


		Span* nextspan = idSpanMap.get(nextid);
		if (nextspan == nullptr)
		{
			break;
		}


		if (nextspan->usecount != 0) {
			break;
		}

		// 超过128页，不需要合并了
		if (nextspan->pagenum + span->pagenum >= NPAGES)
		{
			break;
		}

		spanlist[nextspan->pagenum].Erase(nextspan);
		//开始合并
		span->pagenum += nextspan->pagenum;
		for (PageId i = 0; i < nextspan->pagenum; ++i) {
			idSpanMap[nextspan->startID + i] = span;
		}
		delete nextspan;	
	}

	//把合出大的span插入对应位置
	spanlist[span->pagenum].Push(span);
}







