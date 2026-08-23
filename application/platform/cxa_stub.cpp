// Android/Termux libc++abi compatibility stubs
//
// Termux Clang 21+ emits references to libc++abi symbols that don't exist
// in Termux's libc++_shared.so (as of 2026-06). These weak stubs allow
// linking to succeed. The affected code paths (std::make_exception_ptr,
// std::promise::~promise) will silently no-op instead of throwing — acceptable
// for this project since these paths are only hit on fatal errors in
// libdatachannel where the process would abort anyway.
//
// When Termux ships a libc++_shared.so that exports these symbols, the weak
// stubs will be overridden automatically.
//
// To check: nm -D /data/data/com.termux/files/usr/lib/libc++_shared.so | grep cxa_init_primary
//   — if it shows "T __cxa_init_primary_exception", this file can be removed.

#if defined(__ANDROID__)

extern "C" {

__attribute__((weak))
void __cxa_init_primary_exception(void*, void*, void (*)(void*)) {
}

// std::exception_ptr::__from_native_exception_pointer(void*)
// Mangled name queried via: nm -u libdatachannel-static.a | grep from_native
__attribute__((weak))
void _ZNSt13exception_ptr31__from_native_exception_pointerEPv(void*) {
}

}

#endif
