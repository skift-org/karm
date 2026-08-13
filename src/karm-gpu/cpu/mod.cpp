module;

#include <karm/macros>

export module Karm.Gpu.Soft;

import Karm.Gpu;
import Karm.Drm;
import Karm.Core;
import Karm.Math;
import Karm.Logger;

import Karm.Gpu.Base;
import Karm.Gpu.Core;
import Karm.Gpu.Rast;

using namespace Karm::Literals;

namespace Karm::Gpu {

struct SoftBuffer : Buffer {
    Vec<u8> _buf;

    SoftBuffer(usize size) {
        _buf.resize(size);
    }

    MutBytes map() override {
        return _buf;
    }

    DevicePtr ptr() override {
        return DevicePtr{reinterpret_cast<u64>(_buf.buf())};
    }
};

struct SoftDepthStencilState : DepthStencilState {
    DepthStencilProps props;
};

struct SoftGraphicPipeline : Pipeline {
};

struct SoftComputePipeline : Pipeline {
};

struct SoftCommandBuffer : CommandBuffer {
    struct CopyBufferCommand {
        DevicePtr dest;
        DevicePtr src;
        usize size;
    };

    struct CopyBufferToTextureCommand {
        Rc<Texture> dest;
        DevicePtr src;
        BufferTextureCopyInfo infos;
    };

    struct CopyTextureToBufferCommand {
        DevicePtr dest;
        Rc<Texture> src;
        BufferTextureCopyInfo infos;
    };

    struct BarrierCommand {
        StageFlags before;
        StageFlags after;
    };

    struct PipelineCommand {
        Rc<Pipeline> pipeline;
    };

    struct DepthStencilStateCommand {
        Rc<SoftDepthStencilState> state;
    };

    struct ViewportCommand {
        Viewport viewport;
    };

    struct ScissorCommand {
        Math::Recti rect;
    };

    struct DispatchCommand {
        DevicePtr data;
        Math::Vec3u gridDimensions;
    };

    struct DispatchIndirect {
        DevicePtr data;
        DevicePtr gridDimensions;
    };

    struct BeginRenderPass {
        RenderPassProps const& props;
    };

    struct EndRenderPass {};

    struct FrontFaceCommand {
        FrontFace frontFace;
    };

    struct CullModeCommand {
        Cull cull;
    };

    struct DrawCommand {
        Opt<DevicePtr> vertexData;
        Opt<DevicePtr> fragmentData;
        usize vertexCount;
        usize instanceCount;
    };

    struct DrawIndexedInstancedCommand {
        DrawIndexedInstancedInfo args;
    };

    struct DrawIndexedInstancedIndirectCommand {
        DrawIndexedIndirectInfo args;
    };

    struct DrawIndexedInstancedIndirectMultiCommand {
        MultiDrawIndirectInfo args;
    };

    struct PushDebugCommand {
        String label;
    };

    struct PopDebugCommand {};

    using Command = Union<
        CopyBufferCommand,
        CopyBufferToTextureCommand,
        CopyTextureToBufferCommand,
        BarrierCommand,
        PipelineCommand,
        DepthStencilStateCommand,
        ViewportCommand,
        ScissorCommand,
        DispatchCommand,
        DispatchIndirect,
        BeginRenderPass,
        EndRenderPass,
        FrontFaceCommand,
        CullModeCommand,
        DrawCommand,
        DrawIndexedInstancedCommand,
        DrawIndexedInstancedIndirectCommand,
        DrawIndexedInstancedIndirectMultiCommand,
        PushDebugCommand,
        PopDebugCommand>;

    Vec<Command> commands;

    void copy(DevicePtr dest, DevicePtr src, usize size) override {
        commands.emplaceBack(CopyBufferCommand{dest, src, size});
    }

    void copy(Rc<Texture> dest, DevicePtr src, BufferTextureCopyInfo const& infos) override {
        commands.emplaceBack(CopyBufferToTextureCommand{dest, src, infos});
    }

    void copy(DevicePtr dest, Rc<Texture> src, BufferTextureCopyInfo const& infos) override {
        commands.emplaceBack(CopyTextureToBufferCommand{dest, src, infos});
    }

    void barrier(StageFlags before, StageFlags after) override {
        commands.emplaceBack(BarrierCommand{before, after});
    }

