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
			else if (_stricmp((const char*)pIdxSection->Name, ".rdata") == 0)
			{
				NewMod.RDataSecStart = NewMod.Address + pIdxSection->VirtualAddress;
				NewMod.RDataSecEnd = NewMod.RDataSecStart + pIdxSection->Misc.VirtualSize;
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

			std::string DumpFileName = "DumpResult-" + std::to_string(GetTickCount64());
			std::ofstream OutFile(DumpFileName.c_str(), std::ofstream::binary);
		
			std::vector<OffsetInfo> FoundOffsets = {};

			OutFile << "=== CLASSES ===\n";
			for (auto& IdxClass : JsonData["classes"])
			{
				bool bStringFound = false;
				uintptr_t ClassInstanceAddr = 0;

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

						OffsetInfo NewOff = {};
						NewOff.OffsetName = IdxOffset["name"];

						if (IdxOffset["rtti_search"]["enabled"])
						{
							uintptr_t Limit = IdxOffset["rtti_search"]["limit"];

							if (!ClassInstanceAddr)
							{
								std::string ClassInstanceAddrString;

								printf("[*] Enter an instance to [%s]: ", IdxClassName.c_str());
								std::cin >> ClassInstanceAddrString;

								ClassInstanceAddr = (uintptr_t)strtoll(ClassInstanceAddrString.c_str(), NULL, 0);
							}

							try
							{
								MEMORY_BASIC_INFORMATION MemInfo = {};
								for (unsigned int i = sizeof(void*); i < Limit; i += sizeof(void*))
								{
									uintptr_t PotentialInnerClass = *(uintptr_t*)(ClassInstanceAddr + i);

									if (VirtualQuery((LPCVOID)PotentialInnerClass, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)))
									{
										if ((FoundMod.DataSecStart <= PotentialInnerClass && PotentialInnerClass <= FoundMod.DataSecEnd) ||
											(FoundMod.RDataSecStart <= PotentialInnerClass && PotentialInnerClass <= FoundMod.RDataSecEnd) ||
											(((MemInfo.Protect & PAGE_EXECUTE_READ) || ((MemInfo.Protect & PAGE_READWRITE)) || (MemInfo.Protect & PAGE_READONLY)) && PotentialInnerClass != UINTPTR_MAX))
										{
											uintptr_t PotentialVTable = *(uintptr_t*)PotentialInnerClass;

											if ((FoundMod.DataSecStart <= PotentialVTable && PotentialVTable <= FoundMod.DataSecEnd) ||
												(FoundMod.RDataSecStart <= PotentialVTable && PotentialVTable <= FoundMod.RDataSecEnd))
											{
												uintptr_t PotentialFunctionCode = *(uintptr_t*)PotentialVTable;

												if (FoundMod.TextSecStart <= PotentialFunctionCode && PotentialFunctionCode <= FoundMod.TextSecEnd)
												{
													std::vector<std::string> RttiList = GetRuntimeClassname((void*)PotentialInnerClass);

													if (_stricmp(RttiList[0].c_str(), NewOff.OffsetName.c_str()) == 0)
													{
														NewOff.Offset = i;
														bOffsetFound = true;
													}
												}
											}
										}
									}
								}
							}
							catch (...)
							{
								printf("[!] Rtti search failed.\n");
							}

							if (!bOffsetFound)
							{
								printf("[!] Offset %s couldn't be found by rtti.\n", NewOff.OffsetName.c_str());
							}
						}

						if (!bOffsetFound && IdxOffset["stringref_search"]["enabled"])
						{
							std::string SearchedString = IdxOffset["stringref_search"]["string"];
							std::string Pattern = IdxOffset["stringref_search"]["pattern"];
							intptr_t    Limit = IdxOffset["stringref_search"]["limit"];
							uintptr_t   Read = IdxOffset["stringref_search"]["read"];
							intptr_t    Extra = IdxOffset["stringref_search"]["extra"];
							bool bIsBackwards = Limit < 0;

							try
							{
								std::vector<uintptr_t> FoundRefs = FindStringRefs(SearchedString, FoundMod);
								for (const auto& IdxRef : FoundRefs)
								{
									if (bOffsetFound)
										break;

									unsigned int StartAddr = bIsBackwards ? (IdxRef - abs(Limit)) : IdxRef;
									std::vector<uint8_t> ActualBytes = GetActualBytes(Pattern);
									size_t SearchLength = abs(Limit);

									char* FoundAddress = (char*)FindPattern(StartAddr, ActualBytes, SearchLength);

									if (FoundAddress)
									{
										NewOff.Offset = Read == 1 ? *(FoundAddress + Extra) :
											Read == 2 ? *(uint16_t*)(FoundAddress + Extra) :
											Read == 4 ? *(uint32_t*)(FoundAddress + Extra) :
											0xDEADC0DE;

										if (NewOff.Offset != 0xDEADC0DE)
										{
											bOffsetFound = true;
										}
									}
								}
							}
							catch (...)
							{
								printf("[!] String search failed.\n");
							}

							if (!bOffsetFound)
							{
								printf("[!] Offset %s couldn't be found by string search.\n", NewOff.OffsetName.c_str());
							}
						}

						if (!bOffsetFound)
						{
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
										bOffsetFound = true;
									}

									break;
								}

								if (!bOffsetFound)
								{
									printf("[!] Offset %s couldn't be found by signature [%s].\n", NewOff.OffsetName.c_str(), Pattern.c_str());
								}
							}
						}

						if (!bOffsetFound)
						{
							printf("[!] Offset %s couldn't be found.\n", NewOff.OffsetName.c_str());
						}

						FoundOffsets.push_back(NewOff);
					}

					std::sort(FoundOffsets.begin(), FoundOffsets.end(), [](const OffsetInfo& a, const OffsetInfo& b)
						{
							return a.Offset < b.Offset;
						});

					printf("OFFSETS FOR %s\n", IdxClassName.c_str());
					for (auto& IdxFoundOffset : FoundOffsets)
					{
						OutFile << IdxFoundOffset.OffsetName << " - 0x" << std::hex << IdxFoundOffset.Offset << std::dec << "\n";
						printf("  - 0x%X %s\n", IdxFoundOffset.Offset, IdxFoundOffset.OffsetName.c_str());
					}
					OutFile << "\n";

					printf("[*] %s done.\n", IdxClassName.c_str());
				}
				else
				{
					printf("[!] - Class search - Invalid module: %s\n", IdxModName.c_str());
				}
			}

			OutFile << "=== FUNCTIONS ===\n";
			for (auto& IdxFunction : JsonData["functions"])
			{
				bool bStringFound = false;

				ModuleInfo FoundMod = {};
				std::string IdxFunctionName = IdxFunction["function_name"];
				std::string IdxModName = IdxFunction["module"];
				for (auto& IdxMod : Modules)
				{
					if (_stricmp(IdxModName.c_str(), IdxMod.Name.c_str()) == 0)
					{
						bStringFound = true;
						FoundMod = IdxMod;
					}
				}

				if (bStringFound)
				{
					FoundOffsets.clear();

					bool bFunctionFound = false;

					if (IdxFunction["stringref_search"]["enabled"])
					{
						std::string SearchedString = IdxFunction["stringref_search"]["string"];
						std::string Pattern = IdxFunction["stringref_search"]["pattern"];
						intptr_t    Limit = IdxFunction["stringref_search"]["limit"];
						bool bIsBackwards = Limit < 0;

						std::vector<uintptr_t> FoundRefs = FindStringRefs(SearchedString, FoundMod);
						for (auto& IdxRef : FoundRefs)
						{
							char* FoundAddress = (char*)FindPattern(bIsBackwards ? (IdxRef + Limit) : IdxRef, GetActualBytes(Pattern), abs(Limit));

							if (FoundAddress)
							{
								OutFile << IdxFunctionName.c_str() << " - 0x" << std::hex << ((uintptr_t)FoundAddress - FoundMod.Address) << std::dec << "\n";

								bFunctionFound = true;

								break;
							}
						}

						if (!bFunctionFound)
						{
							printf("[!] Function %s couldn't be found by string search.\n", IdxFunctionName.c_str());
						}
					}

					if (!bFunctionFound)
					{
						for (auto& IdxSignature : IdxFunction["signatures"])
						{
							std::string Pattern = IdxSignature["pattern"];
							intptr_t    Extra = IdxSignature["extra"];

							char* FoundAddress = (char*)FindPattern(FoundMod.Address, GetActualBytes(Pattern), FoundMod.Size);

							if (FoundAddress)
							{
								OutFile << IdxFunctionName.c_str() << " - 0x" << std::hex << ((uintptr_t)FoundAddress + Extra - FoundMod.Address) << std::dec << "\n";

								bFunctionFound = true;

								break;
							}

							if (!bFunctionFound)
							{
								printf("[!] Function %s couldn't be found by signature [%s].\n", IdxFunctionName.c_str(), Pattern.c_str());
							}
						}
					}

					OutFile << "\n";
				}
				else
				{
					printf("[!] - Function search - Invalid module: %s\n", IdxModName.c_str());
				}
			}

			printf("[*] Results dumped into %s\n", DumpFileName.c_str());

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
			if (Pattern[j] != (unsigned char)'\x90' && Pattern[j] != *(unsigned char*)(Start + i + j))
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

