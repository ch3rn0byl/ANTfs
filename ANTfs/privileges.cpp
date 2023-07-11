#include "privileges.h"

Privileges::Privileges() : 
	m_ErrorHandler(nullptr),
	m_hToken(INVALID_HANDLE_VALUE)
{
	BOOL bStatus = OpenProcessToken(
		GetCurrentProcess(),
		TOKEN_QUERY,
		&m_hToken
	);
	if (!bStatus)
	{
		m_ErrorHandler = std::make_unique<ErrorHandler>(GetLastError());
		throw std::exception(m_ErrorHandler->GetLastErrorAsStringA().c_str());
	}
}

Privileges::~Privileges()
{
	if (m_hToken != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hToken);
		m_hToken = INVALID_HANDLE_VALUE;
	}
}

BOOL Privileges::IsRunningAsAdmin()
{
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup = nullptr;

	BOOL bStatus = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, 
		&AdministratorsGroup
	);
	if (bStatus)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &bStatus))
		{
			bStatus = FALSE;
		}
	}

	if (AdministratorsGroup != nullptr)
	{
		FreeSid(AdministratorsGroup);
		AdministratorsGroup = nullptr;
	}

	return bStatus;
}


// EOF