    void pipeline(Rc<Pipeline> pipeline) override {
        commands.emplaceBack(PipelineCommand{pipeline});
    }

    void depthStencilState(Rc<DepthStencilState> state) override {
        auto s = state.cast<SoftDepthStencilState>().unwrap();
        commands.emplaceBack(DepthStencilStateCommand{s});
    }

    void viewport(Viewport viewport) override {
        commands.emplaceBack(ViewportCommand{viewport});
    }

    void scissor(Math::Recti rect) override {
        commands.emplaceBack(ScissorCommand{rect});
    }

    void dispatch(DevicePtr data, Math::Vec3u gridDimensions) override {
        commands.emplaceBack(DispatchCommand{data, gridDimensions});
    }

    void dispatchIndirect(DevicePtr data, DevicePtr gridDimensions) override {
        commands.emplaceBack(DispatchIndirect{data, gridDimensions});
    }

    void beginRenderPass(RenderPassProps const& props) override {
        commands.emplaceBack(BeginRenderPass{props});
    }

    void endRenderPass() override {
        commands.emplaceBack(EndRenderPass{});
    }

    void frontFace(FrontFace frontFace) override {
        commands.emplaceBack(FrontFaceCommand{frontFace});
    }

    void cullMode(Cull cull) override {
        commands.emplaceBack(CullModeCommand{cull});
    }

    void draw(Opt<DevicePtr> vertexData, Opt<DevicePtr> fragmentData, usize vertexCount, usize instanceCount) override {
        commands.emplaceBack(DrawCommand{vertexData, fragmentData, vertexCount, instanceCount});
    }

    void drawIndexedInstanced(DrawIndexedInstancedInfo const& args) override {
        commands.emplaceBack(DrawIndexedInstancedCommand{args});
    }

    void drawIndexedInstancedIndirect(DrawIndexedIndirectInfo const& args) override {
        commands.emplaceBack(DrawIndexedInstancedIndirectCommand{args});
    }

    void drawIndexedInstancedIndirectMulti(MultiDrawIndirectInfo const& args) override {
        commands.emplaceBack(DrawIndexedInstancedIndirectMultiCommand{args});
    }

    void pushDebugGroup(Str label) override {
        commands.emplaceBack(PushDebugCommand{label});
    }

    void popDebugGroup() override {
        commands.emplaceBack(PopDebugCommand{});
    }

    void finalize() override {
        // no-op
    }
};

struct SoftExecutionContext {
    Opt<RenderPassProps> renderPass = NONE;
    Opt<Rc<Pipeline>> pipeline;
    Rasterizer::State pipelineState;

    void execute(SoftCommandBuffer::CopyBufferCommand const& cmd) {
        (void)cmd;
        logWarn("CopyBufferCommand is not implemented");
    }

    void execute(SoftCommandBuffer::CopyBufferToTextureCommand const& cmd) {
        (void)cmd;
        logWarn("CopyBufferToTextureCommand is not implemented");
    }

    void execute(SoftCommandBuffer::CopyTextureToBufferCommand const& cmd) {
        (void)cmd;
        logWarn("CopyTextureToBufferCommand is not implemented");
    }

    void execute(SoftCommandBuffer::BarrierCommand const& cmd) {
        (void)cmd;
        logWarn("BarrierCommand is not implemented");
    }

    void execute(SoftCommandBuffer::PipelineCommand const& cmd) {
        pipeline = Some(cmd.pipeline);
    }

    void execute(SoftCommandBuffer::DepthStencilStateCommand const& cmd) {
        pipelineState.depthStencil = cmd.state->props;
    }

    void execute(SoftCommandBuffer::ViewportCommand const& cmd) {
        pipelineState.viewport = cmd.viewport;
    }

    void execute(SoftCommandBuffer::ScissorCommand const& cmd) {
        pipelineState.scissor = Some(cmd.rect);
    }

    void execute(SoftCommandBuffer::DispatchCommand const& cmd) {
        (void)cmd;
        logWarn("DispatchCommand is not implemented");
    }

    void execute(SoftCommandBuffer::DispatchIndirect const& cmd) {
        (void)cmd;
        logWarn("DispatchIndirect is not implemented");
    }

    void execute(SoftCommandBuffer::BeginRenderPass const& cmd) {
        renderPass = Some(cmd.props);
    }

