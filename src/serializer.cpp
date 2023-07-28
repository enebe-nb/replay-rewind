#include "serializer.hpp"
#include "main.hpp"

using namespace SokuLib;

size_t Serialize::arraySize(size_t size) {
    cbor::Head(stream, cbor::ARRAY, size);
    return size;
}

size_t Serialize::mapSize(size_t size) {
    cbor::Head(stream, cbor::MAP, size);
    return size;
}

void Serialize::parse(char* data, size_t size) {
    cbor::Head(stream, cbor::BINARY, size);
    stream.write(data, size);
}

void Serialize::parse(unsigned int& data) {
    cbor::Head(stream, cbor::INTEGER, data);
}

void Serialize::parse(int& data) {
    if (data < 0) {
        cbor::Head(stream, cbor::NEG_INTEGER, -1-data);
    } else {
        cbor::Head(stream, cbor::INTEGER, data);
    }
}

void Serialize::parse(String& data) {
    cbor::Head(stream, cbor::STRING, data.size);
    stream.write(data.res < String::BUF_SIZE ? data.body.buf : data.body.ptr, data.size);
    // TODO utf-8 convertion?
}

bool Serialize::parseHandle(unsigned int& id) {
    cbor::Head(stream, cbor::INTEGER, id);
    return false;
}

size_t Serialize::parseCustom(void*& data, size_t size) {
    cbor::Head(stream, cbor::BINARY, size);
    stream.write((char*)data, size);
    return size;
}

size_t Restore::arraySize(size_t size) {
    char major;
    size = cbor::Head(stream, major);
    if (major != cbor::ARRAY) throw std::runtime_error("Head not valid for array");
    return size;
}

size_t Restore::mapSize(size_t size) {
    char major;
    size = cbor::Head(stream, major);
    if (major != cbor::MAP) throw std::runtime_error("Head not valid for map");
    return size;
}

void Restore::parse(char* data, size_t size) {
    char major;
    size_t sizeB = cbor::Head(stream, major);
    if (major != cbor::BINARY) throw std::runtime_error("Head not valid for binary");
    if (sizeB != size) throw std::runtime_error("Read data don't matches the size");
    stream.read(data, size);
    if (stream.gcount() != size) throw std::runtime_error("Unexpected EOF.");
}

void Restore::parse(unsigned int& data) {
    char major;
    data = cbor::Head(stream, major);
    if (major != cbor::INTEGER) throw std::runtime_error("Head not valid for unsigned integer");
}

void Restore::parse(int& data) {
    char major;
    uint64_t tmp = cbor::Head(stream, major);
    if (major == cbor::NEG_INTEGER) {
        data = -1-tmp;
    } else if (major == cbor::INTEGER) {
        data = tmp;
    } else throw std::runtime_error("Head not valid for signed integer");
}

void Restore::parse(String& data) {
    char major;
    size_t size = cbor::Head(stream, major);
    if (major != cbor::STRING) throw std::runtime_error("Head not valid for string");
    data.resize(size);
    stream.read(data.res < String::BUF_SIZE ? data.body.buf : data.body.ptr, data.size);
    // TODO utf-8 convertion?
}

bool Restore::parseHandle(unsigned int& id) {
    char major; bool needDestroy = id;
    id = cbor::Head(stream, major);
    if (id) needDestroy = false;
    return needDestroy;
}

size_t Restore::parseCustom(void*& data, size_t size) {
    char major;
    size_t sizeB = cbor::Head(stream, major);
    if (major != cbor::BINARY) throw std::runtime_error("Head not valid for binary");
    if (sizeB != size) {
        if (data) { SokuLib::DeleteFct(data); data = 0; }
        if (sizeB) { data = SokuLib::NewFct(sizeB); }
    }
    stream.read((char*)data, sizeB);
    return sizeB;
}

template<typename T> void BaseVisitor::parse(Vector<T>& data) {
    size_t size = arraySize(data.size());
    if (size != data.size()) data.resize(size); // TODO check validity
    for (int i = 0; i < size; ++i) {
        parse(data[i]);
    }
}

template<typename T> void BaseVisitor::parse(List<T>& data) {
    size_t size = arraySize(data.size());
    if (size != data.size()) data.resize(size); // TODO check validity
    for (auto& val : data) parse(val);
}

