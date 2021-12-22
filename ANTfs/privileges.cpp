#include "privileges.h"

bool privileges::checkAdminPrivileges()
{
	SID_IDENTIFIER_AUTHORITY NtAuthortiy = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup = nullptr;

	BOOL bRetVal = AllocateAndInitializeSid(
		&NtAuthortiy,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup
	);
	if (bRetVal)
	{
		if (!CheckTokenMembership(
			NULL, 
			AdministratorsGroup, 
			reinterpret_cast<PBOOL>(&bRetVal)
		))
		{
			bRetVal = false;
		}
		FreeSid(AdministratorsGroup);
		AdministratorsGroup = nullptr;
	}

	return bRetVal;
}


/// EOF