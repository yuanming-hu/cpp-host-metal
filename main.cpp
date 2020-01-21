// clang++ -std=c++11 main.cpp -framework Metal -framework CoreGraphics -framework Foundation

// Frameworks:
// * Metal for obvious reason
// * CoreGraphics:
// https://developer.apple.com/documentation/metal/1433401-mtlcreatesystemdefaultdevice?language=objc
// * Foundation: objc runtime, https://gist.github.com/TooTallNate/1073294/675f28984cd120cdec14b66b41d86cf01140b1e6
//
// References:
//
// * Halide
// * taichi
// * https://github.com/naleksiev/mtlpp
//
// All <Metal/*> headers are for Obj-C, we cannot import them.
#include <iostream>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <string>

extern "C" {
void NSLog(id /* NSString * */ format, ...);
id MTLCreateSystemDefaultDevice();
}

namespace {

//  Need to have \n at the end, otherwise the compiled library is garbage...
const char *kernel_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "kernel void add1(device int* data [[ buffer(0) ]],\n"
    "                 const uint tid [[thread_position_in_grid]]) {\n"
    "    data[tid] += 42;\n"
    "}\n";

template <typename O, typename C = id, typename... Args>
C call(O *i, const char *select, Args... args) {
  using func = C (*)(id, SEL, Args...);
  return ((func)(objc_msgSend))(reinterpret_cast<id>(i), sel_getUid(select),
                                args...);
}

template <typename C = id, typename... Args>
C clscall(const char *class_name, const char *select, Args... args) {
  using func = C (*)(id, SEL, Args...);
  return ((func)(objc_msgSend))((id)objc_getClass(class_name),
                                sel_getUid(select), args...);
}

template <typename O> void release_ns_object(O *obj) {
  call(reinterpret_cast<id>(obj), "release");
}

constexpr int kNSUTF8StringEncoding = 4;

using NSString = objc_object;
struct MTLDevice;
struct MTLLibrary;
struct MTLComputePipelineState;
struct MTLCommandQueue;
struct MTLCommandBuffer;
struct MTLComputeCommandEncoder;
struct MTLFunction;
struct MTLComputePipelineState;
struct MTLBuffer;

MTLDevice *mtl_create_system_default_device() {
  id dev = MTLCreateSystemDefaultDevice();
  return reinterpret_cast<MTLDevice *>(dev);
}

MTLCommandQueue *new_command_queue(MTLDevice *dev) {
  id queue = call(dev, "newCommandQueue");
  return reinterpret_cast<MTLCommandQueue *>(queue);
}

MTLCommandBuffer* new_command_buffer(MTLCommandQueue* queue) {
  id buffer = call(queue, "commandBuffer");
  return reinterpret_cast<MTLCommandBuffer*>(buffer);
}

MTLComputeCommandEncoder* new_compute_command_encoder(MTLCommandBuffer* buffer) {
  id encoder = call(buffer, "computeCommandEncoder");
  return reinterpret_cast<MTLComputeCommandEncoder*>(encoder);
}

NSString *wrap_string_as_ns_string(const char *str, size_t len) {
  id ns_string = clscall("NSString", "alloc");
  return reinterpret_cast<NSString *>(
      call(ns_string, "initWithBytesNoCopy:length:encoding:freeWhenDone:", str,
           len, kNSUTF8StringEncoding, false));
}

MTLLibrary *new_library_with_source(MTLDevice *device, const char *source,
                                    size_t source_len) {
  NSString *source_str = wrap_string_as_ns_string(source, source_len);

  id options = clscall("MTLCompileOptions", "alloc");
  options = call(options, "init");
  call(options, "setFastMathEnabled:", false);

  id lib = call(device, "newLibraryWithSource:options:error:", source_str,
                options, nullptr);

  release_ns_object(options);
  release_ns_object(source_str);

  return reinterpret_cast<MTLLibrary *>(lib);
}

MTLFunction *new_function_with_name(MTLLibrary *library, const char *name,
                                    size_t name_len) {
  NSString *name_str = wrap_string_as_ns_string(name, name_len);
  id func = call(library, "newFunctionWithName:", name_str);
  release_ns_object(name_str);
  return reinterpret_cast<MTLFunction *>(func);
}

MTLComputePipelineState *
new_compute_pipeline_state_with_function(MTLDevice *device,
                                         MTLFunction *function) {
  id pipeline_state = call(
      device, "newComputePipelineStateWithFunction:error:", function, nullptr);
  return reinterpret_cast<MTLComputePipelineState *>(pipeline_state);
}

void set_compute_pipeline_state(MTLComputeCommandEncoder *encoder,
                                MTLComputePipelineState *pipeline_state) {
  call(encoder, "setComputePipelineState:", pipeline_state);
}

void end_encoding(MTLComputeCommandEncoder *encoder) {
    call(encoder, "endEncoding");
}

MTLBuffer *new_mtl_buffer(MTLDevice *device, size_t length) {
  id buffer = call(device, "newBufferWithLength:options:", length, 0 /* MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeShared */);
  return reinterpret_cast<MTLBuffer *>(buffer);
}

void set_mtl_buffer(MTLComputeCommandEncoder *encoder, MTLBuffer *buffer,
                    size_t offset, size_t index) {
  call(encoder, "setBuffer:offset:atIndex:", buffer, offset, index);
}

void dispatch_threadgroups(MTLComputeCommandEncoder *encoder, int32_t blocks_x,
                           int32_t blocks_y, int32_t blocks_z,
                           int32_t threads_x, int32_t threads_y,
                           int32_t threads_z) {
  struct MTLSize {
    uint64_t width;
    uint64_t height;
    uint64_t depth;
  };

  MTLSize threadgroups_per_grid;
  threadgroups_per_grid.width = blocks_x;
  threadgroups_per_grid.height = blocks_y;
  threadgroups_per_grid.depth = blocks_z;

  MTLSize threads_per_threadgroup;
  threads_per_threadgroup.width = threads_x;
  threads_per_threadgroup.height = threads_y;
  threads_per_threadgroup.depth = threads_z;

  call(encoder,
       "dispatchThreadgroups:threadsPerThreadgroup:", threadgroups_per_grid,
       threads_per_threadgroup);
}

// 1D
void dispatch_threadgroups(MTLComputeCommandEncoder *encoder, int32_t blocks_x,
                           int32_t threads_x) {
  dispatch_threadgroups(encoder, blocks_x, 1, 1, threads_x, 1, 1);
}

void commit_command_buffer(MTLCommandBuffer *cmd_buffer) {
  call(cmd_buffer, "commit");
}

void wait_until_completed(MTLCommandBuffer *cmd_buffer) {
  call(cmd_buffer, "waitUntilCompleted");
}

void *mtl_buffer_contents(MTLBuffer *buffer) {
  return call(buffer, "contents");
}

} // namespace