template<typename T> void BaseVisitor::parse(Deque<T>& data) {
    size_t size = arraySize(data.size());
    if (size != data.size()) data.resize(size); // TODO check validity
    for (auto& val : data) parse(val);
}

template<typename T, typename U> void BaseVisitor::parse(Map<T, U>& data) {
    throw std::runtime_error("Restoring a map is not much easy.");
    // size_t size = mapSize(data.size);
    // if (size != data.size) data.resize(size); // TODO check validity
    // for (auto& val : data) {
    //     parse(val.first);
    //     parse(val.second);
    // }
}

template<class T> void BaseVisitor::parse(HandleManagerEx<T>& data) {
    arraySize(3);

    // TODO data.mutex.lock();
    parse(data.unusedIndexes);
    parse(data.nextBase);

    size_t size = arraySize(data.usedIndexes.size());
    while (size < data.usedIndexes.size()) {
        if (data.usedIndexes[size]) {
            data.vector[size]->~T();
            data.usedIndexes[size] = 0;
        } data.unusedIndexes.push_back(size++);
    }
    if (size != data.usedIndexes.size()) throw std::runtime_error("Can't restore HandleManagerEx size to a bigger value.");
    // TODO data.mutex.unlock();

    for (int i = 0; i < size; ++i) {
        if (parseHandle(data.usedIndexes[i])) data.vector[i]->~T(); 
    }

    // the data in the handles is processed by the owning object (or i hope so)
}

template<class T> void BaseVisitor::parse(v2::EffectManager<T>& data) {
    arraySize(4);

    parse(data.handles);
    parse(data.textureIds);
    // (ignore) pattern data should not change
    //parse(data.patterns);
    //parse(data.patternById);

    parse(*(List<unsigned int>*)&data.effects);
    size_t size = arraySize(data.effects.size());
    if (size != data.effects.size()) throw std::runtime_error("Size of pointers and objects must be the same.");
    for (auto effect : data.effects) parse(*effect);
}

void BaseVisitor::parse(Vector2f& data) {
    parse((char*)&data, 0x08);
}

void BaseVisitor::parse(Card& data) {
    arraySize(3);
    parse(data.id);
    parse(data.cost);
    parse(data.sprite);
}

void BaseVisitor::parse(CardInfo& data) {
    arraySize(3);
    // name
    parse(data.name);
    // type and cost TODO remove align
    parse((char*)&data.type, 0x04);
    // description
    parse(data.description);
    // other data TODO remove align
    parse((char*)&data.apparency, 0x24);
}

void BaseVisitor::parse(v2::SystemEffectObject& data) {
    arraySize(3);

    // unknown04,08
    parse((char*)&data.unknown04, 0x08);
    // sprite
    parse(data.sprite);
    // coords and renderInfo TODO remove align
    parse((char*)&data.position, 0x50);
}

void BaseVisitor::parse(v2::SystemEffectManager& data) {
    arraySize(3);

    parse(data.handles);
    size_t size = arraySize(data.objects.size());
    if (size != data.objects.size()) data.objects.resize(size);
    for (auto& object : data.objects) {
        parse(*(unsigned int*)&object);
    }

    size_t sizeB = arraySize(data.objects.size());
    if (size != sizeB) throw std::runtime_error("Size of pointers and objects must be the same.");
    for (auto& object : data.objects) {
        parse(*object);
    }
}

void BaseVisitor::parse(v2::AnimationObject& data) {
    arraySize(3);

    // sprite
    parse(data.sprite);
    // coords and renderInfo TODO remove align
    parse((char*)&data.position, 0x50);
    // framestate TODO remove align
    parse((char*)&data.frameState, sizeof(data.frameState) +0x04);
}

void BaseVisitor::parse(SokuLib::v2::EffectObjectBase& data) {
    arraySize(2);
    parse((v2::AnimationObject&)data);
    // contains pointers to Sequence data that won't be reallocated
    parse((char*)&data.unknown158, 0x15);
}

void BaseVisitor::parse(SokuLib::v2::WeatherEffectObject& data) {
    arraySize(2);
    parse((v2::EffectObjectBase&)data);
    parse((char*)&data.unknown170, 0x10);
}

