#include "serializer.hpp"
#include "main.hpp"

#define _offset(base, off) (uint8_t*)((int)&base + (off))

static inline uint8_t* createChunk(uint8_t* data, size_t size) {
    uint8_t* chunk = new uint8_t[size + 4];
    *(uint32_t*)chunk = size;
    memcpy(chunk + 4, data, size);
    return chunk;
}

static inline uint8_t* createChunk(size_t size) {
    uint8_t* chunk = new uint8_t[size];
    return chunk;
}

static inline void restoreChunk(uint8_t* chunk, uint8_t* data, size_t size) {
    if (size != *(uint32_t*)chunk) throw std::exception("Restore: Size mismatch");
    memcpy(data, chunk + 4, size);
    delete[] chunk;
}

static int inline GetPlayerExtraSize(SokuLib::Character character) {
    switch(character) {
        case SokuLib::CHARACTER_REMILIA:
        case SokuLib::CHARACTER_CHIRNO:
        case SokuLib::CHARACTER_MEILING:
            return 0x04;
        case SokuLib::CHARACTER_SUIKA:
        case SokuLib::CHARACTER_AYA:
        case SokuLib::CHARACTER_KOMACHI:
            return 0x08;
        case SokuLib::CHARACTER_SAKUYA:
        case SokuLib::CHARACTER_NAMAZU:
            return 0x10;
        case SokuLib::CHARACTER_PATCHOULI:
        case SokuLib::CHARACTER_UTSUHO:
        case SokuLib::CHARACTER_SUWAKO:
        case SokuLib::CHARACTER_IKU:
            return 0x18;
        case SokuLib::CHARACTER_MARISA:
        case SokuLib::CHARACTER_YUYUKO:
            return 0x1C;
        case SokuLib::CHARACTER_SANAE: return 0x20;
        case SokuLib::CHARACTER_UDONGE: return 0x28;
        case SokuLib::CHARACTER_REIMU: return 0x2C;
        case SokuLib::CHARACTER_ALICE: return 0x34;
        case SokuLib::CHARACTER_YUKARI: return 0x44;
        case SokuLib::CHARACTER_YOUMU: return 0x5C;
        case SokuLib::CHARACTER_TENSHI: return 0xA0;
    } return 0;
}

bool Serializer::serialize(uint8_t* data, size_t size) {
    chunkQueue.push_back(createChunk(data, size));
    return true;
}

bool Serializer::serialize(const SokuLib::BattleManager& data) {
    chunkQueue.push_back(createChunk(_offset(data, 0x004), 0x018));
    // TODO test
    chunkQueue.push_back(createChunk(_offset(data, 0x074), 0x018));
    // chunkQueue.push_back(createChunk(_offset(data, 0x074), 0x070));
    // chunkQueue.push_back(createChunk(_offset(data, 0x0F0), 0x010));
    // chunkQueue.push_back(createChunk(_offset(data, 0x22C), 0x05C));
    // chunkQueue.push_back(createChunk(_offset(data, 0x3B4), 0x080));
    // chunkQueue.push_back(createChunk(_offset(data, 0x468), 0x020));
    // chunkQueue.push_back(createChunk(_offset(data, 0x52C), 0x03C));
    // chunkQueue.push_back(createChunk(_offset(data, 0x84C), 0x010));
    // chunkQueue.push_back(createChunk(_offset(data, 0x868), 0x09C));

    serialize(*(SokuLib::v2::Player*)&data.leftCharacterManager);
    serialize(_offset(data, 0x890), GetPlayerExtraSize(((SokuLib::v2::Player*)&data.leftCharacterManager)->characterIndex));
    serialize(*(SokuLib::v2::Player*)&data.rightCharacterManager);
    serialize(_offset(data, 0x890), GetPlayerExtraSize(((SokuLib::v2::Player*)&data.rightCharacterManager)->characterIndex));

    // Random Seed
    serialize((uint8_t*)0x89b660, 0x9C0);
    serialize((uint8_t*)0x896b20, 0x004);

    { // Input queue
        SokuLib::Deque<unsigned short>& inputQueue = *reinterpret_cast<SokuLib::Deque<unsigned short>*>(*(int*)((int)&SokuLib::inputMgr + 0x104) + 0x3c);
        chunkQueue.push_back(createChunk(inputQueue.size()*2 + 4));
        uint8_t* chunk = chunkQueue.back();
        *(uint32_t*)chunk = inputQueue.size(); chunk += 4;
        for (auto val : inputQueue) { *(uint16_t*)chunk = val; chunk += 2; }
    }

    // Weather
    serialize((uint8_t*)0x8971c0, 0x10);

    return true;
}

