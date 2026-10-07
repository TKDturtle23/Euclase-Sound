//
// Created by Loyal on 10/6/26.
//
#include "../../../Defines.h"
#ifdef Wayland

#include <spa/param/audio/format-utils.h>
#include "Platform_Pipewire.h"
#include <pipewire/pipewire.h>
#define M_PI_M2 ( M_PI + M_PI )

#define DEFAULT_RATE            44100
#define DEFAULT_CHANNELS        2
#define DEFAULT_VOLUME          0.7

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <spa/utils/hook.h>
#include <spa/utils/result.h>
namespace Euclase {

Platform_Pipewire::Platform_Pipewire()
{
    pw_init(nullptr, nullptr);
}

    Platform_Pipewire::~Platform_Pipewire()
{
    Stop();
    if (m_loop) pw_thread_loop_stop(m_loop);

    if (m_stream) {
        spa_hook_remove(&m_streamListener);
        pw_stream_destroy(m_stream);
        m_stream = nullptr;
    }
    if (m_registry) {
        spa_hook_remove(&m_registryListener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy*>(m_registry));
        m_registry = nullptr;
    }
    if (m_core) spa_hook_remove(&m_coreListener);
    if (m_context) { pw_context_destroy(m_context);  m_context = nullptr; }
    if (m_loop)    { pw_thread_loop_destroy(m_loop); m_loop = nullptr; }

    pw_deinit();
}

bool Platform_Pipewire::Init(uint32_t sampleRate, uint32_t channels)
{
    m_sampleRate = sampleRate;
    m_channels = channels;

    m_loop = pw_thread_loop_new("EuclaseAudio", nullptr);
    if (!m_loop) { std::cerr << "Failed to create thread loop\n"; return false; }

    m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
    if (!m_context) { std::cerr << "Failed to create context\n"; return false; }

    if (pw_thread_loop_start(m_loop) < 0) { std::cerr << "Failed to start loop\n"; return false; }

    pw_thread_loop_lock(m_loop);

    m_core = pw_context_connect(m_context, nullptr, 0);
    if (!m_core) {
        pw_thread_loop_unlock(m_loop);
        std::cerr << "Failed to connect to PipeWire\n";
        return false;
    }
    static const pw_core_events coreEvents = {
        .version = PW_VERSION_CORE_EVENTS,
        .done    = OnCoreDone,
    };
    pw_core_add_listener(m_core, &m_coreListener, &coreEvents, this);

    m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
    static const pw_registry_events regEvents = {
        .version       = PW_VERSION_REGISTRY_EVENTS,
        .global        = OnRegistryGlobal,
        .global_remove = OnRegistryGlobalRemove,
    };
    pw_registry_add_listener(m_registry, &m_registryListener, &regEvents, this);

    Roundtrip();   // initial device list is populated when this returns

    pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Music",
        nullptr);

    m_stream = pw_stream_new(m_core, "EuclaseSound", props);
    if (!m_stream) {
        pw_thread_loop_unlock(m_loop);
        return false;
    }

