#include "PE.h"
CPE::CPE() :m_dwFileSize(0), m_pFileBase(NULL),
m_dwFileAlign(0), m_dwMemAlign(0)
{
}

DWORD CPE::RVA2OffSet(DWORD dwRVA, PIMAGE_NT_HEADERS64  pNt)
{
	DWORD dwOffset = 0;
	// 1. ��ȡ��һ�����νṹ��
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
	// 2. ��ȡ��������
	DWORD dwSectionCount = pNt->FileHeader.NumberOfSections;
	// 3. ����������Ϣ��
	for (DWORD i = 0; i < dwSectionCount; i++)
	{
		// 4. ƥ��RVA���ڵ�����
		if (dwRVA >= pSection[i].VirtualAddress &&
			dwRVA < (pSection[i].VirtualAddress + pSection[i].Misc.VirtualSize)
			)
		{   // �����Ӧ���ļ�ƫ��
			dwOffset = dwRVA - pSection[i].VirtualAddress +
				pSection[i].PointerToRawData;
			return dwOffset;
		}
	}
	return dwOffset;
}

BOOL CPE::InitPE(CString strPath)
{
	m_objFile.Open(strPath, CFile::modeRead);
	m_dwFileSize = (DWORD)m_objFile.GetLength();
	m_pFileBase = new BYTE[m_dwFileSize];
	if (m_objFile.Read(m_pFileBase, m_dwFileSize))
	{
		// 1. ��ȡDOSͷ
		PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)m_pFileBase;
		// 2. ��ȡNTͷ
		m_pNT = (PIMAGE_NT_HEADERS64)((ULONGLONG)m_pFileBase + pDos->e_lfanew);
		// 3. ��ȡ�ļ����롢�ڴ�������Ϣ
		m_dwFileAlign = m_pNT->OptionalHeader.FileAlignment;
		m_dwMemAlign = m_pNT->OptionalHeader.SectionAlignment;
		m_dwImageBase = m_pNT->OptionalHeader.ImageBase;
		m_dwOEP = m_pNT->OptionalHeader.AddressOfEntryPoint;
		m_dwCodeBase = m_pNT->OptionalHeader.BaseOfCode;
		m_dwCodeSize = m_pNT->OptionalHeader.SizeOfCode;

		// 3. ��ȡ���һ�����κ�ĵ�ַ
		PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(m_pNT);//.text
		m_pLastSection = &pSection[m_pNT->FileHeader.NumberOfSections - 1];
		ImportTable = GetImportTableAddress(m_pFileBase);

		// 4. ��ȡ�¼����ε���ʼRVA,  �ڴ�4K����
		DWORD dwVirtualSize = m_pLastSection->Misc.VirtualSize;
		if (dwVirtualSize % m_dwMemAlign)
			dwVirtualSize = (dwVirtualSize / m_dwMemAlign + 1) * m_dwMemAlign;
		else
			dwVirtualSize = (dwVirtualSize / m_dwMemAlign) * m_dwMemAlign;

		m_dwNewSectionRVA = m_pLastSection->VirtualAddress + dwVirtualSize;
		return TRUE;
	}
	return FALSE;
}
BOOL CPE::addimport(char* newtable, DWORD dwSize)
{
	memcpy(newtable, (PULONGLONG)ImportTable, Importsize);
	memset((void*)ImportTable, 0, Importsize);
	// 1 �޸��ļ�ͷ�е���������
	m_pNT->FileHeader.NumberOfSections++;
	m_pLastSection += 1;
	// 2 �������α���
	memset(m_pLastSection, 0, sizeof(IMAGE_SECTION_HEADER));
	strcpy_s((char*)m_pLastSection->Name, IMAGE_SIZEOF_SHORT_NAME, ".coleak");
	DWORD dwVirtualSize = 0; // ���������С
	DWORD dwSizeOfRawData = 0; // �����ļ���С
	DWORD packersize = 0;
	DWORD dwSizeOfImage = m_pNT->OptionalHeader.SizeOfImage;
	{
		if (dwSizeOfImage % m_dwMemAlign)
			dwSizeOfImage = (dwSizeOfImage / m_dwMemAlign + 1) * m_dwMemAlign;
		else
			dwSizeOfImage = (dwSizeOfImage / m_dwMemAlign) * m_dwMemAlign;

		if (Importsize % m_dwMemAlign)
			dwVirtualSize = (Importsize / m_dwMemAlign + 1) * m_dwMemAlign;
		else
			dwVirtualSize = (Importsize / m_dwMemAlign) * m_dwMemAlign;

		if (Importsize % m_dwFileAlign)
			dwSizeOfRawData = (Importsize / m_dwFileAlign + 1) * m_dwFileAlign;
		else
			dwSizeOfRawData = (Importsize / m_dwFileAlign) * m_dwFileAlign;
	}

	if (dwSize % m_dwFileAlign)
		packersize = (dwSize / m_dwFileAlign + 1) * m_dwFileAlign;
	else
		packersize = (dwSize / m_dwFileAlign) * m_dwFileAlign;

	m_pLastSection->VirtualAddress = dwSizeOfImage;
	m_pLastSection->PointerToRawData = m_dwFileSize + packersize;
	m_pLastSection->SizeOfRawData = dwSizeOfRawData;
	m_pLastSection->Misc.VirtualSize = dwVirtualSize;
	m_pLastSection->Characteristics = 0XE0000040;

	//IMAGE_DATA_DIRECTORY& importDir = m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	//importDir.VirtualAddress = 0;
	//importDir.Size = 0;
	// 3 �����ļ���С,�����ļ�����Ӵ���
	m_pNT->OptionalHeader.SizeOfImage = dwSizeOfImage + dwVirtualSize;
	return TRUE;
}