void BaseVisitor::parse(v2::GameObjectBase& data) {
    arraySize(5);

    // base class
    parse((v2::AnimationObject&)data);
    // gameData
    parse((char*)&data.gameData, sizeof(data.gameData));
    // composed bouding box
    parse(*(unsigned int*)&data.parentA);
    parse(*(List<unsigned int>*)&data.childrenA);
    // hp and other object states TODO remove align
    parse((char*)&data +0x184, 0x30);
    // boxData (ignore)
    //parse((char*)&data.boxData, sizeof(data.boxData));
}

void BaseVisitor::parse(v2::TailObject& data) {
    arraySize(4);
    size_t oldSize = data.vertexBufferSize();

    parse((char*)&data, 0x28);
    parse(data.unknown28);
    parse(data.unknown3C);

    size_t size = arraySize(data.vertexBufferSize());
    if (size != oldSize) {
        SokuLib::DeleteFct(data.vertexBuffer);
        data.vertexBuffer = SokuLib::New<DxVertex>(size);
    }
    for (int i = 0; i < size; ++i) parse((char*)&data.vertexBuffer[i], sizeof(DxVertex));
}

void BaseVisitor::parse(v2::GameObject& data) {
    bool isAliceObject = *(int*)&data == SokuLib::_vtable_info<v2::GameObjectAlice>::baseAddr;
    arraySize(isAliceObject? 7 : 6);

    // base class
    parse((v2::GameObjectBase&)data);

    // lifetime, handle, tail, layer
    v2::TailObject* oldTail = data.tail;
    parse((char*)&data.lifetime, 0x0D);

    // tail data
    if (data.tail != oldTail) {
        if (oldTail) SokuLib::Delete(oldTail);
        if (data.tail) data.tail = SokuLib::New<v2::TailObject>(1);
    }
    if (data.tail) parse(*data.tail);
    else { arraySize(0); }

    // custom Data
    auto& dataSize = customDataSize[&data];
    dataSize = parseCustom(data.customData, dataSize);

    // other data
    parse((char*)&data.unknown360, 0x40);
    parse(*(List<unsigned int>*)&data.childrenB);

    if (isAliceObject) parse((char*)&data +0x3AC, 0x04);
}

void BaseVisitor::parse(v2::Player& data) {
    arraySize(18);

    // base class
    parse((v2::GameObjectBase&)data);
    // basic definitions TODO remove align
    parse((char*)&data +0x34C, 0x0C);
    // portrait
    parse(data.portrait);
    // standInfo
    parse(data.stand);
    // player data TODO remove align
    parse((char*)&data +0x498, 0xE4);
    // deck and hand
    parse(data.deckInfo);
    parse(data.handInfo);
    // skill level data
    parse(data.unknown610);
    parse((char*)&data +0x6A4, 0x54);
    // some other graphic data
    parse((char*)&data.unknown710, 0x04);
    if (data.unknown714.unknown714.size()) throw std::runtime_error("I wanna test this");
    parse(data.unknown714.unknown714);
    parse((char*)&data.unknown714.unknown720, 0x0C);
    // spell backgrounds
    parse(data.spellBgTextures);
    parse((char*)&data.spellBgTimer, 0x10);
    // input
    parse(data.inputData);
    // gui control data
    parse((char*)&data +0x7D0, 0xC0);

    { // Objects
        arraySize(3);
        // handle manager
        if (*(int*)data.objectList == 0x85c9c4) {
            parse(*(HandleManagerEx<v2::GameObjectAlice>*)((int)data.objectList+0x08));
        } else {
            parse(*(HandleManagerEx<v2::GameObject>*)((int)data.objectList+0x08));
        }

        // object list
        auto& list = data.objectList->GetList();
        size_t size = arraySize(list.size());
        if (size != list.size()) list.resize(size);
        for (auto& object : list) parse(*(unsigned int*)&object);
        
        size_t sizeB = arraySize(list.size());
        if (sizeB != size) throw std::runtime_error("Size of pointers and objects must be the same.");
        for (auto& object : list) parse(*object);
    }

    // extra data
    switch(data.characterIndex) {
        case SokuLib::CHARACTER_REMILIA:
        case SokuLib::CHARACTER_CHIRNO:
        case SokuLib::CHARACTER_MEILING:
            parse((char*)&data +0x890, 0x04);
            break;
        case SokuLib::CHARACTER_SUIKA:
        case SokuLib::CHARACTER_AYA:
        case SokuLib::CHARACTER_KOMACHI:
            parse((char*)&data +0x890, 0x08);
            break;
        case SokuLib::CHARACTER_SAKUYA:
        case SokuLib::CHARACTER_NAMAZU:
            parse((char*)&data +0x890, 0x10);
            break;
        case SokuLib::CHARACTER_PATCHOULI:
        case SokuLib::CHARACTER_UTSUHO:
        case SokuLib::CHARACTER_SUWAKO:
        case SokuLib::CHARACTER_IKU:
            parse((char*)&data +0x890, 0x18);
            break;
        case SokuLib::CHARACTER_MARISA:
        case SokuLib::CHARACTER_YUYUKO:
            parse((char*)&data +0x890, 0x1C);
            break;
        case SokuLib::CHARACTER_SANAE:
            parse((char*)&data +0x890, 0x20);
            break;
        case SokuLib::CHARACTER_UDONGE:
            parse((char*)&data +0x890, 0x28);
            break;
        case SokuLib::CHARACTER_REIMU:
            parse((char*)&data +0x890, 0x2C);
            break;
        case SokuLib::CHARACTER_ALICE:
            parse((char*)&data +0x890, 0x34);
            break;
        case SokuLib::CHARACTER_YUKARI:
            parse((char*)&data +0x890, 0x44);
            break;
        case SokuLib::CHARACTER_YOUMU: // TODO
            throw std::runtime_error("Youmu is not implmented");
        case SokuLib::CHARACTER_TENSHI: // TODO
            throw std::runtime_error("Tenshi is not implmented");
        default:
            throw std::runtime_error("Something gone wrong while finding the character");
    }
}

