#include <jni.h>
#include "llama.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_moe_reimu_llama_cpp_android_Model_load(JNIEnv *env, jobject thiz, jstring filename) {
    GGML_UNUSED(thiz);

    llama_model_params model_params = llama_model_default_params();
    const auto* path_to_model = env->GetStringUTFChars(filename, 0);
    llama_model *model = llama_model_load_from_file(path_to_model, model_params);
    env->ReleaseStringUTFChars(filename, path_to_model);

    return reinterpret_cast<jlong>(model);
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Model_free(JNIEnv *env, jobject thiz, jlong pointer) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *model = reinterpret_cast<llama_model *>(pointer);
    llama_model_free(model);
}
