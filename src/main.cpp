#include <windows.h>
#include "main.hpp"
#include "serializer.hpp"
#include <sstream>

// global variable initialization
std::unordered_map<void*, unsigned int> customDataSize;

#ifdef _DEBUG
std::ofstream logging("replay-track.log");
#define REPLAY_DEBUG
bool firstRun = true;
#endif

namespace {
    int (__fastcall* orig_BM_OnBattleFrame)(SokuLib::BattleManager*);
    int (__fastcall* orig_BM_OnBattleEnd)(SokuLib::BattleManager*);

    std::deque<std::stringstream> battleState;

    typedef SokuLib::v2::GameObject* (__fastcall * SpawnObject_t) (void*, int, void*, void*, SokuLib::Action, float, float, SokuLib::Direction, unsigned char, int, size_t);
    template <SpawnObject_t& orig>
    static SokuLib::v2::GameObject* __fastcall SpawnObject(void* self, int unused, void* parent, void* owner, SokuLib::Action actionId, float x, float y, SokuLib::Direction dir, unsigned char layer, int data, size_t size) {
        auto object = orig(self, unused, parent, owner, actionId, x, y, dir, layer, data, size);
        if (SokuLib::subMode == SokuLib::BATTLE_SUBMODE_REPLAY && SokuLib::mainMode == SokuLib::BATTLE_MODE_VSPLAYER) {
            customDataSize[(void*)object] = data ? size : 0;
        } return object;
    }

    std::pair<SpawnObject_t*, SpawnObject_t> orig_SpawnObject[] = {
        {(SpawnObject_t*)0x85bf1c, 0},
        {(SpawnObject_t*)0x85c34c, 0},
        {(SpawnObject_t*)0x85c6d4, 0},
        {(SpawnObject_t*)0x85c9c4, 0},
        {(SpawnObject_t*)0x85ccfc, 0},
        {(SpawnObject_t*)0x85cfac, 0},
        {(SpawnObject_t*)0x85d254, 0},
        {(SpawnObject_t*)0x85d4f4, 0},
        {(SpawnObject_t*)0x85d78c, 0},
        {(SpawnObject_t*)0x85da54, 0},
        {(SpawnObject_t*)0x85dd2c, 0},
        {(SpawnObject_t*)0x85e20c, 0},
        {(SpawnObject_t*)0x85df7c, 0},
        {(SpawnObject_t*)0x85e474, 0},
        {(SpawnObject_t*)0x85e6ec, 0},
        {(SpawnObject_t*)0x85efbc, 0},
        {(SpawnObject_t*)0x85ea84, 0},
        {(SpawnObject_t*)0x85ed0c, 0},
        {(SpawnObject_t*)0x85f5bc, 0},
        {(SpawnObject_t*)0x85f28c, 0},
        {(SpawnObject_t*)0x85f7f4, 0},
    };

    int looplimiting = 0;
}

int __fastcall BM_OnBattleFrame(SokuLib::BattleManager* self) {
    if (SokuLib::subMode != SokuLib::BATTLE_SUBMODE_REPLAY || SokuLib::mainMode != SokuLib::BATTLE_MODE_VSPLAYER) return orig_BM_OnBattleFrame(self);

    if (SokuLib::inputMgrs.input.horizontalAxis < 0) {
        if (SokuLib::inputMgrs.input.horizontalAxis%15 == -1 && battleState.size()) try {
            Restore parser(battleState.back());
            parser.run();
            parser.parse(*reinterpret_cast<SokuLib::Deque<unsigned short>*>(*(int*)((int)&SokuLib::inputMgr + 0x104) + 0x3c));
            battleState.pop_back();
        } catch (std::exception e) {
            MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
            battleState.pop_back();
        }
        --self->frameCount; // revert change on calling function
        SokuLib::camera.update();
        return 0;
    }

    if (self->frameCount % 2 == 1) try {
//#ifndef REPLAY_DEBUG
        // Serializer serializer(battleState.emplace_back());
        // serializer.serialize(SokuLib::getBattleMgr());
        Serialize parser(battleState.emplace_back());
        parser.run();
        parser.parse(*reinterpret_cast<SokuLib::Deque<unsigned short>*>(*(int*)((int)&SokuLib::inputMgr + 0x104) + 0x3c));
//#else
        // if (!battleState.size()) {
        //     Serializer serializer(battleState.emplace_back()); serializer.serialize(SokuLib::getBattleMgr());
        // } else if (firstRun) {
        //     firstRun = !firstRun;
        //     for (auto chunk : compareState) delete[] chunk; compareState.clear();
        //     { Serializer serializer(compareState); serializer.serialize(SokuLib::getBattleMgr()); }
        //     { Serializer serializer(battleState.back()); serializer.restore(SokuLib::getBattleMgr()); }
        // } else {
        //     firstRun = !firstRun;
        //     { Serializer serializer(compareState); serializer.compare(SokuLib::getBattleMgr()); }
        //     { Serializer serializer(battleState.emplace_back()); serializer.serialize(SokuLib::getBattleMgr()); }
        // } 
//#endif
    } catch (std::exception e) {
        MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
        battleState.pop_back();
    }

    // if (SokuLib::inputMgrs.input.verticalAxis == 1) ++looplimiting;
    // else if (SokuLib::inputMgrs.input.verticalAxis == -1) {
    //     --looplimiting;
    //     if (looplimiting < 0) looplimiting = 0;
    // }

    // if (self->frameCount % 120 == 1) {
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
    if (SokuLib::subMode != SokuLib::BATTLE_SUBMODE_REPLAY || SokuLib::mainMode != SokuLib::BATTLE_MODE_VSPLAYER) return orig_BM_OnBattleFrame(self);

    // if (SokuLib::inputMgrs.input.horizontalAxis%15 == -1 && battleState.size()) try {
    //     Serializer serializer(battleState.back());
    //     serializer.restore(SokuLib::getBattleMgr());
    //     battleState.pop_back();
    // } catch (std::exception e) {
    //     MessageBoxA(0, e.what(), "Replay Rewind", MB_OK);
    //     battleState.pop_back();
    // }

    --self->frameCount; // revert change on calling function
    return 0;
}


const BYTE TARGET_HASH[16] = {0xdf, 0x35, 0xd1, 0xfb, 0xc7, 0xb5, 0x83, 0x31, 0x7a, 0xda, 0xbe, 0x8c, 0xd9, 0xf5, 0x3b, 0x2e};
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
    return ::memcmp(TARGET_HASH, hash, sizeof TARGET_HASH) == 0;
}

template <std::size_t... I>
void setup_ObjectListHooks(std::index_sequence<I...>) {
    ((
        orig_SpawnObject[I].second = orig_SpawnObject[I].first[1],
        orig_SpawnObject[I].first[1] = SpawnObject<orig_SpawnObject[I].second>
    ), ...);
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
    DWORD old;
    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old);
    orig_BM_OnBattleFrame = SokuLib::TamperDword(SokuLib::ADDR_VTBL_BATTLE_MANAGER + 0x18, BM_OnBattleFrame);
    orig_BM_OnBattleEnd = SokuLib::TamperDword(SokuLib::ADDR_VTBL_BATTLE_MANAGER + 0x20, BM_OnBattleEnd);

    setup_ObjectListHooks(std::make_index_sequence<sizeof(orig_SpawnObject)/sizeof(orig_SpawnObject[0])>());
    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

    return true;
}

extern "C" __declspec(dllexport) void AtExit() {}

BOOL WINAPI DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return true;
}