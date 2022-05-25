#include "PageCache.h"

PageCache PageCache::sinst;

void* PageCache::SystemAllocPage(size_t n) {
	return SystemAlloc(n);
}

//����centralcache��Ҫ��span��С����iҳ�������ض�Ӧ��span
Span* PageCache::NewSpan(size_t i) {

	std::lock_guard<std::recursive_mutex> lock(_mtx);

	//conurrentallocҪ������ڴ����pagecache��ϣ��������span�ˣ���Ҫ��ϵͳ����
	if (i >= NPAGES) {
		void* ptr = SystemAlloc(i);
		Span* span = new Span;
		span->startID = (ADDRES)ptr >> PAGESHIFT;
		span->pagenum = i;
		
		idSpanMap[span->startID] = span;
		return span;
	}
	if (!spanlist[i].Empty()) {
		//�ж�Ӧ��С��span��ֱ�ӷ���
		//Span* span = spanlist[i].Begin();
		//spanlist[i].Erase(span);
		Span* span = spanlist[i].Pop();
		return span;
	}
	//û�ж�Ӧ��С��span������spanlist��ϣ������û�б������span������У��з�
	for (int j = i + 1; j < NPAGES; ++j) {
		//�Ӵ�span��ͷ���г���Ҫ��span
		if (!spanlist[j].Empty()) {
			/*Span* span = spanlist[j].Begin();
			spanlist[j].Erase(span);*/
			Span* span = spanlist[j].Pop();
			//Ҫ�Ӵ�span���г�iҳ��ʣ�µ���split��ʾ
			Span* split = new Span;
			split->startID = span->startID + i;
			split->pagenum = span->pagenum - i;
			//���зֺ��ʣ�²��ֵ�ҳ���µ�span��Ӧ����
			for (PageId k = 0; k < split->pagenum; ++k) {
				idSpanMap[split->startID + k] = split;
			}
			spanlist[split->pagenum].Push(split);
			span->pagenum = i;
			return span;
		}

	}
	//����spanlist��ϣ����û�б���Ҫ��span��ģ���ϵͳ��ȡ
	Span* bigspan = new Span;
	void* mem = SystemAllocPage(NPAGES - 1);
	//4k��1ҳ��ҳ�ſ����ڴ����ʼ��ַ����ҳ�Ĵ�С�õ�
	bigspan->startID = (size_t)mem >> PAGESHIFT;
	bigspan->pagenum = NPAGES - 1;
	//�����뵽���ڴ��ÿһҳ��span��map��Ӧ����
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

	//������
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

	//��ǰ�ϲ�
	while (1) {
		PageId preid = span->startID - 1;
		//��ǰһҳ��Ӧ��span
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
		//���ǰһ��span��usecountΪ0��˵��ǰһ��spanδ��ʹ�ã����Ժϲ�
		if (prespan->usecount != 0) {
			break;
		}

		// ����128ҳ������Ҫ�ϲ�
		if (prespan->pagenum + span->pagenum >= NPAGES)
		{
			break;
		}


		//��ǰһ��span��spanlist��ϣ����ɾ��
		spanlist[prespan->pagenum].Erase(prespan);
		//��ǰһ��span�뵱ǰspan�ϲ�
		span->startID = prespan->startID;
		span->pagenum += prespan->pagenum;
		//��ؼ��ģ���ǰһ��span��ҳ��spanӳ���ϵ���ģ��뵱ǰspanӳ������
		for (PageId i = 0; i < prespan->pagenum; ++i) {
			idSpanMap[prespan->startID + i] = span;
		}
		delete prespan;
	}

	//���ϲ�
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

		// ����128ҳ������Ҫ�ϲ���
		if (nextspan->pagenum + span->pagenum >= NPAGES)
		{
			break;
		}

		spanlist[nextspan->pagenum].Erase(nextspan);
		//��ʼ�ϲ�
		span->pagenum += nextspan->pagenum;
		for (PageId i = 0; i < nextspan->pagenum; ++i) {
			idSpanMap[nextspan->startID + i] = span;
		}
		delete nextspan;	
	}

	//�Ѻϳ����span�����Ӧλ��
	spanlist[span->pagenum].Push(span);
}







