//

// Disable LeakSanitizer when this file is linked in.
// This function overrides __lsan_is_turned_off from sanitizer/lsan_interface.h
extern "C" int __lsan_is_turned_off();
extern "C" int __lsan_is_turned_off() {
  return 1;
}
