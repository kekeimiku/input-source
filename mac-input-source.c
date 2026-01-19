#include <Carbon/Carbon.h>
#include <emacs-module.h>

__attribute__((used, visibility("default"))) int plugin_is_GPL_compatible;

static emacs_value emacs_input_source_current(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  emacs_value result = env->intern(env, "nil");

  TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
  if (!source) return result;

  CFStringRef sourceID = (CFStringRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
  CFRelease(source);

  if (!sourceID) return result;

  char buffer[128];
  if (!CFStringGetCString(sourceID, buffer, sizeof(buffer), kCFStringEncodingUTF8)) return result;

  return env->make_string(env, buffer, strlen(buffer));
}

static emacs_value emacs_input_source_select(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  emacs_value result = env->intern(env, "nil");
  if (nargs < 1) return result;

  ptrdiff_t size = 0;
  env->copy_string_contents(env, args[0], NULL, &size);

  char *buffer = malloc(size);
  env->copy_string_contents(env, args[0], buffer, &size);

  if (buffer) {
    CFStringRef sourceIDStr = CFStringCreateWithCString(kCFAllocatorDefault, buffer, kCFStringEncodingUTF8);
    if (sourceIDStr) {
      const void *keys[] = { kTISPropertyInputSourceID };
      const void *values[] = { sourceIDStr };
      CFDictionaryRef filter = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

      CFArrayRef sources = TISCreateInputSourceList(filter, false);
      CFRelease(filter);
      CFRelease(sourceIDStr);

      if (sources && CFArrayGetCount(sources) > 0) {
        TISInputSourceRef source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, 0);
        OSStatus status = TISSelectInputSource(source);
        if (status == noErr) result = env->intern(env, "t");
      }

      if (sources) CFRelease(sources);
    }
  }

  free(buffer);

  return result;
}

static emacs_value emacs_input_source_list(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  const void *keys[] = { kTISPropertyInputSourceCategory };
  const void *values[] = { kTISCategoryKeyboardInputSource };
  CFDictionaryRef filter = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  CFArrayRef sources = TISCreateInputSourceList(filter, false);
  CFRelease(filter);

  emacs_value result = env->intern(env, "nil");
  if (!sources) return result;

  emacs_value cons = env->intern(env, "cons");
  CFIndex count = CFArrayGetCount(sources);

  for (CFIndex i = 0; i < count; i++) {
    TISInputSourceRef source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, i);
    CFStringRef sourceID = (CFStringRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceID);

    if (sourceID) {
      CFBooleanRef selectable = (CFBooleanRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsSelectCapable);
      if (selectable && CFBooleanGetValue(selectable)) {
        char buffer[128];
        if (CFStringGetCString(sourceID, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
          emacs_value str = env->make_string(env, buffer, strlen(buffer));
          emacs_value cons_args[] = { str, result };
          result = env->funcall(env, cons, 2, cons_args);
        }
      }
    }
  }

  CFRelease(sources);
  return result;
}

__attribute__((used, visibility("default"))) int emacs_module_init(struct emacs_runtime *runtime) {
  emacs_env *env = runtime->get_environment(runtime);
  emacs_value defalias = env->intern(env, "defalias");

  {
    emacs_value func = env->make_function(env, 0, 0, emacs_input_source_current, "Get current input source ID.", NULL);
    emacs_value symbol = env->intern(env, "mac-input-source-current");
    emacs_value args[] = { symbol, func };
    env->funcall(env, defalias, 2, args);
  }

  {
    emacs_value func = env->make_function(env, 1, 1, emacs_input_source_select, "Select input source by ID.", NULL);
    emacs_value symbol = env->intern(env, "mac-input-source-select");
    emacs_value args[] = { symbol, func };
    env->funcall(env, defalias, 2, args);
  }

  {
    emacs_value func = env->make_function(env, 0, 0, emacs_input_source_list, "List all available input source IDs.", NULL);
    emacs_value symbol = env->intern(env, "mac-input-source-list");
    emacs_value args[] = { symbol, func };
    env->funcall(env, defalias, 2, args);
  }

  return 0;
}