bool Serializer::serialize(const SokuLib::v2::AnimationObject& data) {
    chunkQueue.push_back(createChunk(_offset(data, 0x0EC), 0x068));
    return true;
}

bool Serializer::serialize(const SokuLib::v2::GameObjectBase& data) {
    serialize((SokuLib::v2::AnimationObject&)data);
    chunkQueue.push_back(createChunk(_offset(data, 0x158), 0x01C));
    chunkQueue.push_back(createChunk(_offset(data, 0x184), 0x02C));
    // TODO check 1B0
    if (data.unknown1B0) throw std::runtime_error("Serializer: GameObjectBase+0x1B0 != null");
    return true;
}

bool Serializer::serialize(const SokuLib::v2::GameObject& data) {
    serialize((SokuLib::v2::GameObjectBase&)data);
    chunkQueue.push_back(createChunk(_offset(data, 0x34C), 0x004));
    chunkQueue.push_back(createChunk(_offset(data, 0x360), 0x03C));
    // TODO check 354/35C
    if (data.unknown354) throw std::runtime_error("Serializer: GameObject+0x354 != null");
    // chunkQueue.push_back(createChunk((uint8_t*)data.unknown35C, gameObject35C[(void*)&data].second * 4));
    if (data.parentB) throw std::runtime_error("Serializer: GameObject+0x3A0 != null");

    if (((SokuLib::v2::Player*)data.gameData.owner)->characterIndex == SokuLib::CHARACTER_ALICE) {
        chunkQueue.push_back(createChunk(_offset(data, 0x3AC), 0x004));
    }
    
    return true;
}

bool Serializer::serialize(const SokuLib::v2::Player& data) {
    serialize((SokuLib::v2::GameObjectBase&)data);
    chunkQueue.push_back(createChunk(_offset(data, 0x498), 0x0E4));

    {
        chunkQueue.push_back(createChunk(36 + data.deckData.queue.size()*2));
        uint8_t* chunk = chunkQueue.back();
        memcpy(chunk, _offset(data, 0x5C4), 32); chunk += 32;
        *(uint32_t*)chunk = data.deckData.queue.size(); chunk += 4;
        for (auto val : data.deckData.queue) { *(uint16_t*)chunk = val; chunk += 2; }
    }

    {
        chunkQueue.push_back(createChunk(data.handData.hand.size()*2 + data.handData.usedCards.size()*2 + 12));
        uint8_t* chunk = chunkQueue.back();
        memcpy(chunk, _offset(data, 0x5e4), 4); chunk += 4;
        *(uint32_t*)chunk = data.handData.hand.size(); chunk += 4;
        for (auto val : data.handData.hand) { *(uint16_t*)chunk = val.id; chunk += 2; }
        *(uint32_t*)chunk = data.handData.usedCards.size(); chunk += 4;
        for (auto val : data.handData.usedCards) { *(uint16_t*)chunk = val; chunk += 2; }
        // TODO hand restore: 0x469990
    }

    chunkQueue.push_back(createChunk(_offset(data, 0x6A4), 0x054));
    if (data.unknown714.unknown714.size()) throw std::runtime_error("Serializer: Player+0x714.size() > 0");
    chunkQueue.push_back(createChunk(_offset(data, 0x710), 0x1C));
    chunkQueue.push_back(createChunk(_offset(data, 0x740), 0x10));
    chunkQueue.push_back(createChunk(_offset(data, 0x774), 0x3C));
    {
        chunkQueue.push_back(createChunk(data.inputData.commandInputBuffer.size()*2 + 4));
        uint8_t* chunk = chunkQueue.back();
        *(uint32_t*)chunk = data.inputData.commandInputBuffer.size(); chunk += 4;
        for (auto val : data.inputData.commandInputBuffer) { *(uint16_t*)chunk = val; chunk += 2; }
    }
    chunkQueue.push_back(createChunk(_offset(data, 0x7C4), 0xCC));

    {
        size_t objectCount = data.objectList->VUnknown24()->size();
        chunkQueue.push_back(createChunk((uint8_t*)&objectCount, 4));
        for (auto object : *data.objectList->VUnknown24()) {
            auto& header = gameObject35C[(void*)object];
            chunkQueue.push_back(createChunk((uint8_t*)&header, sizeof(header)));
            chunkQueue.push_back(createChunk((uint8_t*)&object->unknown35C, header.size * 4));
            serialize(*object);
        }
    }

    return true;
}

