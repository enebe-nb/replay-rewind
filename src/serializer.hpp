#pragma once

#include <SokuLib.hpp>
#include <deque>

class Serializer {
private:
    std::deque<uint8_t*>& chunkQueue;
    std::deque<uint8_t*>::iterator iter;
public:
    inline Serializer(std::deque<uint8_t*>& chunkQueue) : chunkQueue(chunkQueue), iter(chunkQueue.begin()) {}

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

    bool compare(uint8_t*, size_t);
    bool compare(SokuLib::BattleManager&);
    bool compare(SokuLib::v2::AnimationObject&);
    bool compare(SokuLib::v2::GameObjectBase&);
    bool compare(SokuLib::v2::GameObject&);
    bool compare(SokuLib::v2::Player&);
};