#include "Globals.hpp"
#include <Windows.h>
#include <fstream>
#include <TlHelp32.h>
#include <string>

#include "Json.hpp"
using json = nlohmann::json;

void Globals::Start()
{
	AllocConsole();

	FILE* EmptyFile;
	freopen_s(&EmptyFile, "CONIN$", "r", stdin);
	freopen_s(&EmptyFile, "CONOUT$", "w", stdout);
	freopen_s(&EmptyFile, "CONOUT$", "w", stderr);
	
	printf(R"(
                         _     _      
                        | |   | |     
   ___ _ __  _ __   ___ | |__ | | ___ 
  / _ \ '_ \| '_ \ / _ \| '_ \| |/ _ \
 |  __/ | | | | | | (_) | |_) | |  __/
  \___|_| |_|_| |_|\___/|_.__/|_|\___|
                                      
	  made by github.com/paskalian                     
)");

	printf("\n\n");

	if (!Init())
	{
		printf("[!] Initialization failed.\n");
		return;
	}
	printf("[*] Initialized.\n");

	Execute();
}

bool Globals::Init()
{
	ModuleInfo NewMod = {};

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

	MODULEENTRY32 ModEntry = {};
	ModEntry.dwSize = sizeof(MODULEENTRY32);

	bool bNextFound = false;

	printf("Modules found: ");
	Module32First(hSnapshot, &ModEntry);
	do
	{
		NewMod.Name = ModEntry.szModule;
		NewMod.Address = (uintptr_t)ModEntry.modBaseAddr;

		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)NewMod.Address;
		PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(NewMod.Address + pDosHeader->e_lfanew);

		NewMod.Size = pNtHeaders->OptionalHeader.SizeOfImage;

		PIMAGE_SECTION_HEADER pFirstSection = IMAGE_FIRST_SECTION(pNtHeaders);
		for (unsigned int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
		{
			PIMAGE_SECTION_HEADER pIdxSection = &pFirstSection[i];

			if (_stricmp((const char*)pIdxSection->Name, ".text") == 0)
			{
				NewMod.TextSecStart = NewMod.Address + pIdxSection->VirtualAddress;
				NewMod.TextSecEnd = NewMod.TextSecStart + pIdxSection->Misc.VirtualSize;
			}
			else if (_stricmp((const char*)pIdxSection->Name, ".data") == 0)
			{
				NewMod.DataSecStart = NewMod.Address + pIdxSection->VirtualAddress;
				NewMod.DataSecEnd = NewMod.DataSecStart + pIdxSection->Misc.VirtualSize;
			}
		}

		Modules.push_back(NewMod);
		printf("%s", ModEntry.szModule);

		bNextFound = Module32Next(hSnapshot, &ModEntry);
		if (bNextFound)
			printf(", ");
	} while (bNextFound);
	printf("\n\n");

	return Modules.size() != 0;
}