ULONGLONG CPE::AddSection(LPBYTE pBuffer, DWORD dwSize, PCHAR pszSectionName)
{
	// 1 �޸��ļ�ͷ�е���������
	m_pNT->FileHeader.NumberOfSections++;
	m_pLastSection += 1;
	// 2 �������α���
	memset(m_pLastSection, 0, sizeof(IMAGE_SECTION_HEADER));
	strcpy_s((char*)m_pLastSection->Name, IMAGE_SIZEOF_SHORT_NAME, pszSectionName);

	DWORD dwVirtualSize = 0; // ���������С
	DWORD dwSizeOfRawData = 0; // �����ļ���С
	DWORD dwSizeOfImage = m_pNT->OptionalHeader.SizeOfImage;
	{
		if (dwSizeOfImage % m_dwMemAlign)
			dwSizeOfImage = (dwSizeOfImage / m_dwMemAlign + 1) * m_dwMemAlign;
		else
			dwSizeOfImage = (dwSizeOfImage / m_dwMemAlign) * m_dwMemAlign;

		if (dwSize % m_dwMemAlign)
			dwVirtualSize = (dwSize / m_dwMemAlign + 1) * m_dwMemAlign;
		else
			dwVirtualSize = (dwSize / m_dwMemAlign) * m_dwMemAlign;

		if (dwSize % m_dwFileAlign)
			dwSizeOfRawData = (dwSize / m_dwFileAlign + 1) * m_dwFileAlign;
		else
			dwSizeOfRawData = (dwSize / m_dwFileAlign) * m_dwFileAlign;
	}

	m_pLastSection->VirtualAddress = dwSizeOfImage;
	m_pLastSection->PointerToRawData = m_dwFileSize;
	m_pLastSection->SizeOfRawData = dwSizeOfRawData;
	m_pLastSection->Misc.VirtualSize = dwVirtualSize;
	m_pLastSection->Characteristics = 0XE0000040;

	// 3 �����ļ���С,�����ļ�����Ӵ���
	m_pNT->OptionalHeader.SizeOfImage = dwSizeOfImage + dwVirtualSize;
	m_pNT->OptionalHeader.AddressOfEntryPoint = m_dwNewOEP + m_pLastSection->VirtualAddress;

	char* newtable = new char[Importsize];
	addimport(newtable, dwSize);
	int append = 0;
	if (Importsize <= m_dwFileAlign)
		append = m_dwFileAlign - Importsize;
	else
		append = m_dwFileAlign * ((int)Importsize / m_dwFileAlign + 1) - Importsize;

	char* appendbuf = new char[append];
	memset(appendbuf, 0, append);
	// 3.1 ��������ļ�·��
	CString strPath = m_objFile.GetFilePath();
	TCHAR szOutPath[MAX_PATH] = { 0 };
	LPWSTR strSuffix = PathFindExtension(strPath);                     // ��ȡ�ļ��ĺ�׺��
	wcsncpy_s(szOutPath, MAX_PATH, strPath, wcslen(strPath)); // ����Ŀ���ļ�·����szOutPath
	PathRemoveExtension(szOutPath);                                         // ��szOutPath�б���·���ĺ�׺��ȥ��
	wcscat_s(szOutPath, MAX_PATH, L"_Pack");                       // ��·����󸽼ӡ�_15Pack��
	wcscat_s(szOutPath, MAX_PATH, strSuffix);                           // ��·����󸽼Ӹոձ���ĺ�׺��

	// 3.2 �����ļ�
	CFile objFile(szOutPath, CFile::modeCreate | CFile::modeReadWrite);
	objFile.Write(m_pFileBase, m_dwFileSize);
	objFile.SeekToEnd();
	objFile.Write(pBuffer, dwSize);
	objFile.SeekToEnd();
	objFile.Write(newtable, Importsize);
	objFile.SeekToEnd();
	objFile.Write(appendbuf, append);
	return (ULONGLONG)m_pLastSection - (ULONGLONG)m_pFileBase + m_dwImageBase;
}

