#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::sinst;

Span* GetOneSpan(SpanList& splist, size_t size) {
	//������spanlist���ҳ�һ�������ڴ��span
	Span* it = splist.Begin();
	while (it != splist.End()) {
		//�ҵ��˻����ڴ��span������span����
		if (it->list != nullptr) {
			return it;
		}
		it = it->next;
	}

	//�ߵ������ʾ��spanlist������û�п��õ�span����PageCacheҪspan
	size_t num = SizeClass::NumMovePage(size);        //����Ҫ��ҳ��С��span
	Span* newspan = PageCache::GetInstance()->NewSpan(num);
	//�Ѵ�pagecache�л�õ�span�г�С����������,����splist��
	char* begin = (char*)(newspan->startID << PAGESHIFT); //�»�ȡ����span����ʼ��ַ
	char* end = begin + (newspan->pagenum << PAGESHIFT);  //�»�ȡ����span��ĩβ��ַ
	while (begin < end) {
		char* next = begin + size;  //��һ��С�ڴ�����ʼ��ַ
		NextObj(begin) = newspan->list;
		newspan->list = begin;
		begin = next;
	}
	newspan->objsize = size;
	splist.Push(newspan);
	return newspan;
}

size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t n, size_t size) {
	//��ȡҪ�����ڴ��С��Ӧspan��spanlist��ϣ���е�λ��
	size_t i = SizeClass::Index(size);

	std::lock_guard<std::mutex> lock(spanlist[i].mtx);

	//�Ӷ�Ӧ��spanlist�л�ȡһ�����õ�span
	Span* span = GetOneSpan(spanlist[i], size);
	//�ڸ�span��ȡ��n��С���ڴ���threadcacheʹ��
	
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
		//ĳ��span�е��ڴ�ȫ�������ˣ��ͽ����ͷŻ�pagecache
		if (span->usecount == 0) {
			spanlist->Erase(span);
			span->list = nullptr;
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		}
		begin = next;
	}
}