//
// Created by Loyal on 9/20/26.
//

#include "Event.h"
namespace Euclase {
    std::unordered_map<uint32_t, std::vector<std::function<void(Event)>>> EventManager::events;
    void EventManager::RegisterEvent(EventType type, std::function<void(Event)> callback) {
    events[static_cast<int>(type)].push_back(callback);

    }

    void EventManager::DispatchEvent(Event event) {
        for (auto& callback : events[static_cast<int>(event.type)])
            callback(event);
    }

}