    static const pw_stream_events events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .process = OnProcess,
    };
    pw_stream_add_listener(m_stream, &m_streamListener, &events, this);

    pw_thread_loop_unlock(m_loop);
    m_initialized = true;
    return true;
}

    void Platform_Pipewire::Start()
{
    if (!m_initialized || m_running) return;

    uint8_t buffer[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    spa_audio_info_raw info = {};
    info.format   = SPA_AUDIO_FORMAT_F32;
    info.rate     = m_sampleRate;
    info.channels = m_channels;
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;

    const spa_pod* params[1];
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info);

    pw_thread_loop_lock(m_loop);

    spa_dict_item item = SPA_DICT_ITEM_INIT(
    PW_KEY_TARGET_OBJECT,
    m_target.empty() ? nullptr : m_target.c_str());   // nullptr removes the key -> default sink
    spa_dict dict = SPA_DICT_INIT(&item, 1);
    pw_stream_update_properties(m_stream, &dict);

    const int result = pw_stream_connect(
        m_stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
        static_cast<pw_stream_flags>(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS),
        params, 1);
    pw_thread_loop_unlock(m_loop);

    if (result < 0) {
        std::cerr << "Failed to connect stream: " << spa_strerror(result) << '\n';
        return;
    }
    m_running = true;
}
    void Platform_Pipewire::Stop()
{
    if (!m_running) return;
    pw_thread_loop_lock(m_loop);
    pw_stream_disconnect(m_stream);
    pw_thread_loop_unlock(m_loop);
    m_running = false;
}


    void Platform_Pipewire::OnProcess(void* userdata)
{
    auto* self = static_cast<Platform_Pipewire*>(userdata);

    pw_buffer* b = pw_stream_dequeue_buffer(self->m_stream);
    if (!b) return;

    spa_buffer* sb = b->buffer;
    spa_data& d = sb->datas[0];

    if (!d.data) {
        pw_stream_queue_buffer(self->m_stream, b);
        return;
    }

    const uint32_t stride = sizeof(float) * self->m_channels;
    uint32_t frames = d.maxsize / stride;
    if (b->requested && b->requested < frames)
        frames = static_cast<uint32_t>(b->requested);

    auto* out = static_cast<float*>(d.data);
    constexpr double freq = 440.0;
    const double step = freq / self->m_sampleRate;

    for (uint32_t f = 0; f < frames; ++f) {
        const float v = static_cast<float>(std::sin(self->m_phase * 2.0 * M_PI) * DEFAULT_VOLUME);
        for (uint32_t c = 0; c < self->m_channels; ++c)
            out[f * self->m_channels + c] = v;

        self->m_phase += step;
        if (self->m_phase >= 1.0) self->m_phase -= 1.0;
    }

    d.chunk->offset = 0;
    d.chunk->stride = stride;
    d.chunk->size   = frames * stride;

    pw_stream_queue_buffer(self->m_stream, b);
}



    void Platform_Pipewire::OnRegistryGlobal(void* data, uint32_t id, uint32_t,
                                             const char* type, uint32_t, const spa_dict* props)
{
    auto* self = static_cast<Platform_Pipewire*>(data);
    if (!props || !type || strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
        return;

    const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!mediaClass) return;

    const bool isSink   = strncmp(mediaClass, "Audio/Sink",   10) == 0;
    const bool isSource = strncmp(mediaClass, "Audio/Source", 12) == 0;
    const bool isDuplex = strcmp (mediaClass, "Audio/Duplex")    == 0;
    if (!isSink && !isSource && !isDuplex) return;

    const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    if (!name) return;

    const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    if (!desc) desc = spa_dict_lookup(props, PW_KEY_NODE_NICK);
    if (!desc) desc = spa_dict_lookup(props, PW_KEY_DEVICE_DESCRIPTION);

    std::lock_guard lock(self->m_devicesMutex);
    self->m_devices.push_back({
        id, name, desc ? desc : name,
        isSink   || isDuplex,
        isSource || isDuplex
    });
}

void Platform_Pipewire::OnRegistryGlobalRemove(void* data, uint32_t id)
{
    auto* self = static_cast<Platform_Pipewire*>(data);
    std::lock_guard lock(self->m_devicesMutex);
    std::erase_if(self->m_devices, [id](const AudioDevice& d) { return d.id == id; });
}

void Platform_Pipewire::OnCoreDone(void* data, uint32_t id, int seq)
{
    auto* self = static_cast<Platform_Pipewire*>(data);
    if (id == PW_ID_CORE && seq == self->m_syncSeq) {
        self->m_syncDone = true;
        pw_thread_loop_signal(self->m_loop, false);
    }
}

void Platform_Pipewire::Roundtrip()
{
    m_syncDone = false;
    m_syncSeq  = pw_core_sync(m_core, PW_ID_CORE, 0);
    while (!m_syncDone)
        pw_thread_loop_wait(m_loop);
}

std::vector<AudioDevice> Platform_Pipewire::GetAudioDevices(DeviceFilter filter)
{
    std::lock_guard lock(m_devicesMutex);
    if (filter == DeviceFilter::All)
        return m_devices;

    std::vector<AudioDevice> out;
    for (const auto& d : m_devices)
        if ((filter == DeviceFilter::Outputs && d.isOutput) ||
            (filter == DeviceFilter::Inputs  && d.isInput))
            out.push_back(d);
    return out;
}

bool Platform_Pipewire::SetDevice(const std::string& nodeName)
{
    if (!nodeName.empty()) {
        std::lock_guard lock(m_devicesMutex);
        bool found = std::any_of(m_devices.begin(), m_devices.end(),
            [&](const AudioDevice& d) { return d.isOutput && d.name == nodeName; });
        if (!found) return false;
    }

    const bool wasRunning = m_running;
    if (wasRunning) Stop();
    m_target = nodeName;
    if (wasRunning) Start();
    return true;
}
}
#endif