// ���ڴ����ض�λStub
void CPE::FixReloc(PBYTE lpImage, DWORD dwCodeRVA)//dllbase,dll.text va,NewSectionRVA
{
	// 1. ��ȡDOSͷ
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)lpImage;
	// 2. ��ȡNTͷ
	PIMAGE_NT_HEADERS64  pNt = (PIMAGE_NT_HEADERS64)((ULONGLONG)lpImage + pDos->e_lfanew);
	// 3. ��ȡ����Ŀ¼��
	PIMAGE_DATA_DIRECTORY pRelocDir = &(pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
	// 4. ��ȡ�ض�λĿ¼
	PIMAGE_BASE_RELOCATION pReloc = (PIMAGE_BASE_RELOCATION)((ULONGLONG)lpImage + pRelocDir->VirtualAddress);
	typedef struct {
		WORD Offset : 12;          // (1) ��СΪ12Bit���ض�λƫ��
		WORD Type : 4;             // (2) ��СΪ4Bit���ض�λ��Ϣ����ֵ
	}TypeOffset, * PTypeOffset;	   // ����ṹ����A1Pass�ܽ��

	// ѭ����ȡÿһ��MAGE_BASE_RELOCATION�ṹ���ض�λ��Ϣ
	while (pReloc->VirtualAddress)
	{
		PTypeOffset pTypeOffset = (PTypeOffset)(pReloc + 1);
		ULONGLONG dwSize = sizeof(IMAGE_BASE_RELOCATION);
		ULONGLONG dwCount = (pReloc->SizeOfBlock - dwSize) / 2;
		for (ULONGLONG i = 0; i < dwCount; i++)
		{
			if (*(PULONGLONG)(&pTypeOffset[i]) == NULL)
				break;
			ULONGLONG dwRVA = pReloc->VirtualAddress + pTypeOffset[i].Offset;
			PULONGLONG pRelocAddr = (PULONGLONG)((ULONGLONG)lpImage + dwRVA);//
			// �޸��ض�λ��Ϣ   ��ʽ����Ҫ�޸��ĵ�ַ-ԭӳ���ַ-ԭ���λ�ַ+�����λ�ַ+��ӳ���ַ
			ULONGLONG dwRelocCode = *pRelocAddr - pNt->OptionalHeader.ImageBase - pNt->OptionalHeader.BaseOfCode +
				dwCodeRVA + m_dwImageBase;
			*pRelocAddr = dwRelocCode;
		}
		pReloc = (PIMAGE_BASE_RELOCATION)((ULONGLONG)pReloc + pReloc->SizeOfBlock);
	}
}

void CPE::SetNewOEP(DWORD dwOEP)
{
	m_dwNewOEP = dwOEP;
}

