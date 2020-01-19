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

struct MTLDevice;
struct MTLLibrary;
struct MTLComputePipelineState;
struct MTLCommandQueue;
struct MTLCommandBuffer;
struct MTLComputeCommandEncoder;

MTLDevice *mtl_create_system_default_device() {
  id dev = MTLCreateSystemDefaultDevice();
  return reinterpret_cast<MTLDevice *>(dev);
}

MTLLibrary* new_default_library(MTLDevice *dev) {
  id lib = call(dev, "newDefaultLibrary");
  return reinterpret_cast<MTLLibrary*>(lib);
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