int main() {
  MTLDevice *device = mtl_create_system_default_device();
  std::cout << "mtl_device=" << device << "\n";

  const std::string kernel_src_str(kernel_src);
  MTLLibrary *kernel_lib = new_library_with_source(
      device, kernel_src_str.data(), kernel_src_str.size());
  std::cout << "kernel_lib=" << kernel_lib << "\n";

  const std::string kernel_func_str("add1");
  MTLFunction *kernel_func = new_function_with_name(
      kernel_lib, kernel_func_str.data(), kernel_func_str.size());
  std::cout << "kernel_func=" << kernel_func << "\n";

  MTLComputePipelineState *pipeline_state =
      new_compute_pipeline_state_with_function(device, kernel_func);
  std::cout << "pipeline_state=" << pipeline_state << "\n";

  constexpr size_t kBufferLen = 128;
  constexpr size_t kBufferBytesLen = sizeof(int32_t) * kBufferLen;
  MTLBuffer *mtl_buffer = new_mtl_buffer(device, kBufferBytesLen);
  std::cout << "mtl_buffer=" << mtl_buffer << "\n";

  MTLCommandQueue *cmd_queue = new_command_queue(device);
  std::cout << "cmd_queue=" << cmd_queue << "\n";
  MTLCommandBuffer* cmd_buffer = new_command_buffer(cmd_queue);
  std::cout << "cmd_buffer=" << cmd_buffer << "\n";

  MTLComputeCommandEncoder* encoder = new_compute_command_encoder(cmd_buffer);
  std::cout << "compute encoder=" << encoder << "\n";

  set_compute_pipeline_state(encoder, pipeline_state);
  std::cout << "set_compute_pipeline_state done\n";

  set_mtl_buffer(encoder, mtl_buffer, /*offset=*/0, /*index=*/0);
  std::cout << "set_mtl_buffer done\n";

  dispatch_threadgroups(encoder, kBufferLen, 1);
  std::cout << "dispatch_threadgroups done\n";

  end_encoding(encoder);
  std::cout << "end_encoding done\n";

  commit_command_buffer(cmd_buffer);
  std::cout << "commit_command_buffer done\n";

  wait_until_completed(cmd_buffer);
  std::cout << "wait_until_completed done\n";

  int32_t *buffer_contents =
      reinterpret_cast<int32_t *>(mtl_buffer_contents(mtl_buffer));
  
  for (int i = 0; i < kBufferLen; ++i) {
    std::cout << "mtl_buffer[" << i << "]=" << buffer_contents[i] << "\n";
  }

  return 0;
}
