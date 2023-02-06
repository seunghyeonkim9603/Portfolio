#pragma once

class ProcessDataHelper final
{
public:
	ProcessDataHelper(const WCHAR* processName);
	~ProcessDataHelper();

	bool TryInit();
	void Update();
	double GetProcessCPUUsage() const;
	double GetProcessUserCPUUsage() const;
	long GetNonPagedPoolSize() const;
	long GetPagedPoolSize() const;
	long GetPageFault() const;
	long GetPrivateBytes() const;
	long GetNumRetransmittedPacket() const;

private:
	enum { MAX_PROCESS_NAME = 128 };

	PDH_HQUERY mQuery;
	PDH_HCOUNTER mUserCPUUsageCounter;
	PDH_HCOUNTER mTotalCPUUsageCounter;
	PDH_HCOUNTER mVirtualBytesCounter;
	PDH_HCOUNTER mPrivateBytesCounter;
	PDH_HCOUNTER mPagedPoolCounter;
	PDH_HCOUNTER mNonPagedPoolCounter;
	PDH_HCOUNTER mPageFaultCounter;
	PDH_HCOUNTER mRetransmittedCounter;

	double mTotalCPUUsage;
	double mUserCPUUsage;
	long mPrivateBytes;
	long mPagedPoolSize;
	long mNonPagedPoolSize;
	long mPageFault;
	long mRetransmitted;

	WCHAR mProcessName[MAX_PROCESS_NAME];
};