void BaseVisitor::parse(v2::Player::StandInfo& data) {
    arraySize(3);
    parse(data.texId);
    parse(data.sprite);
    parse((char*)&data.playerId, 0x11);
}

void BaseVisitor::parse(v2::Player::DeckInfo& data) {
    arraySize(4);
    parse(data.textures);
    // this is hard to restore but probably doesn't change
    //parse(data.cardById);
    parse(data.original);
    parse(data.queue);
    parse((char*)&data.availSkills, 0x32); // TODO problem with align and patchouli
}

void BaseVisitor::parse(v2::Player::HandInfo& data) {
    arraySize(3);
    parse((char*)&data, 0x04);
    parse(data.hand);
    parse(data.usedCards);
}

void BaseVisitor::parse(v2::Player::InputInfo& data) {
    arraySize(3);
    parse((char*)&data, 0x59);
    parse(data.commandInputBuffer);
    parse((char*)&data.movimentCombination, 0x09);
}

void BaseVisitor::parse(v2::GameDataManager& data) {
    arraySize(2);

    // player pointers
    parse((char*)&data.players, 0x14);

    // active players
    size_t size = arraySize(data.activePlayers.size());
    if (size != data.activePlayers.size()) data.activePlayers.resize(size);
    for (int i = 0; i < size; ++i) {
        parse(*data.activePlayers[i]);
    }
}

void BaseVisitor::parse(BattleManager& data) {
    arraySize(4);

    // frameCount, players
    parse((char*)&data +0x04, sizeof(0x18));
    // damage, matchState
    parse((char*)&data +0x74, sizeof(0x18));
    // scenario.effects TODO the whole scenario
    parse(data.scenario.effects);
    // roundData
    parse((char*)&data +0x900, sizeof(0x08));
}

void BaseVisitor::parse(v2::WeatherManager& data) {
    arraySize(4);

    // weather ids
    parse((char*)&data, sizeof(0x05));
    // tex ids
    parse(data.someTexIds);
    // sprites
    parse(data.someSprites);
    // maybe the weather display (text)
    parse(*(Deque<std::pair<unsigned int, unsigned int>>*)&data.unknown28);
    // (ignore) maybe the map that choose which effect to pla in each stage
    //parse(data.unknown3C);
    // EffectManager
    parse(data.effects);
    // some float at the end
    parse((char*)&data.unknownDC, sizeof(0x10));
}

void BaseVisitor::run() {
    arraySize(8);
    parse(*v2::GameDataManager::instance); // 0x8985DC
    parse(SokuLib::getBattleMgr()); // 0x8985E4
    parse(*v2::WeatherManager::instance); // 0x8985EC

    // random seed
    parse((char*)0x89b660, 0x9C0);
    parse((char*)0x896b20, 0x004);

    parse((char*)0x883cc8, 0x04); // two bytes used in battle manager
    parse((char*)0x8971c0, 0x18); // global weather variables (maybe 0x500 after it)
    parse((char*)0x8985d8, 0x04); // battle frame counter
    //parse((char*)0x8986dc, 0x04);
    //parse(*(char**)0x8971c8, 0x4C);
}

