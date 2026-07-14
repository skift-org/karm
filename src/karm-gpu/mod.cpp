export module Karm.Gpu;

import Karm.Core;
import Karm.Math;

/// Based off Loon GPU
/// MIT License
/// Copyright (c) 2026 R. Kevin Gibson
/// https://github.com/rkevingibson/loon_gpu/tree/main
namespace Karm::Gpu {

export struct Device;
export struct Pipeline;
export struct Texture;
export struct TextureHeap;
export struct DepthStancilState;
export struct Semaphore;
export struct Queue;
export struct CommandBuffer;
export struct Buffer;

using DevicePtr = Distinct<u64, struct _DevicePtrTag>;

export enum struct Nil {
    NIL
};

export using enum Nil;

template <typename T>
struct Handle : Distinct<u64, T> {
    using Distinct<u64, T>::Distinct;

    Handle(Nil) : Distinct<u64, T>(0) {}

    bool operator==(Nil const&) const {
        return Distinct<u64, T>::value() == 0;
    }
};

export enum struct Memory : u8 {
    DEFAULT,  //< CPU visible memory, optimized for writing to from the CPU and reading from GPU
    DEVICE,   //< GPU-only memory, not visible from the CPU
    READBACK, //< CPU visible memory, optimized for reading from the CPU.
};

export enum struct FrontFace : u8 {
    COUNTER_CLOCKWISE = 0,
    CLOCKWISE,
};

export enum struct Cull : u8 {
    FRONT,
    BACK,
    NONE,
};

export enum struct DepthFlags : u8 {
    NONE = 0,
    READ = 1 << 0,
    WRITE = 1 << 1,
};

/// Comparison operation for depth and stencil testing
export enum struct Op : u8 {
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS,
};

/// Operations for stencil buffers
export enum struct StencilOp : u8 {
    KEEP,
    ZERO,
    REPLACE,
    INCREMENT_CLAMP,
    DECREMENT_CLAMP,
    INVERT,
    INCREMENT_WRAP,
    DECREMENT_WRAP,
};

/// Operations for color/alpha blending.
export enum struct Blend : u8 {
    ADD,
    SUBTRACT,
    REV_SUBTRACT,
    MIN,
    MAX,
};

/// Blend factors for color/alpha blending.
export enum struct Factor : u8 {
    ZERO,
    ONE,
    SRC_COLOR,
    DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
};

/// Input primitive to be used for a render pass.
export enum struct Topology : u8 {
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
};

export enum struct TextureType : u8 {
    TEX_1D,
    TEX_2D,
    TEX_3D,
    TEX_CUBE,
    TEX_2D_ARRAY,
    TEX_CUBE_ARRAY,
};

enum struct Format : u32 {
    NONE = 0x00000000,
    R8_UNORM = 0x00000001,
    R8_SNORM = 0x00000002,
    R8_UINT = 0x00000003,
    R8_SINT = 0x00000004,
    R16_UNORM = 0x00000005,
    R16_SNORM = 0x00000006,
    R16_UINT = 0x00000007,
    R16_SINT = 0x00000008,
    R16_FLOAT = 0x00000009,
    RG8_UNORM = 0x0000000A,
    RG8_SNORM = 0x0000000B,
    RG8_UINT = 0x0000000C,
    RG8_SINT = 0x0000000D,
    R32_FLOAT = 0x0000000E,
    R32_UINT = 0x0000000F,
    R32_SINT = 0x00000010,
    RG16_UNORM = 0x00000011,
    RG16_SNORM = 0x00000012,
    RG16_UINT = 0x00000013,
    RG16_SINT = 0x00000014,
    RG16_FLOAT = 0x00000015,
    RGBA8_UNORM = 0x00000016,
    RGBA8_UNORM_SRGB = 0x00000017,
    RGBA8_SNORM = 0x00000018,
    RGBA8_UINT = 0x00000019,
    RGBA8_SINT = 0x0000001A,
    BGRA8_UNORM = 0x0000001B,
    BGRA8_UNORM_SRGB = 0x0000001C,
    RGB10_A2_UINT = 0x0000001D,
    RGB10_A2_UNORM = 0x0000001E,
    RG11_B10_UFLOAT = 0x0000001F,
    RGB9_E5_UFLOAT = 0x00000020,
    RG32_FLOAT = 0x00000021,
    RG32_UINT = 0x00000022,
    RG32_SINT = 0x00000023,
    RGBA16_UNORM = 0x00000024,
    RGBA16_SNORM = 0x00000025,
    RGBA16_UINT = 0x00000026,
    RGBA16_SINT = 0x00000027,
    RGBA16_FLOAT = 0x00000028,
    RGBA32_FLOAT = 0x00000029,
    RGBA32_UINT = 0x0000002A,
    RGBA32_SINT = 0x0000002B,
    STENCIL8 = 0x0000002C,
    DEPTH16_UNORM = 0x0000002D,
    DEPTH24_PLUS = 0x0000002E,
    DEPTH24_PLUS_STENCIL8 = 0x0000002F,
    DEPTH32_FLOAT = 0x00000030,
    DEPTH32_FLOAT_STENCIL8 = 0x00000031,