bool Serializer::restore(uint8_t* data, size_t size) {
    restoreChunk(chunkQueue.front(), data, size); chunkQueue.pop_front();
    return true;
}

bool Serializer::restore(SokuLib::BattleManager& data) {
    restoreChunk(chunkQueue.front(), _offset(data, 0x004), 0x018); chunkQueue.pop_front();
    // TODO test
    restoreChunk(chunkQueue.front(), _offset(data, 0x074), 0x018); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x074), 0x070); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x0F0), 0x010); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x22C), 0x05C); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x3B4), 0x080); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x468), 0x020); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x52C), 0x03C); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x84C), 0x010); chunkQueue.pop_front();
    // restoreChunk(chunkQueue.front(), _offset(data, 0x868), 0x09C); chunkQueue.pop_front();

    restore(*(SokuLib::v2::Player*)&data.leftCharacterManager);
    restore(_offset(data, 0x890), GetPlayerExtraSize(((SokuLib::v2::Player*)&data.leftCharacterManager)->characterIndex));
    restore(*(SokuLib::v2::Player*)&data.rightCharacterManager);
    restore(_offset(data, 0x890), GetPlayerExtraSize(((SokuLib::v2::Player*)&data.rightCharacterManager)->characterIndex));

    // Random Seed
    restore((uint8_t*)0x89b660, 0x9C0);
    restore((uint8_t*)0x896b20, 0x004);

    { // Input queue
        SokuLib::Deque<unsigned short>& inputQueue = *reinterpret_cast<SokuLib::Deque<unsigned short>*>(*(int*)((int)&SokuLib::inputMgr + 0x104) + 0x3c);
        uint8_t* chunk = chunkQueue.front();
        uint32_t size = *(uint32_t*)chunk; chunk += 4;
        inputQueue.clear(); while (size--) { inputQueue.push_back(*(uint16_t*)chunk); chunk += 2; }
        delete[] chunkQueue.front(); chunkQueue.pop_front();
    }

    // Weather
    restore((uint8_t*)0x8971c0, 0x10);

    return true;
}

bool Serializer::restore(SokuLib::v2::AnimationObject& data) {
    restoreChunk(chunkQueue.front(), _offset(data, 0x0EC), 0x068); chunkQueue.pop_front();
    return true;
}

bool Serializer::restore(SokuLib::v2::GameObjectBase& data) {
    restore((SokuLib::v2::AnimationObject&)data);
    restoreChunk(chunkQueue.front(), _offset(data, 0x158), 0x01C); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x184), 0x02C); chunkQueue.pop_front();
    return true;
}

