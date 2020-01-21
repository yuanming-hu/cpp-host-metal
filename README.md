# C++ Hosted Metal

## Progress

* `newBufferWithBytesNoCopy:length:options:deallocator:` working!
  * Pass the memory allocated by `mmap()`. 
  * **Both `ptr` and `length`** must be aligned to the page size. `ptr`'s alignment is guaranteed by `mmap()`, but not `length`. So we have to manually round up `length` to page size.
* The Metal compute pipeline works in C++ via objc-runtime.
  * Each line in the Metal compute kernel must be separated by `\n`.