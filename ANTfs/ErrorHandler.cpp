#include "ErrorHandler.h"

_Use_decl_annotations_
ErrorHandler::ErrorHandler(DWORD dwError) :
	m_dwError(dwError),
	m_lpMessageBuffer(nullptr),
	m_ErrorAsStringW()
{
	LPCWSTR lpMessageString = nullptr;

	DWORD dwBufferLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		m_dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&m_lpMessageBuffer),
		NULL,
		NULL
	);

	lpMessageString = static_cast<LPCWSTR>(
		m_lpMessageBuffer
		);

	//
	// Copy the buffer into the std::wstring datatype.
	//
	m_ErrorAsStringW.assign(lpMessageString);

	//
	// FormatMessage inserts newlines into the string. It is possible to use more
	// formatting flags to take them out, but my brain is too smooth to do so. For
	// now, just use erase and remove to take them out rather than dicking with
	// formatting to get it right.
	//
	m_ErrorAsStringW.erase(
		std::remove(m_ErrorAsStringW.begin(), m_ErrorAsStringW.end(), '\n'),
		m_ErrorAsStringW.end()
	);
}

ErrorHandler::~ErrorHandler()
{
	if (m_lpMessageBuffer != nullptr)
	{
		LocalFree(m_lpMessageBuffer);
		m_lpMessageBuffer = nullptr;
	}
}

std::string
ErrorHandler::GetLastErrorAsStringA()
{
	std::vector<char> MessageBuffer(m_ErrorAsStringW.size());

	std::use_facet<std::ctype<wchar_t>>(std::locale{}).narrow(
		m_ErrorAsStringW.data(),
		m_ErrorAsStringW.data() + m_ErrorAsStringW.size(),
		'?',
		MessageBuffer.data()
	);

	return std::string(MessageBuffer.data(), MessageBuffer.size());
}

std::wstring
ErrorHandler::GetLastErrorAsStringW()
{
	return m_ErrorAsStringW;
}


// EOF