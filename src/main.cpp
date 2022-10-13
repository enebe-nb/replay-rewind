#include <windows.h>
#include "main.hpp"
#include "serializer.hpp"

// global variable initialization
std::filesystem::path modulePath;
std::unordered_map<void*, GOHeader> gameObject35C;

#ifdef _DEBUG
std::ofstream logging("replay-track.log");
#endif

namespace {
    void* (__fastcall* orig_BM_OnDestruct)(SokuLib::BattleManager*, int, int);
    // void (__fastcall* orig_BM_OnRender)(SokuLib::BattleManager*);
    int (__fastcall* orig_BM_OnBattleFrame)(SokuLib::BattleManager*);
    int (__fastcall* orig_BM_OnBattleEnd)(SokuLib::BattleManager*);

    std::deque<std::deque<uint8_t*>> battleState;
    std::deque<std::deque<uint8_t*>> compareState;
    bool isComparing = false;

    std::deque<uint8_t*> test;

    bool _SpawnObjectIsSet = false;
    typedef SokuLib::v2::GameObject* (__fastcall * SpawnObject_t) (void*, int, void*, void*, SokuLib::Action, float, float, SokuLib::Direction, unsigned char, int, size_t);
    SpawnObject_t orig_SpawnObject[2];
    template <SpawnObject_t& orig>
    static SokuLib::v2::GameObject* __fastcall SpawnObject(void* self, int unused, void* parent, void* owner, SokuLib::Action actionId, float x, float y, SokuLib::Direction dir, unsigned char layer, int data, size_t size) {
        auto object = orig(self, unused, parent, owner, actionId, x, y, dir, layer, data, size);
        gameObject35C[(void*)object] = {owner, actionId, x, y, dir, layer, size};
        return object;
    }

    int looplimiting = 0;
}

void* __fastcall BM_OnDestruct(SokuLib::BattleManager* self, int a, int b) {
    gameObject35C.clear();
    _SpawnObjectIsSet = false;
    return orig_BM_OnDestruct(self, a, b);
}

// void __fastcall BM_OnRender(SokuLib::BattleManager* self) {
//     return orig_BM_OnRender(self);
// }

int __fastcall BM_OnBattleFrame(SokuLib::BattleManager* self) {
    if (SokuLib::subMode != SokuLib::BATTLE_SUBMODE_REPLAY) return orig_BM_OnBattleFrame(self);

    if (!_SpawnObjectIsSet) {
        // TODO just hook all characters in the dll main
        DWORD old; VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old);
        _SpawnObjectIsSet = true;
        orig_SpawnObject[0] = ((SpawnObject_t**)&self->leftCharacterManager.objects)[0][1];
        ((SpawnObject_t**)&self->leftCharacterManager.objects)[0][1] = SpawnObject<orig_SpawnObject[0]>;
        orig_SpawnObject[1] = ((SpawnObject_t**)&self->rightCharacterManager.objects)[0][1];
        ((SpawnObject_t**)&self->rightCharacterManager.objects)[0][1] = SpawnObject<orig_SpawnObject[1]>;
        VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);
    }

    if (SokuLib::inputMgrs.input.horizontalAxis < 0) {
        if (SokuLib::inputMgrs.input.horizontalAxis%8 == -1 && battleState.size()) try {
            Serializer serializer(battleState.back());
            serializer.restore(SokuLib::getBattleMgr());
            battleState.pop_back();
        } catch (std::exception e) {
            MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
        }
        --self->frameCount; // revert change on calling function
        return 0;
    }

    if (self->frameCount % 60 == 0) try {
        Serializer serializer(battleState.emplace_back());
        serializer.serialize(SokuLib::getBattleMgr());
    } catch (std::exception e) {
        MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
    }

    // if (SokuLib::inputMgrs.input.verticalAxis == 1) ++looplimiting;
    // else if (SokuLib::inputMgrs.input.verticalAxis == -1) {
    //     --looplimiting;
    //     if (looplimiting < 0) looplimiting = 0;
    // }

    // if (self->frameCount % 60 == 0) {
    //     if (battleState.size() > looplimiting) try {
    //         Serializer serializer(battleState.back());
    //         serializer.restore(SokuLib::getBattleMgr());
    //         battleState.pop_back();
    //         --self->frameCount; // revert change on calling function
    //         return 0;
    //     } catch (std::exception e) {
    //         MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
    //     } else try {
    //         Serializer serializer(battleState.emplace_back());
    //         serializer.serialize(SokuLib::getBattleMgr());
    //     } catch (std::exception e) {
    //         MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
    //     }
    // }

    return orig_BM_OnBattleFrame(self);
}

int __fastcall BM_OnBattleEnd(SokuLib::BattleManager* self) {
    if (SokuLib::subMode != SokuLib::BATTLE_SUBMODE_REPLAY) return orig_BM_OnBattleFrame(self);

    if (SokuLib::inputMgrs.input.horizontalAxis%8 == -1 && battleState.size()) try {
        Serializer serializer(battleState.back());
        serializer.restore(SokuLib::getBattleMgr());
        battleState.pop_back();
    } catch (std::exception e) {
        MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
    }

    return 0;
}


static bool GetModulePath(HMODULE handle, std::filesystem::path& result) {
    // use wchar for better path handling
    std::wstring buffer;
    int len = MAX_PATH + 1;
    do {
        buffer.resize(len);
        len = GetModuleFileNameW(handle, buffer.data(), buffer.size());
    } while(len > buffer.size());

    if (len) result = std::filesystem::path(buffer.begin(), buffer.begin()+len).parent_path();
    return len;
}

const BYTE TARGET_HASH[16] = {0xdf, 0x35, 0xd1, 0xfb, 0xc7, 0xb5, 0x83, 0x31, 0x7a, 0xda, 0xbe, 0x8c, 0xd9, 0xf5, 0x3b, 0x2e};
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
    return ::memcmp(TARGET_HASH, hash, sizeof TARGET_HASH) == 0;
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
    GetModulePath(hMyModule, modulePath);
#ifdef _DEBUG
    logging << "modulePath: " << modulePath << std::endl;
#endif

    DWORD old;
    VirtualProtect((LPVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_WRITECOPY, &old);
    // TODO orig_setPaletteSelect = SokuLib::TamperNearCall(0x42087d, setPaletteSelect);
    VirtualProtect((LPVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);

    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old);
    // TODO orig_profileOnUpdate = SokuLib::TamperDword((DWORD)0x859878, profileOnUpdate);
    orig_BM_OnDestruct = SokuLib::TamperDword(SokuLib::ADDR_VTBL_BATTLE_MANAGER + 0x00, BM_OnDestruct);
    // orig_BM_OnRender = SokuLib::TamperDword(SokuLib::ADDR_VTBL_BATTLE_MANAGER + 0x38, BM_OnRender);
    orig_BM_OnBattleFrame = SokuLib::TamperDword(SokuLib::ADDR_VTBL_BATTLE_MANAGER + 0x18, BM_OnBattleFrame);
    orig_BM_OnBattleEnd = SokuLib::TamperDword(SokuLib::ADDR_VTBL_BATTLE_MANAGER + 0x20, BM_OnBattleEnd);
    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

    return true;
}

extern "C" __declspec(dllexport) void AtExit() {}

BOOL WINAPI DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return true;
}