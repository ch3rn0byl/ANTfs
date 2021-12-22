#pragma once
#include <Windows.h>

namespace privileges
{
	/// <summary>
	/// Checks to see if the callee is running with 
	/// Administrator privileges. 
	/// </summary>
	/// <returns>Returns true if successful.</returns>
	bool checkAdminPrivileges();
}


/// EOF