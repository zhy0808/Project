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
//其他平台的申请内存接口
#endif


typedef uintptr_t PageId;                   //uintptr_t : unsigned long int 无符号长整型，与平台下指针大小相等
typedef uintptr_t SpanLength;


static const size_t NLISTS = 184;           //ThreadCache中freelist哈希表和CentralCache中spanlist哈希表的长度
static const size_t MaxBytes = 64 * 1024;      //ThreadCache中freelist哈希表中链表最大连接的内存大小
static const size_t NPAGES = 129;           //pagecache中哈希表的长度
static const size_t PAGESHIFT = 12;         //一页为多少字节，32位平台下为4k，即2^12
#ifdef _WIN32
	//32位平台下地址是32位，size_t可以表示
	typedef size_t ADDRES;
#else
	typedef unsigned long long ADDRES;      //64位平台下  [0,2^64)表示地址
#endif
class SizeClass {
public:
	//[1 - 128字节]                按8字节对齐           用freelist哈希表中[0-16)下标对应的链表组织
	//[129 - 1024字节]             按16字节对齐          用freelist哈希表中[16-72)下标对应的链表组织
	//[1025 - 8*1024字节]		   按128字节对齐		 用freelist哈希表中[72-128)下标对应的链表组织
	//[8*1024+1 - 64*1024字节]     按1024字节对齐        用freelist哈希表中[128-184)下标对应的链表组织
	
	//计算要申请size大小的内存在freelist哈希表中的下标
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
		//(size + 对齐数 - 1) / 对齐数 - 1 
		return ((size + (1 << shift) - 1) >> shift) - 1;
	}

	//根据请求内存块的大小求出ThreadCache向CentralCache要内存块时的门限个数
	static size_t NumMoveSize(size_t size) {
		if (size == 0)
			return 0;
		//为什么要用64k除？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？
		int num = MaxBytes / size;
		//小对象一次获取批量多，大对象少
		if (num < 2)
			num = 2;
		else if (num > 512)
			num = 512;
		return num;
	}

	//CentralCache向pagechche要几页的span
	static size_t NumMovePage(size_t size) {
		//threadcache一次向centralcache要NumMoveSize(size)个小内存块
		size_t num = NumMoveSize(size);
		//则centralcache向pagechche要span时至少要NumMoveSize(size)*size字节
		size_t spansize = num * size;
		//将需要的字节数除以一页包含的字节数就是需要的页数，右移即除以2的PAGESHIFT次方
		size_t npage = spansize >> PAGESHIFT;
		//如果所需小于1页，至少分配1页
		if (npage == 0)
			npage = 1;

		return npage;
	}

	//将内存块对齐
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
		//大于64k的内存块，按一页的大小对齐
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
	//将还回来的内存块头插进freelist链表中
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
	//将表头内存块分配出去
	void* Pop() {
		void* obj = head;
		head = *(void**)head;
		size--;
		return obj;
	}
	//链表过长释放内存时，取出该链中的前n个
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
	size_t maxsize = 1;          //threadcache向centralcache要内存块时的慢启动
	size_t size = 0;             //freelist中内存块的个数
}; 






struct Span {
	void* list = nullptr;     //把span中的内存分成小块链接起来，便于回收
	size_t usecount = 0;      //使用计数，为0表示该span所有小内存块都在
	size_t objsize;           //该span中每个小内存块的大小
	
	PageId startID = 0;         //该span的起始页号
	SpanLength pagenum = 0;    //该span从起始页开始，后面页的数量
	
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
	//这里的删除实际是把一个span从spanlist的连接关系去掉，而不是释放掉这块内存
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
	// brk mmap等
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
	// sbrk unmmap等
#endif
}