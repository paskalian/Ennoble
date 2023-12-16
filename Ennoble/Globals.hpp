#include <iostream>
#include <vector>

#define RGlobals Globals::Get()

struct ModuleInfo
{
	std::string Name;
	uintptr_t Address;
	uintptr_t Size;
	uintptr_t TextSecStart;
	uintptr_t TextSecEnd;
	uintptr_t DataSecStart;
	uintptr_t DataSecEnd;
	uintptr_t RDataSecStart;
	uintptr_t RDataSecEnd;
};

struct OffsetInfo
{
	std::string OffsetName;
	uintptr_t Offset;
};

class Globals
{
private:
	Globals() {};
	~Globals() {};
	Globals(const Globals&) = delete;
	Globals& operator= (const Globals&) = delete;
public:
	static Globals& Get() { static Globals g; return g; };

	void Start();
	bool Init();
	bool Execute();
	void* FindPattern(uintptr_t Start, const std::vector<uint8_t>& Pattern, size_t SearchLength);
	std::vector<uint8_t> GetActualBytes(std::string Pattern);
	std::vector<std::string> GetRuntimeClassname(void* pClassInstance);
	std::vector<ModuleInfo> Modules;
};