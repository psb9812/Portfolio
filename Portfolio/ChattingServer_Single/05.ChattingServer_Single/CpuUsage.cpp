#include "CpuUsage.h"

//---------------------------------------------------------------------- 
// ������, Ȯ�δ�� ���μ��� �ڵ�. ���Է½� �ڱ� �ڽ�. 
//---------------------------------------------------------------------- 
CpuUsage::CpuUsage(HANDLE hProcess)
{
	//------------------------------------------------------------------ 
	// ���μ��� �ڵ� �Է��� ���ٸ� �ڱ� �ڽ��� �������... 
	//------------------------------------------------------------------ 
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		_hProcess = GetCurrentProcess();
	}

	//------------------------------------------------------------------ 
	// ���μ��� ������ Ȯ���Ѵ�. 
	// 
	// ���μ��� (exe) ����� ���� cpu ������ �����⸦ �Ͽ� ���� ������ ����. 
	//------------------------------------------------------------------ 
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);
	_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;



	_fProcessorTotal = 0;
	_fProcessorUser = 0;
	_fProcessorKernel = 0;

	_fProcessTotal = 0;
	_fProcessUser = 0;
	_fProcessKernel = 0;

	_ftProcessor_LastKernel.QuadPart = 0;
	_ftProcessor_LastUser.QuadPart = 0;
	_ftProcessor_LastIdle.QuadPart = 0;

	_ftProcess_LastUser.QuadPart = 0;
	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastTime.QuadPart = 0;

	UpdateCpuTime();
}

//////////////////////////////////////////////////////////////////////// 
// CPU ������ �����Ѵ�. 500ms ~ 1000ms ������ ȣ���� �����ѵ�. 
// 
// 
//////////////////////////////////////////////////////////////////////// 
void CpuUsage::UpdateCpuTime()
{
	//--------------------------------------------------------- 
	// ���μ��� ������ �����Ѵ�. 
	// 
	// ������ ��� ����ü�� FILETIME ������, ULARGE_INTEGER �� ������ �����Ƿ� �̸� �����. 
	// FILETIME ����ü�� 100 ���뼼���� ������ �ð� ������ ǥ���ϴ� ����ü��. 
	//--------------------------------------------------------- 
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;

	//--------------------------------------------------------- 
	// �ý��� ��� �ð��� ���Ѵ�. 
	// 
	// ���̵� Ÿ�� / Ŀ�� ��� Ÿ�� (���̵�����) / ���� ��� Ÿ�� 
	//--------------------------------------------------------- 
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
	{
		return;
	}

	// Ŀ�� Ÿ�ӿ��� ���̵� Ÿ���� ���Ե�. 
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcessor_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftProcessor_LastUser.QuadPart;
	ULONGLONG IdleDiff = Idle.QuadPart - _ftProcessor_LastIdle.QuadPart;

	ULONGLONG Total = KernelDiff + UserDiff;
	ULONGLONG TimeDiff;


	if (Total == 0)
	{
		_fProcessorUser = 0.0f;
		_fProcessorKernel = 0.0f;
		_fProcessorTotal = 0.0f;
	}
	else
	{
		// Ŀ�� Ÿ�ӿ� ���̵� Ÿ���� �����Ƿ� ���� ���. 
		_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
		_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
	}

	_ftProcessor_LastKernel = Kernel;
	_ftProcessor_LastUser = User;
	_ftProcessor_LastIdle = Idle;


	//--------------------------------------------------------- 
	// ������ ���μ��� ������ �����Ѵ�. 
	//--------------------------------------------------------- 
	ULARGE_INTEGER None;
	ULARGE_INTEGER NowTime;

	//--------------------------------------------------------- 
	// ������ 100 ���뼼���� ���� �ð��� ���Ѵ�. UTC �ð�. 
	// 
	// ���μ��� ���� �Ǵ��� ���� 
	// 
	// a = ���ð����� �ý��� �ð��� ����. (�׳� ������ ������ �ð�) 
	// b = ���μ����� CPU ��� �ð��� ����. 
	// 
	// a : 100 = b : ����   �������� ������ ����. 
	//--------------------------------------------------------- 

	//--------------------------------------------------------- 
	// ���� �ð��� �������� 100 ���뼼���� �ð��� ����,  
	//--------------------------------------------------------- 
	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);

	//--------------------------------------------------------- 
	// �ش� ���μ����� ����� �ð��� ����. 
	// 
	// �ι�°, ����°�� ����,���� �ð����� �̻��. 
	//--------------------------------------------------------- 
	GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

	//--------------------------------------------------------- 
	// ������ ����� ���μ��� �ð����� ���� ���ؼ� ������ ���� �ð��� �������� Ȯ��. 
	// 
	// �׸��� ���� ������ �ð����� ������ ������ ����. 
	//--------------------------------------------------------- 
	TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
	UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
	KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;

	Total = KernelDiff + UserDiff;


	_fProcessTotal = (float)(Total / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernel = (float)(KernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUser = (float)(UserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);

	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;

}