void CPE::ClearBundleImport()
{
	// 1. ��ȡDOSͷ
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)m_pFileBase;
	// 2. ��ȡNTͷ
	PIMAGE_NT_HEADERS64  pNt = (PIMAGE_NT_HEADERS64)((ULONGLONG)m_pFileBase + pDos->e_lfanew);
	// 3. ��ȡ����Ŀ¼��
	PIMAGE_DATA_DIRECTORY pDir = pNt->OptionalHeader.DataDirectory;
	// 4. ��հ��������
	ZeroMemory(&(pDir[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT]), sizeof(IMAGE_DATA_DIRECTORY));
}

void CPE::ClearRandBase()
{
	m_pNT->OptionalHeader.DllCharacteristics &= ~0x0040;
}


DWORD CPE::Gettextsize(PBYTE lpImage)
{
	// 1. ��ȡDOSͷ
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)lpImage;
	// 2. ��ȡNTͷ
	PIMAGE_NT_HEADERS64  pNt = (PIMAGE_NT_HEADERS64)((ULONGLONG)lpImage + pDos->e_lfanew);
	// 3. ��ȡ��һ�����νṹ��
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
	DWORD dwSize = pSection[0].SizeOfRawData;
	return dwSize;
}


DWORD CPE::GetSectionData(PBYTE lpImage, PBYTE& lpBuffer, DWORD& dwCodeBaseRVA)
{
	// 1. ��ȡDOSͷ
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)lpImage;
	// 2. ��ȡNTͷ
	PIMAGE_NT_HEADERS64  pNt = (PIMAGE_NT_HEADERS64)((ULONGLONG)lpImage + pDos->e_lfanew);
	// 3. ��ȡ��һ�����νṹ��
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
	DWORD dwSize = pSection[0].SizeOfRawData;

	ULONGLONG dwCodeAddr = (ULONGLONG)lpImage + pSection[0].VirtualAddress;
	lpBuffer = (PBYTE)dwCodeAddr;
	dwCodeBaseRVA = pSection[0].VirtualAddress;
	return dwSize;
}

void CPE::encryptCode(PGLOBAL_PARAM g_stcParam)
{
	DWORD dwVirtualAddr = m_dwCodeBase;
	DWORD dwOffset = RVA2OffSet(m_dwCodeBase, m_pNT);
	PBYTE pBase = (PBYTE)((ULONGLONG)m_pFileBase + dwOffset);
	BYTE j;
	int len = strlen(g_stcParam->pass);
	for (DWORD i = 0; i < g_stcParam->dwCodeSize; i++)
	{
		j = (BYTE)g_stcParam->pass[i % len] + (BYTE)i;
		pBase[i] ^= j;
	}
}

void CPE::hideimporttable(int& Size, int& VirtualAddress, int& viradd, int& virsize)
{
	IMAGE_DATA_DIRECTORY& importDir = m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	DWORD memali = m_pNT->OptionalHeader.SectionAlignment;
	Size = importDir.Size;
	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(m_pNT);
	int m_dwSectionNum = m_pNT->FileHeader.NumberOfSections;
	int dwVirtualSize = 0;
	for (DWORD i = 0; i < m_dwSectionNum; i++, pSectionHeader++)
	{
		if (pSectionHeader->Misc.VirtualSize % m_dwMemAlign)
			dwVirtualSize = (pSectionHeader->Misc.VirtualSize / m_dwMemAlign + 1) * m_dwMemAlign;
		else
			dwVirtualSize = (pSectionHeader->Misc.VirtualSize / m_dwMemAlign) * m_dwMemAlign;

		if (importDir.VirtualAddress >= pSectionHeader->VirtualAddress &&
			importDir.VirtualAddress <= pSectionHeader->VirtualAddress + dwVirtualSize)
		{
			//��������ε���ʼ��ַ�ʹ�С
			viradd = pSectionHeader->VirtualAddress;
			virsize = dwVirtualSize;
			break;
		}
	}
}

