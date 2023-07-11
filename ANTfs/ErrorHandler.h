#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <string>
#include <vector>
#include <locale>

// <summary>
// This class takes in an error code from GetLastError and
// converts it into a readable string. The return types are:
//   1. std::string
//   2. std::wstring
// </summary>
class ErrorHandler
{
public:
	// <summary>
	// This constructor uses the value of GetLastError() to
	// convert into a readable error. 
	// </summary>
	// <param name="dwError"></param>
	ErrorHandler(
		_In_ DWORD dwError
	);

	~ErrorHandler();

	// <summary>
	// Returns the error as an ascii string.
	// </summary>
	// <returns>std::string</returns>
	std::string GetLastErrorAsStringA();

	// <summary>
	// Returns the error as a wide char string.
	// </summary>
	// <returns>std::wstring</returns>
	std::wstring GetLastErrorAsStringW();

protected:
	std::wstring m_ErrorAsStringW;

private:
	DWORD m_dwError;
	LPVOID m_lpMessageBuffer;

};


// EOF