bool Globals::Execute()
{
	while (true)
	{
		std::string ConfigPath;

		printf("[*] Enter config path: ");
		std::cin >> ConfigPath;

		try
		{
			std::ifstream JsonStream(ConfigPath.c_str());
			json JsonData = json::parse(JsonStream);

			std::string DumpFileName = "DumpResult-" + std::to_string(GetTickCount());
			std::ofstream OutFile(DumpFileName.c_str(), std::ofstream::binary);
		
			std::vector<OffsetInfo> FoundOffsets = {};

			for (auto& IdxClass : JsonData["classes"])
			{
				bool bStringFound = false;

				ModuleInfo FoundMod = {};
				std::string IdxClassName = IdxClass["class_name"];
				std::string IdxModName = IdxClass["module"];
				for (auto& IdxMod : Modules)
				{
					if (_stricmp(IdxModName.c_str(), IdxMod.Name.c_str()) == 0)
					{
						bStringFound = true;
						FoundMod = IdxMod;
					}
				}

				OutFile << "[" << IdxClassName.c_str() << "]\n";
	
				if (bStringFound)
				{
					FoundOffsets.clear();

					for (auto& IdxOffset : IdxClass["offsets"])
					{
						bool bOffsetFound = false;

						if (IdxOffset["rtti_search"]["enabled"])
						{
							uintptr_t Limit = IdxOffset["rtti_search"]["limit"];

							std::string ClassInstanceAddrString;

							printf("[*] Enter an instance to [%s]: ", IdxClassName.c_str());
							std::cin >> ClassInstanceAddrString;

							uintptr_t ClassInstanceAddr = (uintptr_t)strtoll(ClassInstanceAddrString.c_str(), NULL, 0);

							try
							{
								for (unsigned int i = sizeof(void*); i < Limit; i += sizeof(void*))
								{
									uintptr_t PotentialInnerClass = *(uintptr_t*)(ClassInstanceAddr + i);

									if (FoundMod.DataSecStart <= PotentialInnerClass && PotentialInnerClass <= FoundMod.DataSecEnd)
									{
										uintptr_t PotentialVTable = *(uintptr_t*)PotentialInnerClass;

										if (FoundMod.DataSecStart <= PotentialVTable && PotentialVTable <= FoundMod.DataSecEnd)
										{
											uintptr_t PotentialFunctionCode = *(uintptr_t*)PotentialVTable;

											if (FoundMod.TextSecStart <= PotentialFunctionCode && PotentialFunctionCode <= FoundMod.TextSecEnd)
											{
												std::vector<std::string> RttiList = GetRuntimeClassname((void*)PotentialInnerClass);
												for (auto& IdxRtti : RttiList)
													printf("RTTI: %s\n", IdxRtti.c_str());
												printf("\n");
											}
										}
									}
								}
							}
							catch (...)
							{
								printf("[!] Rtti search failed.\n");
							}
						}

						if (!bOffsetFound && IdxOffset["string_search"]["enabled"])
						{
							// Do string search.
						}

						if (!bOffsetFound)
						{
							OffsetInfo NewOff = {};
							NewOff.OffsetName = IdxOffset["name"];

							for (auto& IdxSignature : IdxOffset["signatures"])
							{
								std::string Pattern = IdxSignature["pattern"];
								intptr_t    Extra	= IdxSignature["extra"];
								uintptr_t   Read	= IdxSignature["read"];

								char* FoundAddress = (char*)FindPattern(FoundMod.Address, GetActualBytes(Pattern), FoundMod.Size);

								if (FoundAddress)
								{
									NewOff.Offset = Read == 1 ? *(FoundAddress + Extra) :
													Read == 2 ? *(uint16_t*)(FoundAddress + Extra) :
													Read == 4 ? *(uint32_t*)(FoundAddress + Extra) :
													0xDEADC0DE;

									if (NewOff.Offset != 0xDEADC0DE)
									{
										//printf("%X", NewOff.Offset);
										FoundOffsets.push_back(NewOff);
									}

									break;
								}
							}
						}
					}

					for (auto& IdxFoundOffset : FoundOffsets)
						OutFile << IdxFoundOffset.OffsetName << " - 0x" << std::hex << IdxFoundOffset.Offset << std::dec << "\n";
					OutFile << "\n";
				}
				else
				{
					printf("[!] - Class search - Invalid module: %s\n", IdxModName.c_str());
				}
			}

		/*	for (auto& IdxFunction : JsonData["functions"])
			{
				auto Function = std::find(Modules.begin(), Modules.end(), IdxFunction["module"]);
					
				if (Function != Modules.end())
				{
					;
				}
				else
					printf("[!] - Function search - Invalid module: %s\n", IdxFunction["module"]);
			}*/

			OutFile.close();
		}
		catch (...)
		{
			printf("[!] Invalid json file.\n");
		}
	}

	return true;
}