ULONGLONG CPE::GetImportTableAddress(LPVOID lpFileBase)
{
	// 1. ��ȡDOSͷ
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		std::cerr << "Invalid DOS signature." << std::endl;
		return 0;
	}

	// 2. ��ȡNTͷ
	PIMAGE_NT_HEADERS64 pNtHeaders = (PIMAGE_NT_HEADERS64)((ULONGLONG)lpFileBase + pDosHeader->e_lfanew);
	if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
		return 0;
	}

	// 3. ��ȡ�����ĵ�ַ��RVA���ʹ�С
	DWORD importTableRVA = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	DWORD importTableSize = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

	if (importTableRVA == 0) {
		return 0;
	}

	// 4. ��RVAת��Ϊʵ���ļ���ַ
	// ��ȡ��һ������ͷ
	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
	for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
		DWORD sectionVA = pSectionHeader->VirtualAddress;
		DWORD sectionSize = pSectionHeader->Misc.VirtualSize;

		// ��鵼����Ƿ��ڵ�ǰ������
		if (importTableRVA >= sectionVA && importTableRVA < (sectionVA + sectionSize)) {
			// ת��������RVA���ļ�ƫ��
			ULONGLONG importTableOffset = (ULONGLONG)lpFileBase + importTableRVA - sectionVA + pSectionHeader->PointerToRawData;
			return importTableOffset;
		}
		pSectionHeader++;
	}
	return 0;
}

DWORD CPE::findtablerva(DWORD size)
{
	DWORD dwVirtualSize = 0;
	DWORD dwSizeOfImage = m_pNT->OptionalHeader.SizeOfImage;
	if (size % m_dwMemAlign)
		dwVirtualSize = (size / m_dwMemAlign + 1) * m_dwMemAlign;
	else
		dwVirtualSize = (size / m_dwMemAlign) * m_dwMemAlign;
	DWORD tmp = dwSizeOfImage + dwVirtualSize;
	return tmp;
}


BOOL CPE::Pack(CString strPath, const char* byXor)
{
	InitPE(strPath);
	BOOL bRet = FALSE;
	HMODULE hMod = LoadLibrary(L"x64/Release/Stub.dll");
	PGLOBAL_PARAM pstcParam = (PGLOBAL_PARAM)GetProcAddress(hMod, "g_stcParam");
	memcpy_s(pstcParam->pass, strlen(byXor), byXor, strlen(byXor));
	pstcParam->passlen = strlen(byXor);
	pstcParam->dwCodeSize = m_dwCodeSize;
	encryptCode(pstcParam);//m_dwCodeBase
	pstcParam->dwImageBase = m_dwImageBase;
	pstcParam->dwOEP = m_dwOEP;
	pstcParam->lpStartVA = (PBYTE)m_dwCodeBase;
	memset(pstcParam->pass, 0, strlen(byXor));
	int size = 0; int viradd = 0; int secadr = 0; int secsize = 0;
	hideimporttable(size, viradd, secadr, secsize);
	Importsize = size;
	pstcParam->Virrva = secadr;
	pstcParam->Virsize = secsize;
	pstcParam->importsize = size;
	pstcParam->importrva = findtablerva(Gettextsize((PBYTE)hMod));
	IMAGE_DATA_DIRECTORY& importDir = m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	importDir.VirtualAddress = 0;
	importDir.Size = 0;

	MODULEINFO modinfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hMod, &modinfo, sizeof(MODULEINFO));
	PBYTE  lpMod = new BYTE[modinfo.SizeOfImage];
	memcpy_s(lpMod, modinfo.SizeOfImage, hMod, modinfo.SizeOfImage);

	PBYTE pCodeSection = NULL;//dll->.text newva
	DWORD dwCodeBaseRVA = 0;//dll .text oldrva
	DWORD dwSize = GetSectionData(lpMod, pCodeSection, dwCodeBaseRVA);

	FixReloc(lpMod, m_dwNewSectionRVA);

	DWORD dwStubOEPRVA = (DWORD)pstcParam->dwStart - (DWORD)hMod;//start() rva
	DWORD dwNewOEP = dwStubOEPRVA - dwCodeBaseRVA; //start() rva -.text rva=��rva

	//StubOEP = dwStubOEPRVA - ԭRVA + �����ε�RVA;
	SetNewOEP(dwNewOEP);
	ClearRandBase();
	ClearBundleImport();//�����а��������ĳ���
	AddSection(pCodeSection, dwSize, (PCHAR)"packer");
	bRet = TRUE;
	delete lpMod;
	lpMod = NULL;
	return bRet;
}