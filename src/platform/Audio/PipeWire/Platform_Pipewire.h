//
// Created by Loyal on 10/6/26.
//

#ifndef EUCLASESOUND_PLATFORM_PIPEWIRE_H
#define EUCLASESOUND_PLATFORM_PIPEWIRE_H

#include <mutex>

#include "../Platform_Audio.h"
#include <pipewire/pipewire.h>
namespace Euclase {

    class Platform_Pipewire : public Platform_Audio {
    public:
        Platform_Pipewire();

        ~Platform_Pipewire();
        [[nodiscard]] bool Init(uint32_t sampleRate, uint32_t channels) override;
        void Start() override;
        void Stop() override;

        [[nodiscard]] bool IsInitialized() const { return m_initialized; }
        std::vector<AudioDevice> GetAudioDevices(DeviceFilter filter) override;
        [[nodiscard]] bool SetDevice(const std::string& nodeName) override;
    private:
        static void OnProcess(void* userdata);

        spa_hook m_streamListener{};
        bool m_initialized = false;
        bool m_running = false;

        uint32_t m_sampleRate = 48000;
        uint32_t m_channels = 2;

        pw_thread_loop* m_loop = nullptr;
        pw_context* m_context = nullptr;
        pw_core* m_core = nullptr;
        pw_stream* m_stream = nullptr;

        double m_phase = 0.0;
    private:
        //device enumeration
        static void OnRegistryGlobal(void* data, uint32_t id, uint32_t permissions,
                             const char* type, uint32_t version, const spa_dict* props);
        static void OnRegistryGlobalRemove(void* data, uint32_t id);
        static void OnCoreDone(void* data, uint32_t id, int seq);
        void Roundtrip();   // call with loop locked

        pw_registry*  m_registry = nullptr;
        spa_hook      m_registryListener{};
        spa_hook      m_coreListener{};
        int           m_syncSeq  = 0;
        bool          m_syncDone = false;

        std::mutex               m_devicesMutex;
        std::vector<AudioDevice> m_devices;
        std::string              m_target;
    };
} // Euclase

#endif //EUCLASESOUND_PLATFORM_PIPEWIRE_H
