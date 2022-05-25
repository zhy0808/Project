#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <assert.h>
#include <exception>
#include <time.h>


#ifdef _WIN32
#include <Windows.h>
#else
//����ƽ̨�������ڴ�ӿ�
#endif


typedef uintptr_t PageId;                   //uintptr_t : unsigned long int �޷��ų����ͣ���ƽ̨��ָ���С���
typedef uintptr_t SpanLength;


static const size_t NLISTS = 184;           //ThreadCache��freelist��ϣ���CentralCache��spanlist��ϣ��ĳ���
static const size_t MaxBytes = 64 * 1024;      //ThreadCache��freelist��ϣ��������������ӵ��ڴ��С
static const size_t NPAGES = 129;           //pagecache�й�ϣ��ĳ���
static const size_t PAGESHIFT = 12;         //һҳΪ�����ֽڣ�32λƽ̨��Ϊ4k����2^12
#ifdef _WIN32
	//32λƽ̨�µ�ַ��32λ��size_t���Ա�ʾ
	typedef size_t ADDRES;
#else
	typedef unsigned long long ADDRES;      //64λƽ̨��  [0,2^64)��ʾ��ַ
#endif
class SizeClass {
public:
	//[1 - 128�ֽ�]                ��8�ֽڶ���           ��freelist��ϣ����[0-16)�±��Ӧ��������֯
	//[129 - 1024�ֽ�]             ��16�ֽڶ���          ��freelist��ϣ����[16-72)�±��Ӧ��������֯
	//[1025 - 8*1024�ֽ�]		   ��128�ֽڶ���		 ��freelist��ϣ����[72-128)�±��Ӧ��������֯
	//[8*1024+1 - 64*1024�ֽ�]     ��1024�ֽڶ���        ��freelist��ϣ����[128-184)�±��Ӧ��������֯
	
	//����Ҫ����size��С���ڴ���freelist��ϣ���е��±�
	static size_t Index(size_t size) {
		assert(size < MaxBytes);
		int arr[4] = { 16, 56, 56, 56 };
		if (size <= 128) {
			return _Index(size, 3);
		}
		else if (size <= 1024) {
			return _Index(size - 128, 4) + arr[0];
		}
		else if (size <= 8 * 1024) {
			return _Index(size - 1024, 8) + arr[0] + arr[1];
		}
		else {
			return _Index(size - 8 * 1024, 10) + arr[0] + arr[1] + arr[2];
		}
	}
	static size_t _Index(size_t size, size_t shift) {
		//(size + ������ - 1) / ������ - 1 
		return ((size + (1 << shift) - 1) >> shift) - 1;
	}

	//���������ڴ��Ĵ�С���ThreadCache��CentralCacheҪ�ڴ��ʱ�����޸���
	static size_t NumMoveSize(size_t size) {
		if (size == 0)
			return 0;
		//ΪʲôҪ��64k��������������������������������������������������������������������
		int num = MaxBytes / size;
		//С����һ�λ�ȡ�����࣬�������
		if (num < 2)
			num = 2;
		else if (num > 512)
			num = 512;
		return num;
	}

	//CentralCache��pagechcheҪ��ҳ��span
	static size_t NumMovePage(size_t size) {
		//threadcacheһ����centralcacheҪNumMoveSize(size)��С�ڴ��
		size_t num = NumMoveSize(size);
		//��centralcache��pagechcheҪspanʱ����ҪNumMoveSize(size)*size�ֽ�
		size_t spansize = num * size;
		//����Ҫ���ֽ�������һҳ�������ֽ���������Ҫ��ҳ�������Ƽ�����2��PAGESHIFT�η�
		size_t npage = spansize >> PAGESHIFT;
		//�������С��1ҳ�����ٷ���1ҳ
		if (npage == 0)
			npage = 1;

		return npage;
	}

	//���ڴ�����
	static size_t RoundUp(size_t bytes) {
		if (bytes <= 128) {
			return _RoundUp(bytes, 8, 3);
		}
		else if (bytes <= 1024) {
			return _RoundUp(bytes, 16, 4);
		}
		else if (bytes <= 8 * 1024) {
			return _RoundUp(bytes, 128, 8);
		}
		else if (bytes <= 64 * 1024) {
			return _RoundUp(bytes, 1024, 10);
		}
		//����64k���ڴ�飬��һҳ�Ĵ�С����
		else {
			return _RoundUp(bytes, 1 << PAGESHIFT, PAGESHIFT);
		}
	}
	static size_t _RoundUp(size_t bytes, size_t align, size_t shift) {
		return (((bytes + align - 1) >> shift) << shift);
	}
};


static void*& NextObj(void* obj) {
	return *((void**)obj);
}
class FreeList {
public:
	//�����������ڴ��ͷ���freelist������
	void Push(void* obj) {
		NextObj(obj) = head;
		head = obj;
		size++;

	}
	void PushRange(void* begin, void* end, size_t fetchCount) {
		NextObj(end) = head;
		head = begin;
		size += fetchCount;
	}
	//����ͷ�ڴ������ȥ
	void* Pop() {
		void* obj = head;
		head = *(void**)head;
		size--;
		return obj;
	}
	//��������ͷ��ڴ�ʱ��ȡ�������е�ǰn��
	void PopRange(void*& begin, void*& end, size_t n) {
		begin = head;
		for (size_t i = 0; i < n; ++i) {
			end = head;
			head = NextObj(head);
		}
		NextObj(end) = nullptr;
		size -= n;
	}
	bool Empty() {
		return head == nullptr;
	}
	size_t MaxSize() {
		return maxsize;
	}
	void SetMaxSize(size_t size) {
		maxsize = size;
	}

	size_t Size() {
		return size;
	}
private:
	void* head = nullptr;
	size_t maxsize = 1;          //threadcache��centralcacheҪ�ڴ��ʱ��������
	size_t size = 0;             //freelist���ڴ��ĸ���
}; 






struct Span {
	void* list = nullptr;     //��span�е��ڴ�ֳ�С���������������ڻ���
	size_t usecount = 0;      //ʹ�ü�����Ϊ0��ʾ��span����С�ڴ�鶼��
	size_t objsize;           //��span��ÿ��С�ڴ��Ĵ�С
	
	PageId startID = 0;         //��span����ʼҳ��
	SpanLength pagenum = 0;    //��span����ʼҳ��ʼ������ҳ������
	
	Span* next = nullptr;
	Span* prev = nullptr;
};


class SpanList {
public:
	SpanList() {
		head = new Span;
		head->next = head;
		head->prev = head;
	}

	Span* Begin() {
		return head->next;
	}
	Span* End() {
		return head;
	}
	void Push(Span* newspan) {
		newspan->next = head->next;
		head->next->prev = newspan;
		head->next = newspan;
		newspan->prev = head;
	}
	Span* Pop() {
		Span* span = Begin();
		Erase(span);
		return span;
	}
	void Insert(Span* cur, Span* newspan) {
		Span* prev = cur->prev;
		prev->next = newspan;
		newspan->prev = prev;
		newspan->next = cur;
		cur->prev = newspan;
	}
	//�����ɾ��ʵ���ǰ�һ��span��spanlist�����ӹ�ϵȥ�����������ͷŵ�����ڴ�
	void Erase(Span* cur) {
		Span* prev = cur->prev;
		prev->next = cur->next;
		cur->next->prev = prev;
	}
	bool Empty() {
		return head->next == head;
	}
	std::mutex mtx;

private:
	Span* head;
};


static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage * (1 << PAGESHIFT),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap��
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap��
#endif
}