#define LEA_INSTRUCTION_SIZE 7
#define LEA_OFFSET 3

std::vector<uintptr_t> Globals::FindStringRefs(const std::string& String, const ModuleInfo& ModInfo)
{
	

	uintptr_t StartAddr = ModInfo.TextSecStart;
	uintptr_t EndAddr = ModInfo.TextSecEnd;
	uintptr_t SearchLength = EndAddr - StartAddr;
	uintptr_t Offset = 0;

	std::vector<uintptr_t> Refs;

#ifdef _WIN64
	uintptr_t FoundInstruction = NULL;

	// lea, [???]
	static std::vector<uint8_t> SearchBytes = { (unsigned char)'\x48', (unsigned char)'\x8D', (unsigned char)'\x90' };
#endif
	
#ifdef _WIN64
	do
	{
		FoundInstruction = (uintptr_t)FindPattern(StartAddr + Offset, SearchBytes, SearchLength - Offset);
		if (FoundInstruction)
		{

			// Offset becomes the first instruction AFTER lea.
			Offset = (FoundInstruction + LEA_INSTRUCTION_SIZE) - StartAddr;

			uint32_t PotentialStringOffset = *(uint32_t*)((uintptr_t)FoundInstruction + LEA_OFFSET);
			uintptr_t PotentialStringAddr = (uintptr_t)FoundInstruction + LEA_INSTRUCTION_SIZE + PotentialStringOffset;

			// Must be in .rdata section since we are searching for a static string.
			if (PotentialStringAddr >= ModInfo.RDataSecStart && PotentialStringAddr <= ModInfo.RDataSecEnd)
			{
				std::string PotentialString = (const char*)PotentialStringAddr;
				PotentialString.resize(MAX_PATH);

				if (_stricmp(PotentialString.c_str(), String.c_str()) == 0)
				{
					Refs.push_back(FoundInstruction + LEA_INSTRUCTION_SIZE);
				}
			}
		}
	} while (FoundInstruction);
#else
	// There's probably a better way to do this task but anyways.

	// Convert the given string into a byte array (which technically it already is but I can't cast a string into vector directly)
	std::vector<uint8_t> StringBytes;
	for (auto& IdxChar : String)
		StringBytes.push_back(IdxChar);

	const char* StringAddr = (const char*)FindPattern(ModInfo.Address, StringBytes, ModInfo.Size - 1);

	if (StringAddr)
	{
		// Convert the found string address into bytes so we can find it's references.
		std::vector<uint8_t> StringAddrBytes;
		for (unsigned int i = 0; i < sizeof(uint32_t); i++)
			StringAddrBytes.push_back(*(uint8_t*)((uintptr_t)&StringAddr + i));

		uintptr_t RefAddr = 0;
		do
		{
			RefAddr = (uintptr_t)FindPattern(ModInfo.Address + Offset, StringAddrBytes, ModInfo.Size - 1 - Offset);
			if (RefAddr)
			{
				Offset = RefAddr - ModInfo.Address + sizeof(uint32_t);

				Refs.push_back(RefAddr + sizeof(uint32_t));
			}
		} while (RefAddr);
	}
#endif

	return Refs;
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