    // TODO: Implement compressed texture formats
    // BC1_RGBA_UNORM = 0x00000032,
    // BC1_RGBA_UNORM_SRGB = 0x00000033,
    // BC2_RGBA_UNORM = 0x00000034,
    // BC2_RGBA_UNORM_SRGB = 0x00000035,
    // BC3_RGBA_UNORM = 0x00000036,
    // BC3_RGBA_UNORM_SRGB = 0x00000037,
    // BC4_R_UNORM = 0x00000038,
    // BC4_R_SNORM = 0x00000039,
    // BC5_RG_UNORM = 0x0000003A,
    // BC5_RG_SNORM = 0x0000003B,
    // BC6_HRGB_UFLOAT = 0x0000003C,
    // BC6_HRGB_FLOAT = 0x0000003D,
    // BC7_RGBA_UNORM = 0x0000003E,
    // BC7_RGBA_UNORM_SRGB = 0x0000003F,
    // ETC2_RGB8_UNORM = 0x00000040,
    // ETC2_RGB8_UNORM_SRGB = 0x00000041,
    // ETC2_RGB8_A1_UNORM = 0x00000042,
    // ETC2_RGB8_A1_UNORM_SRGB = 0x00000043,
    // ETC2_RGBA8_UNORM = 0x00000044,
    // ETC2_RGBA8_UNORM_SRGB = 0x00000045,
    // EACR11_UNORM = 0x00000046,
    // EACR11_SNORM = 0x00000047,
    // EACRG11_UNORM = 0x00000048,
    // EACRG11_SNORM = 0x00000049,
    // ASTC4X4_UNORM = 0x0000004A,
    // ASTC4X4_UNORM_SRGB = 0x0000004B,
    // ASTC5X4_UNORM = 0x0000004C,
    // ASTC5X4_UNORM_SRGB = 0x0000004D,
    // ASTC5X5_UNORM = 0x0000004E,
    // ASTC5X5_UNORM_SRGB = 0x0000004F,
    // ASTC6X5_UNORM = 0x00000050,
    // ASTC6X5_UNORM_SRGB = 0x00000051,
    // ASTC6X6_UNORM = 0x00000052,
    // ASTC6X6_UNORM_SRGB = 0x00000053,
    // ASTC8X5_UNORM = 0x00000054,
    // ASTC8X5_UNORM_SRGB = 0x00000055,
    // ASTC8X6_UNORM = 0x00000056,
    // ASTC8X6_UNORM_SRGB = 0x00000057,
    // ASTC8X8_UNORM = 0x00000058,
    // ASTC8X8_UNORM_SRGB = 0x00000059,
    // ASTC10X5_UNORM = 0x0000005A,
    // ASTC10X5_UNORM_SRGB = 0x0000005B,
    // ASTC10X6_UNORM = 0x0000005C,
    // ASTC10X6_UNORM_SRGB = 0x0000005D,
    // ASTC10X8_UNORM = 0x0000005E,
    // ASTC10X8_UNORM_SRGB = 0x0000005F,
    // ASTC10X10_UNORM = 0x00000060,
    // ASTC10X10_UNORM_SRGB = 0x00000061,
    // ASTC12X10_UNORM = 0x00000062,
    // ASTC12X10_UNORM_SRGB = 0x00000063,
    // ASTC12X12_UNORM = 0x00000064,
    // ASTC12X12_UNORM_SRGB = 0x00000065,

