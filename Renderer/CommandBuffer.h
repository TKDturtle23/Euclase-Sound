#pragma once

namespace Euclase {

    class CommandBuffer {
    public:
        virtual ~CommandBuffer() = default;

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        virtual void Reset() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void Draw(unsigned int vertices) = 0;

    protected:
        CommandBuffer() = default;
    };

}