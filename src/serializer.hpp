#pragma once

#include <SokuLib.hpp>
#include <WeatherManager.hpp>
#include <StageManager.hpp>
#include <InfoManager.hpp>
#include <map>
#include <ostream>
#include <istream>

namespace cbor {
    enum MajorType : char {
        INTEGER         = 0,
        NEG_INTEGER     = 1 << 5,
        BINARY          = 2 << 5,
        STRING          = 3 << 5,
        ARRAY           = 4 << 5,
        MAP             = 5 << 5,
        TAG             = 6 << 5,
        FLOAT           = 7 << 5,
    };

    inline char Major(char byte) { return byte & 0xE0; }
    inline char Count(char byte) { return byte & 0x1F; }
    inline size_t Head(std::istream& stream, char& major) {
        size_t value = 0;
        if (stream.get(major).fail()) throw std::runtime_error("Could not read Type of data");
        char count = Count(major);
        major = Major(major);
        switch (count) {
            case 0x18: stream.read((char*)&value, 1); break;
            case 0x19: stream.read((char*)&value, 2); break;
            case 0x1A: stream.read((char*)&value, 4); break;
            case 0x1B: assert(sizeof(value) >= 8); stream.read((char*)&value, 8); break;
            default: value = count;
        }

        return value;
    }

    inline void Head(std::ostream& stream, char major, size_t value) {
        char count = value < 24 ? value
            : value < 0x100 ? 0x18
            : value < 0x10000 ? 0x19
            : value < 0x100000000 ? 0x1A : 0x1B;
        stream.put(Major(major) | count);
        switch (count) {
            case 0x18: stream.write((char*)&value, 1); break;
            case 0x19: stream.write((char*)&value, 2); break;
            case 0x1A: stream.write((char*)&value, 4); break;
            case 0x1B: assert(sizeof(value) >= 8); stream.write((char*)&value, 8); break;
        }
    }
}

class BaseVisitor {
public:
    virtual size_t arraySize(size_t) = 0;
    virtual size_t mapSize(size_t) = 0;
    virtual void parse(char*, size_t) = 0;
    virtual void parse(unsigned int&) = 0;
    virtual void parse(int&) = 0;
    virtual void parse(SokuLib::String&) = 0;
    virtual bool parseHandle(unsigned int&) = 0;
    virtual size_t parseCustom(void*&, size_t) = 0;

    inline void parse(unsigned short& data) { unsigned int tmp = data; parse(tmp); data = tmp; }
    inline void parse(SokuLib::Sprite& data) { parse((char*)&data, sizeof(data)); }
    inline void parse(SokuLib::SpriteEx& data) { parse((char*)&data, sizeof(data)); }

    //template<class T> inline void parse(T *& data) { parse((unsigned int&)data); }
    template<class T> void parse(SokuLib::HandleManagerEx<T>&);
    template<class T> void parse(SokuLib::v2::EffectManager<T>&);
    template<typename T> void parse(SokuLib::Vector<T>&);
    template<typename T> void parse(SokuLib::List<T>&);
    template<typename T> void parse(SokuLib::Deque<T>&);
    template<typename T, typename U> void parse(SokuLib::Map<T, U>&);
    template<typename T, typename U> inline void parse(std::pair<T, U>& data) { arraySize(2); parse(data.first); parse(data.second); }

    void parse(SokuLib::Vector2f& data);
    void parse(SokuLib::Card& data);
    void parse(SokuLib::CardInfo& data);
    void parse(SokuLib::v2::SystemEffectObject& data);
    void parse(SokuLib::v2::SystemEffectManager& data);
    void parse(SokuLib::v2::AnimationObject& data);
    void parse(SokuLib::v2::EffectObject& data);
    void parse(SokuLib::v2::InfoEffectObject& data);
    void parse(SokuLib::v2::WeatherEffectObject& data);
    void parse(SokuLib::v2::GameObjectBase& data);
    void parse(SokuLib::v2::TailObject& data);
    void parse(SokuLib::v2::GameObject& data);
    void parse(SokuLib::v2::Player& data);
    void parse(SokuLib::v2::Player::StandInfo& data);
    void parse(SokuLib::v2::Player::DeckInfo& data);
    void parse(SokuLib::v2::Player::HandInfo& data);
    void parse(SokuLib::v2::Player::InputInfo& data);
    void parse(SokuLib::v2::GameDataManager& data);
    void parse(SokuLib::v2::StageManager& data);
    void parse(SokuLib::BattleManager& data);
    void parse(SokuLib::v2::InfoManagerBase& data);
    void parse(SokuLib::v2::WeatherManager& data);

    void run();
};

class Serialize : public BaseVisitor {
public:
    using BaseVisitor::parse;
    std::ostream& stream;

    size_t arraySize(size_t) override;
    size_t mapSize(size_t) override;
    void parse(char*, size_t) override;
    void parse(unsigned int&) override;
    void parse(int&) override;
    void parse(SokuLib::String&) override;
    bool parseHandle(unsigned int&) override;
    size_t parseCustom(void*&, size_t) override;

    inline Serialize(std::ostream& stream) : stream(stream) {}
};

class Restore : public BaseVisitor {
public:
    using BaseVisitor::parse;
    std::istream& stream;

    size_t arraySize(size_t) override;
    size_t mapSize(size_t) override;
    void parse(char*, size_t) override;
    void parse(unsigned int&) override;
    void parse(int&) override;
    void parse(SokuLib::String&) override;
    bool parseHandle(unsigned int&) override;
    size_t parseCustom(void*&, size_t) override;

    inline Restore(std::istream& stream) : stream(stream) {}
};