/*
bool Serializer::serialize(const SokuLib::BattleManager& data) {
    chunkQueue.push_back(createChunk(_offset(data, 0x004), 0x018));
    chunkQueue.push_back(createChunk(_offset(data, 0x074), 0x018));
    //chunkQueue.push_back(createChunk(_offset(data, 0x868), 0x09C));

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

    // Globals (Weather)
    serialize((uint8_t*)0x883cc8, 0x04);
    serialize((uint8_t*)0x8971c0, 0x18 + 0x500);
    serialize((uint8_t*)0x8985d8, 0x04);
    serialize((uint8_t*)0x8986dc, 0x04);
    serialize(*(uint8_t**)0x8971c8, 0x4C);

    return true;
}

bool Serializer::restore(SokuLib::BattleManager& data) {
    restoreChunk(chunkQueue.front(), _offset(data, 0x004), 0x018); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x074), 0x018); chunkQueue.pop_front();
    //restoreChunk(chunkQueue.front(), _offset(data, 0x868), 0x09C); chunkQueue.pop_front();

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

    // Globals (Weather)
    restore((uint8_t*)0x883cc8, 0x04);
    restore((uint8_t*)0x8971c0, 0x18 + 0x500);
    restore((uint8_t*)0x8985d8, 0x04);
    restore((uint8_t*)0x8986dc, 0x04);
    restore(*(uint8_t**)0x8971c8, 0x4C);

    return true;
}

bool Serializer::serialize(const SokuLib::v2::AnimationObject& data) {
    chunkQueue.push_back(createChunk(_offset(data, 0x0EC), 0x068));
    return true;
}

bool Serializer::restore(SokuLib::v2::AnimationObject& data) {
    restoreChunk(chunkQueue.front(), _offset(data, 0x0EC), 0x068); chunkQueue.pop_front();
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

bool Serializer::restore(SokuLib::v2::GameObjectBase& data) {
    restore((SokuLib::v2::AnimationObject&)data);
    restoreChunk(chunkQueue.front(), _offset(data, 0x158), 0x01C); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x184), 0x02C); chunkQueue.pop_front();
    return true;
}

bool Serializer::serialize(const SokuLib::v2::GameObject& data) {
    serialize((SokuLib::v2::GameObjectBase&)data);
    chunkQueue.push_back(createChunk(_offset(data, 0x34C), 0x004));
    chunkQueue.push_back(createChunk(_offset(data, 0x354), 0x008));
    // if (data.unknown354) {
    //     Unknown354Data adata;
    //     adata.length = data.unknown354->unknown28.size();
    //     adata.x04 = data.unknown354->paramB;
    //     adata.x08 = data.unknown354->paramC;
    //     adata.x10 = data.unknown354->paramA;
    //     adata.x14 = data.unknown354->paramD;
    //     chunkQueue.push_back(createChunk((uint8_t*)&adata, sizeof(adata)));
    // }
    chunkQueue.push_back(createChunk(_offset(data, 0x360), 0x03C));

    if (((SokuLib::v2::Player*)data.gameData.owner)->characterIndex == SokuLib::CHARACTER_ALICE) {
        chunkQueue.push_back(createChunk(_offset(data, 0x3AC), 0x004));
    }
    
    return true;
}

bool Serializer::restore(SokuLib::v2::GameObject& data) {
    restore((SokuLib::v2::GameObjectBase&)data);
    restoreChunk(chunkQueue.front(), _offset(data, 0x34C), 0x004); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x354), 0x008); chunkQueue.pop_front();
    data.tail = 0;
        // Unknown354Data adata;
        // restoreChunk(chunkQueue.front(), (uint8_t*)&adata, sizeof(adata)); chunkQueue.pop_front();
        // data.unknown354 = 0;
        // data.spawnParticles(data.frameState.actionId, adata.x10, adata.x04, adata.x08, adata.x14);
        // while(data.unknown354->unknown28.size() < adata.length) data.unknown354->update();
    restoreChunk(chunkQueue.front(), _offset(data, 0x360), 0x03C); chunkQueue.pop_front();

    if (((SokuLib::v2::Player*)data.gameData.owner)->characterIndex == SokuLib::CHARACTER_ALICE) {
        restoreChunk(chunkQueue.front(), _offset(data, 0x3AC), 0x004);
        chunkQueue.pop_front();
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
        for (auto& val : data.handData.hand) { *(uint16_t*)chunk = val.id; chunk += 2; }
        *(uint32_t*)chunk = data.handData.usedCards.size(); chunk += 4;
        for (auto val : data.handData.usedCards) { *(uint16_t*)chunk = val; chunk += 2; }
        // TODO hand restore: 0x469990
    }

    chunkQueue.push_back(createChunk(_offset(data, 0x6A4), 0x054));
    chunkQueue.push_back(createChunk(_offset(data, 0x710), 0x04));
    chunkQueue.push_back(createChunk(_offset(data, 0x720), 0x0C));
    chunkQueue.push_back(createChunk(_offset(data, 0x740), 0x10));
    chunkQueue.push_back(createChunk((uint8_t*)data.keyManager.keymapManager, 0x078));
    chunkQueue.push_back(createChunk(_offset(data, 0x754), 0x5C));
    {
        chunkQueue.push_back(createChunk(data.inputData.commandInputBuffer.size()*2 + 4));
        uint8_t* chunk = chunkQueue.back();
        *(uint32_t*)chunk = data.inputData.commandInputBuffer.size(); chunk += 4;
        for (auto val : data.inputData.commandInputBuffer) { *(uint16_t*)chunk = val; chunk += 2; }
    }
    chunkQueue.push_back(createChunk(_offset(data, 0x7C4), 0xCC));

    {
        auto list = data.objectList->VUnknown24();
        size_t objectCount = list->size();
        chunkQueue.push_back(createChunk((uint8_t*)&objectCount, 4));
        for (auto object : *list) {
            auto header = goHeaders[(void*)object];
            if (object->parentB) {
                int index = 0; for(auto it = list->begin(); it != list->end() && object->parentB != (void*)*it; ++it, ++index);
                if (index >= list->size()) throw std::exception("Serializer: parent not found in list");
                header.parentIndex = index;
            } else header.parentIndex = -1;
            chunkQueue.push_back(createChunk((uint8_t*)&header, sizeof(header)));
            chunkQueue.push_back(createChunk((uint8_t*)object->unknown35C, header.size * 4));
            serialize(*object);
        }
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
            const auto cardInfo = data.deckData.cardLookup(*(uint16_t*)chunk);
            data.handData.hand.push_back({*(uint16_t*)chunk, cardInfo->type == 1 ? (unsigned short)1 : cardInfo->costOrSlot});
            if (cardInfo->texIdB) data.handData.hand.back().sprite.setTexture2(cardInfo->texIdB, 0, 0, *(int*)0x897004, *(int*)0x897000);
            chunk += 2;
        } size = *(uint32_t*)chunk; chunk += 4;
        data.handData.usedCards.clear(); while (size--) { data.handData.usedCards.push_back(*(uint16_t*)chunk); chunk += 2; }
        delete[] chunkQueue.front(); chunkQueue.pop_front();
    }

    restoreChunk(chunkQueue.front(), _offset(data, 0x6A4), 0x054); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x710), 0x004); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x720), 0x00C); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x740), 0x010); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), (uint8_t*)data.keyManager.keymapManager, 0x078); chunkQueue.pop_front();
    restoreChunk(chunkQueue.front(), _offset(data, 0x754), 0x05C); chunkQueue.pop_front();
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
            void* parent = 0; if (header.parentIndex != -1) {
                auto list = data.objectList->VUnknown24();
                if (header.parentIndex >= list->size()) throw std::exception("Restore: index >= list->size()");
                auto it = list->begin(); while(header.parentIndex--) ++it;
                parent = *it;
            }
            auto object = data.objectList->VUnknown04(parent, header.owner, 0, header.x, header.y, header.dir, header.layerMaybe, 0, header.size);
            restoreChunk(chunkQueue.front(), (uint8_t*)object->unknown35C, header.size * 4); chunkQueue.pop_front();
            restore(*object);
            reinterpret_cast<void (__fastcall*)(void*)>(0x43a250)(object); // Update Sprite
        }
    }

    reinterpret_cast<void (__fastcall*)(void*)>(0x43a250)(&data); // Update Sprite

    return true;
}
*/