    _LEN,
};

export enum struct UsageFlags : u16 {
    NONE = 0,
    SAMPLED = 1 << 0,
    STORAGE = 1 << 1,
    COLOR_ATTACHMENT = 1 << 2,
    DEPTH_STENCIL_ATTACHMENT = 1 << 3,
    TRANSFER_SRC = 1 << 4,
    TRANSFER_DST = 1 << 5,
};

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

export enum struct LoadOp : u8 {
    UNDEFINED,
    LOAD,
    CLEAR,
};

export enum struct StoreOp : u8 {
    UNDEFINED,
    STORE,
    DISCARD,
};

export enum struct QueueType : u8 {
    DEFAULT,  //< Queue capable of doing graphics, compute and transfer work
    COMPUTE,  //< Dedicated compute-only queue
    TRANSFER, //< Dedicated transfer-only queue

    _LEN,
};

export enum struct PresentMode : u8 {
    IMMEDIATE,
    MAILBOX,
    FIFO,
    FIFO_RELAXED,

    _LEN,
};

export enum struct SurfaceStatus : u8 {
    SUCCESS,
    SUBOPTIMAL,
    OUT_OF_DATE,
    ERROR,

    _LEN,
};

export enum struct SamplerCoords : u8 {
    NORMALIZED, ///< Coordinates lie in [0,1] range
    PIXEL,      ///< Coordinates lie in [0, width] and [0, height] range

    _LEN,
};

export enum struct SamplerFilter : u8 {
    NEAREST,
    LINEAR,

    _LEN,
};

export enum struct SamplerAddressing : u8 {
    CLAMP_TO_EDGE,
    REPEAT,
    MIRRORED,

    _LEN,
};

export enum struct IndexType : u8 {
    U16,
    U32,