#ifdef _WIN64
std::vector<std::string> Globals::GetRuntimeClassname(void* pClassInstance)
{
	void** pVT = *(void***)pClassInstance;

	std::vector<std::string> ClassNames;

	if (pVT != (void*)0xCCCCCCCCCCCCCCCC)
	{
		void* pData = *(pVT - 1);

		uint32_t ClassHierarchyDescOff = *(uint32_t*)((uintptr_t)pData + 0x10);

		if (ClassHierarchyDescOff)
		{
			uint32_t ModBaseOff = *(uint32_t*)((uintptr_t)pData + 0x14);

			uintptr_t ModBase = (uintptr_t)pData - ModBaseOff;
			uintptr_t ClassHierarchyDesc = ModBase + ClassHierarchyDescOff;
			uint32_t ClassCount = *(uint32_t*)(ClassHierarchyDesc + 0x8);
			uint32_t BaseClassArrayOff = *(uint32_t*)(ClassHierarchyDesc + 0xC);

			if (BaseClassArrayOff)
			{
				uint32_t* BaseClassArray = (uint32_t*)(ModBase + BaseClassArrayOff);

				if (ClassCount > 0 && ClassCount < 25)
				{
					for (unsigned int i = 0; i < ClassCount; i++)
					{
						uint32_t BaseClassDescOff = BaseClassArray[i];

						if (BaseClassDescOff)
						{
							uint32_t TypeDescriptorOff = *(uint32_t*)(ModBase + BaseClassDescOff);
							if (TypeDescriptorOff)
							{
								uintptr_t TypeInfo = ModBase + TypeDescriptorOff;
								std::string TypeName = (const char*)TypeInfo + 0x14;

								if (TypeName.find_last_of("@@") != std::string::npos)
									TypeName.resize(TypeName.length() - 2);

								ClassNames.push_back(TypeName);
							}
						}
					}
				}


			}
		}
	}

	return ClassNames;
}
#else
std::vector<std::string> Globals::GetRuntimeClassname(void* pClassInstance)
{
	void** pVT = *(void***)pClassInstance;

	std::vector<std::string> ClassNames;

	if (pVT != (void*)0xCCCCCCCC)
	{
		void* pData = *(pVT - 1);

		uint32_t ClassHierarchyDesc = *(uint32_t*)((uintptr_t)pData + 0x10);
		uint32_t ClassCount = *(uint32_t*)(ClassHierarchyDesc + 0x8);

		if (ClassCount > 0 && ClassCount < 25)
		{
			uint32_t** BaseClassArray = *(uint32_t***)(ClassHierarchyDesc + 0xC);

			for (unsigned int i = 0; i < ClassCount; i++)
			{
				uint32_t TypeInfo = BaseClassArray[i][0];

				std::string TypeName = (const char*)TypeInfo + 0x0C;

				if (TypeName.find_last_of("@@") != std::string::npos)
					TypeName.resize(TypeName.length() - 2);

				ClassNames.push_back(TypeName);
			}
		}
	}

	return ClassNames;
}
#endif

void* Globals::FindPattern(uintptr_t Start, const std::vector<uint8_t>& Pattern, size_t SearchLength)
{
	for (size_t i = 0; i < SearchLength; ++i)
	{
		bool bPatternFound = true;
		for (size_t j = 0; j < Pattern.size(); ++j)
		{
			if (Pattern[j] != '\x90' && Pattern[j] != *(PCHAR)(Start + i + j))
			{
				bPatternFound = false;
				break;
			}
		}

		if (bPatternFound)
			return (void*)(Start + i);
	}

	return nullptr;
}

std::vector<uint8_t> Globals::GetActualBytes(std::string Pattern)
{
	size_t CurrentOffset = 0;

	std::vector<uint8_t> ReturnBytes;

	for (unsigned int i = 0; i < Pattern.length(); i+=3)
	{
		if (*(Pattern.c_str() + i) == '?')
		{
			ReturnBytes.push_back(0x90);
			continue;
		}

		ReturnBytes.push_back((uint8_t)strtol(Pattern.c_str() + i, NULL, 16));
	}

	return ReturnBytes;
}