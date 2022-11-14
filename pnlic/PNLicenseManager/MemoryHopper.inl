
#pragma once
#include <vector>

class CMemoryHopper
{
	unsigned int m_LicenseType;

	std::vector<unsigned int*> m_hoppers;

	CMemoryHopper();
public:
	~CMemoryHopper();

	static CMemoryHopper& GetInstance();

	unsigned int *GetLicenseType();
	bool SetLicenseType(unsigned int newLicType);
};

CMemoryHopper::CMemoryHopper()
{
	m_hoppers.reserve(10);
}

CMemoryHopper::~CMemoryHopper()
{
	m_hoppers.clear();
}

CMemoryHopper& CMemoryHopper::GetInstance()
{
	static CMemoryHopper g_memoryHopper;
	return g_memoryHopper;
}

unsigned int *CMemoryHopper::GetLicenseType()
{
	unsigned int *t_pMemoryHopper = new unsigned int(m_LicenseType);
	m_hoppers.push_back(t_pMemoryHopper); 
	return t_pMemoryHopper;
}

bool CMemoryHopper::SetLicenseType(unsigned int newLicType)
{
	m_LicenseType = newLicType;

	return true;
}