    void execute(SoftCommandBuffer::EndRenderPass const& cmd) {
        (void)cmd;
        renderPass = NONE;
    }

    void execute(SoftCommandBuffer::FrontFaceCommand const& cmd) {
        pipelineState.frontFace = cmd.frontFace;
    }

    void execute(SoftCommandBuffer::CullModeCommand const& cmd) {
        pipelineState.cullMode = cmd.cull;
    }

    void execute(SoftCommandBuffer::DrawCommand const& cmd) {
        (void)cmd;
        logWarn("DrawCommand is not implemented");
    }

    void execute(SoftCommandBuffer::DrawIndexedInstancedCommand const& cmd) {
        (void)cmd;
        logWarn("DrawIndexedInstancedCommand is not implemented");
    }

    void execute(SoftCommandBuffer::DrawIndexedInstancedIndirectCommand const& cmd) {
        (void)cmd;
        logWarn("DrawIndexedInstancedIndirectCommand is not implemented");
    }

    void execute(SoftCommandBuffer::DrawIndexedInstancedIndirectMultiCommand const& cmd) {
        (void)cmd;
        logWarn("DrawIndexedInstancedIndirectMultiCommand is not implemented");
    }

    void execute(SoftCommandBuffer::PushDebugCommand const& cmd) {
        (void)cmd;
        logWarn("PushDebugCommand is not implemented");
    }

    void execute(SoftCommandBuffer::PopDebugCommand const& cmd) {
        (void)cmd;
        logWarn("PopDebugCommand is not implemented");
    }

    void execute(SoftCommandBuffer const& buf) {
        for (auto const& cmd : buf.commands)
            cmd.visit([&](auto const& cmd) {
                execute(cmd);
            });
    }
};

struct SoftQueue : Queue {
    QueueType _type;

    SoftQueue(QueueType type) : _type(type) {}

    void submit(Slice<Rc<CommandBuffer>> commandBuffers, Slice<SemaphoreInfo> waitSemaphores, Slice<SemaphoreInfo> signalSemaphores) override {
        SoftExecutionContext ctx;
        (void)waitSemaphores;
        (void)signalSemaphores;
        for (auto& buf : commandBuffers)
            ctx.execute(*buf.cast<SoftCommandBuffer>().unwrap("invalid command buffer type"));
    }

    Rc<CommandBuffer> startCommandRecording() override {
        return makeRc<SoftCommandBuffer>();
    }

    void onCompleted(Func<void()> fn) override {
        (void)fn;
    }

    void cancel(Slice<Rc<CommandBuffer>> commandBuffers) override {
        (void)commandBuffers;
    }
};

struct SoftDevice : Device {
    Str backend() override {
        return "software"s;
    }

    Rc<Pipeline> createComputePipeline(ShaderSource compute, Slice<SpecializationConstant> constants) override;

    Rc<DepthStencilState> createDepthStencilState(DepthStencilProps const& props) override;

    Rc<Pipeline> createGraphicPipeline(ShaderSource vertex, ShaderSource fragment, RasterProps const& props, Slice<SpecializationConstant> constants) override;

    Rc<Queue> createQueue(QueueType type) override {
        return makeRc<SoftQueue>(type);
    }

    Rc<Texture> createRwTextureView(TextureViewProps const& props) override;

    Rc<Sampler> createSampler(SamplerProps const& props) override;

    Rc<Semaphore> createSemaphore(u64 initial) override;

    Rc<Semaphore> createSemaphore(Rc<Drm::Sync> from) override;

    Rc<Texture> createTexture(Opt<DevicePtr> location, TextureProps const& props) override;

    Rc<Texture> createTexture(Rc<Drm::Buffer> from, UsageFlags usage) override;

    Rc<Texture> createTextureView(TextureViewProps const& props) override;

    Rc<Buffer> createBuffer(usize size, Memory memory) override {
        (void)memory;
        return makeRc<SoftBuffer>(size);
    }

    Rc<Buffer> createBuffer(usize size, usize align, Memory memory) override {
        (void)align;
        (void)memory;
        return makeRc<SoftBuffer>(size);
    }

    TextureSizeAlign textureSizeAlign(TextureProps const& props) override;

    void waitForIdle() override {
        // no-op
    }
};

} // namespace Karm::Gpu
