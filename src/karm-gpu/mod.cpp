export module Karm.Gpu;

import Karm.Core;
import Karm.Math;
import Karm.Drm;
import Karm.Gpu.Base;

/// Based off Loon GPU
/// MIT License
/// Copyright (c) 2026 R. Kevin Gibson
/// https://github.com/rkevingibson/loon_gpu/tree/main

namespace Karm::Gpu {

export struct Device;
export struct Pipeline;
export struct Texture;
export struct DepthStencilState;
export struct Semaphore;
export struct Queue;
export struct CommandBuffer;
export struct Buffer;
export struct Sampler;

/// An address in GPU memory, usable from shaders and copy commands.
export using DevicePtr = Distinct<u64, struct _DevicePtr>;

/// Type of memory an allocation lives in, trading CPU visibility for GPU speed.
export enum struct Memory : u8 {
    DEFAULT,  //< CPU visible memory, optimized for writing to from the CPU and reading from GPU
    DEVICE,   //< GPU-only memory, not visible from the CPU
    READBACK, //< CPU visible memory, optimized for reading from the CPU.
};

/// Dimensionality and layout of a texture resource.
export enum struct TextureType : u8 {
    TEX_1D,
    TEX_2D,
    TEX_3D,
    TEX_CUBE,
    TEX_2D_ARRAY,
    TEX_CUBE_ARRAY,
};

export enum struct Format : u32 {
    NONE,
#define FORMAT(NAME) NAME,
#include "defs/formats.inc"

#undef FORMAT
    _LEN,
};

/// Flags describing how a texture may be used.
export enum struct UsageFlags : u16 {
    NONE = 0,
    SAMPLED = 1 << 0,
    STORAGE = 1 << 1,
    COLOR_ATTACHMENT = 1 << 2,
    DEPTH_STENCIL_ATTACHMENT = 1 << 3,
    TRANSFER_SRC = 1 << 4,
    TRANSFER_DST = 1 << 5,
};

/// Pipeline stages used to express synchronization scopes for barriers and semaphores.
export enum struct StageFlags : u16 {
    NONE = 0,
    INDIRECT_ARGUMENTS = 1 << 0,
    TRANSFER = 1 << 1,
    COMPUTE = 1 << 2,
    RASTER_COLOR_OUT = 1 << 3,
    PIXEL_SHADER = 1 << 4,
    FRAGMENT_TESTS = 1 << 5,
    VERTEX_SHADER = 1 << 6,
    HOST = 1 << 7,
};

/// Kind of work a queue is able to execute.
export enum struct QueueType : u8 {
    DEFAULT,  //< Queue capable of doing graphics, compute and transfer work
    COMPUTE,  //< Dedicated compute-only queue
    TRANSFER, //< Dedicated transfer-only queue

    _LEN,
};

/// Coordinate space used when sampling a texture.
export enum struct SamplerCoords : u8 {
    NORMALIZED, ///< Coordinates lie in [0,1] range
    PIXEL,      ///< Coordinates lie in [0, width] and [0, height] range

    _LEN,
};

/// Filtering applied when a texture is minified, magnified, or sampled between mips.
export enum struct SamplerFilter : u8 {
    NEAREST,
    LINEAR,

    _LEN,
};

/// How texture coordinates outside the [0,1] range are resolved.
export enum struct SamplerAddressing : u8 {
    CLAMP_TO_EDGE,
    REPEAT,
    MIRRORED,

    _LEN,
};

/// Bit width of the indices in an index buffer.
export enum struct IndexType : u8 {
    U16,
    U32,

