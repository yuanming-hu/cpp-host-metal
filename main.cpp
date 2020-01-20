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

extern "C" {
id MTLCreateSystemDefaultDevice();
}

namespace {

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

struct NSString;
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
           len, 4, false));
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

void set_buffer(MTLComputeCommandEncoder *encoder, MTLBuffer *buffer,
                size_t offset, size_t index) {
  call(encoder, "setBuffer:offset:atIndex:", buffer, offset, index);
}

void commit_command_buffer(MTLCommandBuffer *cmd_buffer) {
  call(cmd_buffer, "commit");
}

void wait_until_completed(MTLCommandBuffer *cmd_buffer) {
  call(cmd_buffer, "waitUntilCompleted");
}

} // namespace

int main() {
  MTLDevice* dev = mtl_create_system_default_device();
  std::cout << "mtl_device=" << dev << "\n";
  MTLCommandQueue* cmd_queue = new_command_queue(dev);
  std::cout << "cmd_queue=" << cmd_queue << "\n";
  MTLCommandBuffer* cmd_buffer = new_command_buffer(cmd_queue);
  std::cout << "cmd_buffer=" << cmd_buffer << "\n";

  MTLComputeCommandEncoder* encoder = new_compute_command_encoder(cmd_buffer);
  std::cout << "compute encoder=" << encoder << "\n";
  return 0;
}