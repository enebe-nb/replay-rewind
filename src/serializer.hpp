#pragma once

#include <SokuLib.hpp>
#include <deque>

class Serializer {
private:
    std::deque<uint8_t*>& chunkQueue;
public:
    inline Serializer(std::deque<uint8_t*>& chunkQueue) : chunkQueue(chunkQueue) {}

    bool serialize(uint8_t*, size_t);
    bool serialize(const SokuLib::BattleManager&);
    bool serialize(const SokuLib::v2::AnimationObject&);
    bool serialize(const SokuLib::v2::GameObjectBase&);
    bool serialize(const SokuLib::v2::GameObject&);
    bool serialize(const SokuLib::v2::Player&);

    bool restore(uint8_t*, size_t);
    bool restore(SokuLib::BattleManager&);
    bool restore(SokuLib::v2::AnimationObject&);
    bool restore(SokuLib::v2::GameObjectBase&);
    bool restore(SokuLib::v2::GameObject&);
    bool restore(SokuLib::v2::Player&);
};