    _LEN,
};

/// An RGBA color value.
export using Color = Math::Vec4f;

/// Description of how a sampler filters and addresses texture reads.
export struct SamplerProps {
    SamplerCoords coord = SamplerCoords::NORMALIZED;
    SamplerFilter filter = SamplerFilter::NEAREST;
    SamplerAddressing address = SamplerAddressing::CLAMP_TO_EDGE;
    f32 maxAnisotropy = 1.0f;
};

/// Options used when creating a device.
export struct DeviceProps {
    Opt<Str> preferredBackend = NONE;
};

/// Description of how source and destination color/alpha are blended.
export struct BlendProps {
    Blend colorOp = Blend::ADD;
    Factor srcColorFactor = Factor::ONE;
    Factor dstColorFactor = Factor::ZERO;
    Blend alphaOp = Blend::ADD;
    Factor srcAlphaFactor = Factor::ONE;
    Factor dstAlphaFactor = Factor::ZERO;
    u8 colorWriteMask = 0xf;
};

/// Format and blend state of a single color attachment targeted by a pipeline.
export struct ColorTarget {
    Format format = Format::NONE;
    BlendProps blendState = {};
};

/// Fixed-function rasterization state used to create a graphics pipeline.
export struct RasterProps {
    Topology topology = Topology::TRIANGLE_LIST;
    bool alphaToCoverage = false;
    u8 sampleCount = 1;
    Format depthFormat = Format::NONE;
    Format stencilFormat = Format::NONE;
    Slice<ColorTarget> colorTargets = {};
};

/// A texture attachment of a render pass along with its load/store behavior.
export struct RenderAttachment {
    Opt<Rc<Texture>> texture = NONE;
    LoadOp loadOp = LoadOp::CLEAR;
    StoreOp storeOp = StoreOp::STORE;
    Color clearColor = {};
};

/// Attachments and render area describing a render pass.
export struct RenderPassProps {
    Slice<RenderAttachment> colorAttachments = {};
    RenderAttachment depthAttachment = {};
    RenderAttachment stencilAttachment = {};
    Math::Recti renderArea = {};
};

/// Description of a texture's type, size, format, and allowed usages.
export struct TextureProps {
    TextureType type = TextureType::TEX_2D;
    Math::Vec3u dimensions;
    u32 mipCount = 1;
    u32 layerCount = 1;
    u32 sampleCount = 1;
    Format format = Format::NONE;
    UsageFlags usage = UsageFlags::NONE;
};

/// Description of a view over a subrange of a texture's mips and layers.
export struct TextureViewProps {
    Rc<Texture> texture;
    Format format = Format::NONE;
    u8 baseMip = 0;
    u8 mipCount = 1;
    u16 baseLayer = 0;
    u16 layerCount = 1;
};

/// Memory size and alignment requirements of a texture.
export struct TextureSizeAlign {
    usize size;
    usize align;
};

/// A compile-time constant value bound to a shader specialization slot.
export struct SpecializationConstant {
    u32 constantId;
    Union<bool, u8, u16, u32, i8, i16, i32, f32> value;
};

/// Shader bytecode along with the entry point to execute.
export struct ShaderSource {
    Bytes source;
    Str entryPoint;
};

/// A semaphore paired with the value and stage to wait on or signal at.
export struct SemaphoreInfo {
    Rc<Semaphore> semaphore;
    u64 value;
    StageFlags stage = StageFlags::NONE; // What stage must be blocked on the wait operation
};

/// Layout information for copying data between a buffer and a texture region.
export struct BufferTextureCopyInfo {
    Math::Vec3u imageExtent;

    ///< Number of pixels between subsequent rows of data in the buffer. If 0,
    ///< treated as equal to imageExtent.x. Otherwise, should be >= imageExtent.x
    u32 bufferRowPixelsStride = 0;

    ///< Number of rows in a plane of image in the buffer. If 0, treated as equal
    ///< to imageExtent.y. Otherwise, should be >= imageExtent.y.
    u32 bufferPlaneRowsStride = 0;

    Math::Vec3u textureImageOffset{0, 0, 0};