bool Serializer::restore(SokuLib::v2::GameObject& data) {
    restore((SokuLib::v2::GameObjectBase&)data);
    restoreChunk(chunkQueue.front(), _offset(data, 0x34C), 0x004); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x360), 0x03C); chunkQueue.pop_front();

    // { // unknown35C
    //     uint8_t* chunk = chunkQueue.front();
    //     uint32_t size = *(uint32_t*)chunk; chunk += 4;
    //     if (size) { data.unknown35C = SokuLib::NewFct(size); memcpy(data.unknown35C, chunk, size); }
    //     gameObject35C[(void*)&data].second = size / 4;
    //     delete[] chunkQueue.front(); chunkQueue.pop_front();
    // }

    if (((SokuLib::v2::Player*)data.gameData.owner)->characterIndex == SokuLib::CHARACTER_ALICE) {
        restoreChunk(chunkQueue.front(), _offset(data, 0x3AC), 0x004);
        chunkQueue.pop_front();
    }

    return true;
}

bool Serializer::restore(SokuLib::v2::Player& data) {
    restore((SokuLib::v2::GameObjectBase&)data);
    restoreChunk(chunkQueue.front(), _offset(data, 0x498), 0x0E4); chunkQueue.pop_front();

    {
        uint8_t* chunk = chunkQueue.front();
        memcpy(_offset(data, 0x5C4), chunk, 32); chunk += 32;
        uint32_t size = *(uint32_t*)chunk; chunk += 4;
        data.deckData.queue.clear(); while (size--) { data.deckData.queue.push_back(*(uint16_t*)chunk); chunk += 2; }
        delete[] chunkQueue.front(); chunkQueue.pop_front();
    }

    {
        uint8_t* chunk = chunkQueue.front();
        memcpy(_offset(data, 0x5e4), chunk, 4); chunk += 4;
        uint32_t size = *(uint32_t*)chunk; chunk += 4;
        data.handData.hand.clear(); while (size--) {
            const auto& cardInfo = data.deckData.cardById[*(uint16_t*)chunk];
            data.handData.hand.push_back({*(uint16_t*)chunk, cardInfo.type == 1 ? (unsigned short)1 : cardInfo.costOrSlot});
            if (cardInfo.texIdB) data.handData.hand.back().sprite.setTexture2(cardInfo.texIdB, 0, 0, *(int*)0x897004, *(int*)0x897000);
            chunk += 2;
        } size = *(uint32_t*)chunk; chunk += 4;
        data.handData.usedCards.clear(); while (size--) { data.handData.usedCards.push_back(*(uint16_t*)chunk); chunk += 2; }
        delete[] chunkQueue.front(); chunkQueue.pop_front();
    }

    restoreChunk(chunkQueue.front(), _offset(data, 0x6A4), 0x054); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x710), 0x01C); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x740), 0x010); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x774), 0x03C); chunkQueue.pop_front();
    {
        uint8_t* chunk = chunkQueue.front();
        uint32_t size = *(uint32_t*)chunk; chunk += 4;
        data.inputData.commandInputBuffer.clear(); while (size--) { data.inputData.commandInputBuffer.push_back(*(uint16_t*)chunk); chunk += 2; }
        delete[] chunkQueue.front(); chunkQueue.pop_front();
    }
    restoreChunk(chunkQueue.front(), _offset(data, 0x7C4), 0x0CC); chunkQueue.pop_front();

    {
        uint32_t size; restoreChunk(chunkQueue.front(), (uint8_t*)&size, 4); chunkQueue.pop_front();
        data.objectList->VUnknown08(); while (size--) {
            GOHeader header; restoreChunk(chunkQueue.front(), (uint8_t*)&header, sizeof(header)); chunkQueue.pop_front();
            auto object = data.objectList->VUnknown04(0, header.owner, header.actionId, header.x, header.y, header.dir, header.layerMaybe, 0, header.size);
            restoreChunk(chunkQueue.front(), (uint8_t*)object->unknown35C, header.size * 4); chunkQueue.pop_front();
            restore(*object);
        }
    }

    return true;
}