    _LEN,
};

export using Color = Math::Vec4f;

export struct Stencil {
    Op test = Op::ALWAYS;
    StencilOp failOp = StencilOp::KEEP;
    StencilOp passOp = StencilOp::KEEP;
    StencilOp depthFailOp = StencilOp::KEEP;
    u8 reference = 0;
};

/// Descriptor for a sampler object.
export struct SamplerDesc {
    SamplerCoords coord = SamplerCoords::NORMALIZED;
    SamplerFilter filter = SamplerFilter::NEAREST;
    SamplerAddressing address = SamplerAddressing::CLAMP_TO_EDGE;
    f32 maxAnisotropy = 1.0f;
};

/// Descriptor for a texture object.
export struct DeviceDesc {
    Opt<Str> preferredBackend = NONE;
    usize windowHandle = 0;
    usize instanceHandle = 0;
};

export struct DepthStencilDesc {
    DepthFlags depthMode = DepthFlags::NONE;
    Op depthTest = Op::ALWAYS;
    f32 depthBias = 0.0f;
    f32 depthBiasSlopeFactor = 0.0f;
    f32 depthBiasClamp = 0.0f;
    u8 stencilReadMask = 0xff;
    u8 stencilWriteMask = 0xff;
    Stencil stencilFront;
    Stencil stencilBack;
};

export struct BlendDesc {
    Blend colorOp = Blend::ADD;
    Factor srcColorFactor = Factor::ONE;
    Factor dstColorFactor = Factor::ZERO;
    Blend alphaOp = Blend::ADD;
    Factor srcAlphaFactor = Factor::ONE;
    Factor dstAlphaFactor = Factor::ZERO;
    u8 colorWriteMask = 0xf;
};

export struct ColorTarget {
    Format format = Format::NONE;
    BlendDesc blendState = {};
};

export struct RasterDesc {
    Topology topology = Topology::TRIANGLE_LIST;
    bool alphaToCoverage = false;
    u8 sampleCount = 1;
    Format depthFormat = Format::NONE;
    Format stencilFormat = Format::NONE;
    Slice<ColorTarget> colorTargets = {};
};

export struct RenderAttachment {
    Handle<Texture> texture = NIL;
    LoadOp loadOp;
    StoreOp storeOp;
    Color clearColor;
};

export struct RenderPassDesc {
    Slice<RenderAttachment> colorAttachments;
    RenderAttachment depthAttachment;
    RenderAttachment stencilAttachment;
    Math::Recti renderArea;
};

export struct TextureDesc {
    TextureType type = TextureType::TEX_2D;
    Math::Vec3u dimensions;
    u32 mipCount = 1;
    u32 layerCount = 1;
    u32 sampleCount = 1;
    Format format = Format::NONE;
    UsageFlags usage = UsageFlags::NONE;
};

export struct TextureHeapDesc {
    u32 textureCount = 0;
    u32 rwTextureCount = 0;
    u32 samplerCount = 0;
};

export struct TextureViewDesc {
    Handle<Texture> texture;
    Format format = Format::NONE;
    u8 baseMip = 0;
    u8 mipCount = 1;
    u16 baseLayer = 0;
    u16 layerCount = 1;
};

export struct TextureSizeAlign {
    usize size;
    usize align;
};

export struct SpecializationConstant {
    u32 constantId;
    Union<bool, u8, u16, u32, i8, i16, i32, f32> value;
};

export struct ShaderSource {
    Bytes source;
    Str entryPoint;
};

export struct SurfaceCapabilities {
    UsageFlags usages;
    Slice<Format> formats;
    Slice<PresentMode> presentModes;
};

export struct SurfaceConfiguration {
    Format format;
    UsageFlags usages;
    u32 width;
    u32 height;
    PresentMode presentMode;
};

export struct SurfaceTextureInfo {
    SurfaceStatus status;
    Handle<Texture> texture;
};

export struct SemaphoreInfo {
    Handle<Semaphore> semaphore;
    u64 value;
    StageFlags stage = StageFlags::NONE; // What stage must be blocked on the wait operation
};

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

export struct DrawIndexedInstancedInfo {
    DevicePtr vertexDataGpu;
    DevicePtr fragmentDataGpu;
    DevicePtr indicesGpu;
    u32 indexCount;
    u32 instanceCount = 1;
    IndexType type = IndexType::U16;
};

export struct DrawIndexedIndirectInfo {
    DevicePtr vertexDataGpu;
    DevicePtr fragmentDataGpu;
    DevicePtr indicesGpu;
    DevicePtr argsGpu;
    IndexType type = IndexType::U16;
};

export struct MultiDrawIndirectInfo {
    DevicePtr vertexDataGpu;
    DevicePtr pixelDataGpu;
    DevicePtr indicesGpu;
    DevicePtr argsGpu;
    DevicePtr drawCountGpu;
    u32 maxDraws;
    IndexType type = IndexType::U16;
};

export struct DrawIndexedIndirectGpuArgs {
    u32 indexCount;
    u32 instanceCount;
    u32 firstIndex;
    i32 vertexOffset;
    u32 firstInstance;
};

export template <typename T>
struct Ressource {
    Device& _device;
    Handle<T> _handle;

    ~Ressource();

    Device& device() { return _device; }

    Handle<T> handle() const { return _handle; }
};

export struct Device {
    // Create a device object
    static Res<Device> create(DeviceDesc const& desc);

