//
// Created by Loyal on 9/20/26.
//

#ifndef EUCLASESOUND_EVENT_H
#define EUCLASESOUND_EVENT_H
#include <cstdint>
#include <functional>
#include <unordered_map>
namespace Euclase {
    enum class EventType {
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMove,
        WindowRender,
        KeyDown,
        KeyUp,
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseScroll
    };
    struct Event {
        EventType type;
        uint8_t data[12];
        uint32_t data32[2];
        uint64_t data64[2];
        double dataDouble[2];
    };
    class EventManager {
    public:
        static void RegisterEvent(EventType type, std::function<void(Event)> callback);
        static void DispatchEvent(Event event);
    private:
        static std::unordered_map<uint32_t, std::vector<std::function<void(Event)>>> events;
    };

}

#endif //EUCLASESOUND_EVENT_H
