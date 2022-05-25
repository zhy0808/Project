#pragma once
#include "Common.h"

class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &sinst;
	}
	//��threadcacheһ���ڴ��
	size_t FetchRangeObj(void*& begin, void*& end, size_t n, size_t size);
	//��ĳ��spanlist�л�ȡһ������С�ڴ��span
	Span* GetOneSpane(SpanList list, size_t size);

	//threadcache�黹һ��С�ڴ�鵽centralcache��
	void ReleaseListToCentralCache(void* start, size_t size);
private:
	SpanList spanlist[NLISTS];

private:
	//���캯��˽��
	CentralCache() {

	}
	//C++11������
	CentralCache(const CentralCache&) = delete;

	//����ģʽ������ģʽ��������������̬���󣬽���Ĺ���Ϳ������캯������Ϊ˽�У��ҿ�������delete
	//��.cpp�ļ����������󣬱�֤����ֻ��һ������������CentralCache.obj�У������.h�ж��壬����ļ�������.h�ļ�ʱ���´���������
	static CentralCache sinst;
};

