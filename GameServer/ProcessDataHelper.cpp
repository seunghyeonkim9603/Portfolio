#include "stdafx.h"

ProcessDataHelper::ProcessDataHelper(const WCHAR* processName)
	: mTotalCPUUsage(0.0),
	mUserCPUUsage(0.0),
	mPrivateBytes(0),
	mPagedPoolSize(0),
	mNonPagedPoolSize(0),
	mPageFault(0),
	mRetransmitted(0)
{
	wcscpy_s(mProcessName, processName);
	PdhOpenQuery(NULL, NULL, &mQuery);
}

ProcessDataHelper::~ProcessDataHelper()
{
	PdhCloseQuery(mQuery);
}

bool ProcessDataHelper::TryInit()
{
	WCHAR szQuery[1024];

	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\%% Processor Time", mProcessName);
	PdhAddCounter(mQuery, szQuery, NULL, &mTotalCPUUsageCounter);
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\%% User Time", mProcessName);
	PdhAddCounter(mQuery, szQuery, NULL, &mUserCPUUsageCounter);
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Page Faults/sec", mProcessName);
	PdhAddCounter(mQuery, szQuery, NULL, &mPageFaultCounter);
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Pool Nonpaged Bytes", mProcessName);
	PdhAddCounter(mQuery, szQuery, NULL, &mNonPagedPoolCounter);
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Pool Paged Bytes", mProcessName);
	PdhAddCounter(mQuery, szQuery, NULL, &mPagedPoolCounter);
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Private Bytes", mProcessName);
	PdhAddCounter(mQuery, szQuery, NULL, &mPrivateBytesCounter);
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\TCPv4\\Segments Retransmitted/sec");
	PdhAddCounter(mQuery, szQuery, NULL, &mRetransmittedCounter);

	Update();

	return true;
}

void ProcessDataHelper::Update()
{
	PdhCollectQueryData(mQuery);

	PDH_FMT_COUNTERVALUE cpuTotalUsage;
	PDH_FMT_COUNTERVALUE cpuUserUsage;
	PDH_FMT_COUNTERVALUE nonPagedPoolSize;
	PDH_FMT_COUNTERVALUE pagedPoolSize;
	PDH_FMT_COUNTERVALUE pageFault;
	PDH_FMT_COUNTERVALUE privateBytes;
	PDH_FMT_COUNTERVALUE retransmitted;

	PdhGetFormattedCounterValue(mTotalCPUUsageCounter, PDH_FMT_DOUBLE, NULL, &cpuTotalUsage);
	PdhGetFormattedCounterValue(mUserCPUUsageCounter, PDH_FMT_DOUBLE, NULL, &cpuUserUsage);
	PdhGetFormattedCounterValue(mNonPagedPoolCounter, PDH_FMT_LONG, NULL, &nonPagedPoolSize);
	PdhGetFormattedCounterValue(mPagedPoolCounter, PDH_FMT_LONG, NULL, &pagedPoolSize);
	PdhGetFormattedCounterValue(mPageFaultCounter, PDH_FMT_LONG, NULL, &pageFault);
	PdhGetFormattedCounterValue(mPrivateBytesCounter, PDH_FMT_LONG, NULL, &privateBytes);
	PdhGetFormattedCounterValue(mRetransmittedCounter, PDH_FMT_LONG, NULL, &retransmitted);

	mTotalCPUUsage = cpuTotalUsage.doubleValue;
	mUserCPUUsage = cpuUserUsage.doubleValue;
	mNonPagedPoolSize = nonPagedPoolSize.longValue;
	mPagedPoolSize = pagedPoolSize.longValue;
	mPageFault = pageFault.longValue;
	mPrivateBytes = privateBytes.longValue / 1000000;
	mRetransmitted = retransmitted.longValue;
}

double ProcessDataHelper::GetProcessCPUUsage() const
{
	return mTotalCPUUsage;
}

double ProcessDataHelper::GetProcessUserCPUUsage() const
{
	return mUserCPUUsage;
}

long ProcessDataHelper::GetNonPagedPoolSize() const
{
	return mNonPagedPoolSize;
}

long ProcessDataHelper::GetPagedPoolSize() const
{
	return mPagedPoolSize;
}

long ProcessDataHelper::GetPageFault() const
{
	return mPageFault;
}

long ProcessDataHelper::GetPrivateBytes() const
{
	return mPrivateBytes;
}

long ProcessDataHelper::GetNumRetransmittedPacket() const
{
	return mRetransmitted;
}
