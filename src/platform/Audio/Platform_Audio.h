//
// Created by Loyal on 10/6/26.
//

#ifndef EUCLASESOUND_PLATFORM_AUDIO_H
#define EUCLASESOUND_PLATFORM_AUDIO_H
#include <cstdint>
#include <memory>
#include <vector>

namespace Euclase {
    struct AudioDevice {
        uint32_t    id;
        std::string name;
        std::string description;
        bool        isOutput;   // Audio/Sink (or Duplex)
        bool        isInput;    // Audio/Source (or Duplex)
    };

    enum class DeviceFilter { All, Outputs, Inputs };
    class Platform_Audio {
    public:
        static std::vector<std::shared_ptr<Platform_Audio>> EnumerateDevices();

        virtual std::vector<AudioDevice> GetAudioDevices(DeviceFilter filter = DeviceFilter::All) = 0;
        virtual bool SetDevice(const std::string& nodeName) = 0;
        virtual bool Init(std::uint32_t sampleRate, uint32_t channels) = 0;
        virtual void Start() = 0;
        virtual void Stop() = 0;
    };

}


#endif //EUCLASESOUND_PLATFORM_AUDIO_H
