//
// Created by Loyal on 9/20/26.
//

#ifndef EUCLASESOUND_PLATFORM_H
#define EUCLASESOUND_PLATFORM_H
#include <memory>
#include "../Defines.h"

#include <vulkan/vulkan.h>
namespace Euclase {
    class Platform {
        public:
        virtual ~Platform() = default;

        static std::shared_ptr<Platform> GetNewWindow();
        virtual bool ShouldClose() = 0;
        virtual bool create(int width, int height, const char *title) = 0;
        virtual void Disconnect() = 0;
        virtual void Dispatch() = 0;
    };
} // Euclase

#endif //EUCLASESOUND_PLATFORM_H