    virtual ~Device() = default;

    Str backend();

    // Block until any pending work on the GPU is completed.
    void waitForIdle();

    // Get the surface capabilities for a device.
    SurfaceCapabilities surfaceCapabilities();

    // Configures the presentation surface.
    Res<> configureSurface(SurfaceConfiguration const& c);

    void unconfigureSurface();

    SurfaceTextureInfo currentTexture();

    SurfaceStatus present(Handle<Queue> queue);

    Buffer malloc(usize size, Memory memory = Memory::DEFAULT);

    Buffer malloc(usize size, usize align, Memory memory = Memory::DEFAULT);

    void free(Handle<Buffer> handle);

    TextureSizeAlign textureSizeAlign(TextureDesc const& desc);

    Texture createTexture(TextureDesc const& desc, Opt<DevicePtr> location);

    void free(Handle<Texture> handle);

    DepthStancilState createDepthStancilState(DepthStencilDesc const& desc);

    Pipeline createComputePipeline(ShaderSource compute, Slice<SpecializationConstant> constants = {});

    Pipeline createGraphicPipeline(ShaderSource vertex, ShaderSource fragment, RasterDesc const& desc, Slice<SpecializationConstant> constants = {});

    void free(Handle<Pipeline>);

    Semaphore createSemaphore(u64 initial);

    void free(Handle<Semaphore>);

    void wait(Handle<Semaphore> sema, u64 value);

    Rc<Queue> createQueue(QueueType type);
};

template <typename T>
Ressource<T>::~Ressource() {
    if (_handle != NIL)
        _device.free(_handle);
}

export struct DepthStancilState : Ressource<DepthStancilState> {};

export struct Texture : Ressource<Texture> {};

export struct Pipeline : Ressource<Pipeline> {};

export struct Semaphore : Ressource<Semaphore> {};

export struct Buffer : Ressource<Buffer> {
    void* host = nullptr;
};

export struct CommandBuffer {
    virtual ~CommandBuffer() = default;

    void copy(DevicePtr dest, DevicePtr src, usize size);

    void copy(Handle<Texture> dest, DevicePtr src, BufferTextureCopyInfo const& infos);

    void copy(DevicePtr src, Handle<Texture> dest, BufferTextureCopyInfo const& infos);

    void barrier(StageFlags before, StageFlags after);

    void pipeline(Handle<Pipeline> pipeline);

    void depthStencilState(Handle<DepthStancilState> state);

    void viewport(Math::Recti rect);

    void scissor(Math::Recti rect);

    void dispatch(DevicePtr data, Math::Vec3u gridDimensions);

    void dispatchIndirect(DevicePtr data, DevicePtr gridDimensions);

    void beginRenderPass(RenderPassDesc const& desc);
    void endRenderPass();

    void frontFace(FrontFace frontFace);

    void cullMode(Cull cull);

    void draw(DevicePtr vertexData, DevicePtr fragmentData, usize vertexCount, usize instanceCount);

    void drawIndexedInstanced(DrawIndexedInstancedInfo const& args);

    void drawIndexedInstancedIndirect(DrawIndexedIndirectInfo const& args);

    void drawIndexedInstancedIndirectMulti(MultiDrawIndirectInfo const& args);

    void waitForSurfaceTexture();

    void signalSurfaceTexture();

    void pushDebugGroup(Str label);

    void popDebugGroup();

    void finalize();
};

export struct Queue {
    virtual ~Queue() = default;

    void submit(
        Slice<Rc<CommandBuffer>> commandBuffers,
        Slice<SemaphoreInfo> waitSemaphores = {},
        Slice<SemaphoreInfo> signalSemaphores = {}
    );

    void cancel(Slice<Rc<CommandBuffer>> commandBuffers);

    void onCompleted(Func<void()> fn);

    Rc<CommandBuffer> startCommandRecording();
};

} // namespace Karm::Gpu
