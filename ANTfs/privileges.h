#pragma once
#include <Windows.h>
#include <exception>
#include "ErrorHandler.h"

class Privileges
{
public:
	Privileges();
	~Privileges();

	BOOL IsRunningAsAdmin();

private:
	std::unique_ptr<ErrorHandler> m_ErrorHandler;
	HANDLE m_hToken;
};


// EOF