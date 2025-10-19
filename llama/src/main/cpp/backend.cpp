#include <jni.h>
#include <android/log.h>
#include "llama.h"

static const char *TAG = "Llama-Backend-Cpp";

static void log_callback(ggml_log_level level, const char *fmt, void *data) {
    if (level == GGML_LOG_LEVEL_ERROR) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, data);
    } else if (level == GGML_LOG_LEVEL_INFO) {
        __android_log_print(ANDROID_LOG_INFO, TAG, fmt, data);
    } else if (level == GGML_LOG_LEVEL_WARN) {
        __android_log_print(ANDROID_LOG_WARN, TAG, fmt, data);
    } else {
        __android_log_print(ANDROID_LOG_DEFAULT, TAG, fmt, data);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Backend_init(JNIEnv *env, jobject thiz) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    llama_backend_init();
    llama_log_set(log_callback, nullptr);
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Backend_free(JNIEnv *env, jobject thiz) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    llama_backend_free();
}