    u8 baseMip = 0;
    u8 baseLayer = 0;
};

/// Arguments for an indexed, instanced draw call.
export struct DrawIndexedInstancedInfo {
    DevicePtr vertexDataGpu;
    DevicePtr fragmentDataGpu;
    DevicePtr indicesGpu;
    u32 indexCount;
    u32 instanceCount = 1;
    IndexType type = IndexType::U16;
};

/// Arguments for an indexed draw whose parameters are read from GPU memory.
export struct DrawIndexedIndirectInfo {
    DevicePtr vertexDataGpu;
    DevicePtr fragmentDataGpu;
    DevicePtr indicesGpu;
    DevicePtr argsGpu;
    IndexType type = IndexType::U16;
};

/// Arguments for multiple indirect indexed draws with a GPU-provided draw count.
export struct MultiDrawIndirectInfo {
    DevicePtr vertexDataGpu;
    DevicePtr pixelDataGpu;
    DevicePtr indicesGpu;
    DevicePtr argsGpu;
    DevicePtr drawCountGpu;
    u32 maxDraws;
    IndexType type = IndexType::U16;
};

/// GPU-side layout of the arguments consumed by an indirect indexed draw.
export struct DrawIndexedIndirectGpuArgs {
    u32 indexCount;
    u32 instanceCount;
    u32 firstIndex;
    i32 vertexOffset;
    u32 firstInstance;
};

/// A logical GPU device, entry point for creating all other GPU resources.
export struct Device {
    /// Create a device object
    static Res<Rc<Device>> create(DeviceProps const& props) {
        (void)props;
        return Error::notImplemented("Device::create");
    }

    virtual ~Device() = default;

    /// Name of the underlying graphics API backend.
    virtual Str backend() = 0;

    /// Block until any pending work on the GPU is completed.
    virtual void waitForIdle() = 0;

    /// Allocate a buffer of the given size in the requested memory type.
    virtual Rc<Buffer> createBuffer(usize size, Memory memory = Memory::DEFAULT) = 0;

    /// Allocate a buffer with an explicit alignment in the requested memory type.
    virtual Rc<Buffer> createBuffer(usize size, usize align, Memory memory = Memory::DEFAULT) = 0;

    /// Query the size and alignment a texture with this description would require.
    virtual TextureSizeAlign textureSizeAlign(TextureProps const& props) = 0;

    /// Create a texture, optionally placed at an existing GPU memory location.
    virtual Rc<Texture> createTexture(Opt<DevicePtr> location, TextureProps const& props) = 0;

    virtual Rc<Texture> createTexture(Rc<Drm::Buffer> from, UsageFlags usage) = 0;

    /// Create a view into a texture
    virtual Rc<Texture> createTextureView(TextureViewProps const& props) = 0;

    /// Create a view into a texture
    virtual Rc<Texture> createRwTextureView(TextureViewProps const& props) = 0;

    /// Create a sampler
    virtual Rc<Sampler> createSampler(SamplerProps const& props) = 0;

    /// Create an immutable depth/stencil state object from a description.
    virtual Rc<DepthStencilState> createDepthStencilState(DepthStencilProps const& props) = 0;

    /// Create a compute pipeline from a compute shader.
    virtual Rc<Pipeline> createComputePipeline(ShaderSource compute, Slice<SpecializationConstant> constants = {}) = 0;

    /// Create a graphics pipeline from vertex and fragment shaders and raster state.
    virtual Rc<Pipeline> createGraphicPipeline(ShaderSource vertex, ShaderSource fragment, RasterProps const& props, Slice<SpecializationConstant> constants = {}) = 0;

    /// Create a timeline semaphore with the given initial value.
    virtual Rc<Semaphore> createSemaphore(u64 initial) = 0;

    virtual Rc<Semaphore> createSemaphore(Rc<Drm::Sync> from) = 0;

    /// Get a queue of the requested type for submitting work.
    virtual Rc<Queue> createQueue(QueueType type) = 0;
};

/// A linear allocation of GPU-addressable memory.
export struct Buffer {
    virtual ~Buffer() = default;

    virtual DevicePtr ptr() = 0;

    virtual MutBytes map() = 0;
};

/// Immutable depth/stencil test configuration bound during rendering.
export struct DepthStencilState {
    virtual ~DepthStencilState() = default;
};

/// An image resource living in GPU memory.
export struct Texture {
    virtual ~Texture() = default;

    virtual DevicePtr ptr() = 0;
};

/// A sampler living in GPU memory
export struct Sampler {
    virtual ~Sampler() = default;

    virtual DevicePtr ptr() = 0;
};

/// A compiled compute or graphics pipeline state object.
export struct Pipeline {
    virtual ~Pipeline() = default;
};

/// A timeline semaphore used to synchronize GPU and CPU work.
export struct Semaphore {
    virtual ~Semaphore() = default;

    /// Block the CPU until the semaphore reaches the given value.
    virtual void wait(Rc<Semaphore> sema, u64 value) = 0;
};

/// Records GPU commands for later submission to a queue.
export struct CommandBuffer {
    virtual ~CommandBuffer() = default;

    /// Copy a range of bytes between two GPU memory locations.
    virtual void copy(DevicePtr dest, DevicePtr src, usize size) = 0;

    /// Copy data from GPU memory into a texture region.
    virtual void copy(Rc<Texture> dest, DevicePtr src, BufferTextureCopyInfo const& infos) = 0;

    /// Copy a texture region into GPU memory.
    virtual void copy(DevicePtr src, Rc<Texture> dest, BufferTextureCopyInfo const& infos) = 0;

    /// Insert a memory barrier between two sets of pipeline stages.
    virtual void barrier(StageFlags before, StageFlags after) = 0;

    /// Bind a compute or graphics pipeline for subsequent commands.
    virtual void pipeline(Rc<Pipeline> pipeline) = 0;

    /// Bind a depth/stencil state for subsequent draws.
    virtual void depthStencilState(Rc<DepthStencilState> state) = 0;

    /// Set the viewport transform for subsequent draws.
    virtual void viewport(Viewport viewport) = 0;

    /// Set the scissor rectangle for subsequent draws.
    virtual void scissor(Math::Recti rect) = 0;

    /// Dispatch a compute grid, passing a GPU pointer as shader data.
    virtual void dispatch(DevicePtr data, Math::Vec3u gridDimensions) = 0;

    /// Dispatch a compute grid whose dimensions are read from GPU memory.
    virtual void dispatchIndirect(DevicePtr data, DevicePtr gridDimensions) = 0;

    /// Begin a render pass with the given attachments and render area.
    virtual void beginRenderPass(RenderPassProps const& props) = 0;

    /// End the current render pass.
    virtual void endRenderPass() = 0;

    /// Set which winding order is considered front-facing.
    virtual void frontFace(FrontFace frontFace) = 0;

    /// Set which triangle faces are culled.
    virtual void cullMode(Cull cull) = 0;

    /// Draw non-indexed primitives, passing GPU pointers as shader data.
    virtual void draw(Opt<DevicePtr> vertexData, Opt<DevicePtr> fragmentData, usize vertexCount, usize instanceCount) = 0;

    /// Draw indexed, instanced primitives.
    virtual void drawIndexedInstanced(DrawIndexedInstancedInfo const& args) = 0;

    /// Draw indexed primitives with arguments read from GPU memory.
    virtual void drawIndexedInstancedIndirect(DrawIndexedIndirectInfo const& args) = 0;

    /// Issue multiple indirect indexed draws with a GPU-provided draw count.
    virtual void drawIndexedInstancedIndirectMulti(MultiDrawIndirectInfo const& args) = 0;

    /// Begin a labeled debug group for graphics debuggers.
    virtual void pushDebugGroup(Str label) = 0;

    /// End the current debug group.
    virtual void popDebugGroup() = 0;

    /// Finish recording, making the command buffer ready for submission.
    virtual void finalize() = 0;
};

/// A GPU submission queue that executes recorded command buffers.
export struct Queue {
    virtual ~Queue() = default;

    /// Submit command buffers, waiting on and signaling the given semaphores.
    virtual void submit(
        Slice<Rc<CommandBuffer>> commandBuffers,
        Slice<SemaphoreInfo> waitSemaphores = {},
        Slice<SemaphoreInfo> signalSemaphores = {}
    ) = 0;

    /// Discard recorded command buffers without executing them.
    virtual void cancel(Slice<Rc<CommandBuffer>> commandBuffers) = 0;

    /// Invoke a callback once all currently submitted work has completed.
    virtual void onCompleted(Func<void()> fn) = 0;

    /// Begin recording a new command buffer on this queue.
    virtual Rc<CommandBuffer> startCommandRecording() = 0;
};

} // namespace Karm::Gpu
