#include <jni.h>
#include "llama.h"
#include <android/log.h>

extern "C"
JNIEXPORT jlong JNICALL
Java_moe_reimu_llama_cpp_android_Sampler_samplerChainInit(JNIEnv *env, jobject thiz) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);
    llama_sampler *smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    return reinterpret_cast<jlong>(smpl);
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Sampler_addMinP(
        JNIEnv *env, jobject thiz, jlong pointer, jfloat p, jlong min_keep
) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *smpl = reinterpret_cast<llama_sampler *>(pointer);
    llama_sampler_chain_add(smpl, llama_sampler_init_min_p(p, min_keep));
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Sampler_addTemp(JNIEnv *env, jobject thiz, jlong pointer, jfloat temp) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *smpl = reinterpret_cast<llama_sampler *>(pointer);
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(temp));
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Sampler_addDist(JNIEnv *env, jobject thiz, jlong pointer, jint seed) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *smpl = reinterpret_cast<llama_sampler *>(pointer);
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(seed));
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Sampler_samplerFree(JNIEnv *env, jobject thiz, jlong pointer) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *smpl = reinterpret_cast<llama_sampler *>(pointer);
    __android_log_print(
            ANDROID_LOG_INFO,
            "Llama-Backend-Cpp",
            "Freeing sampler at %p",
            smpl
    );
    llama_sampler_free(smpl);
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Sampler_addGrammar(JNIEnv *env, jobject thiz, jlong pointer,
                                          jlong model_pointer, jstring grammar,
                                          jstring grammar_root) {
    GGML_UNUSED(thiz);

    auto *smpl = reinterpret_cast<llama_sampler *>(pointer);
    auto *model = reinterpret_cast<llama_model *>(model_pointer);
    const llama_vocab *vocab = llama_model_get_vocab(model);
    const char *grammar_cstr = env->GetStringUTFChars(grammar, 0);
    const char *grammar_root_cstr = env->GetStringUTFChars(grammar_root, 0);

    llama_sampler *grammar_sampler = llama_sampler_init_grammar(
            vocab,
            grammar_cstr,
            grammar_root_cstr
    );
    if (grammar_sampler == nullptr) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalArgumentException"),
                "Failed to initialize grammar sampler"
        );
        return;
    }

    llama_sampler_chain_add(smpl, grammar_sampler);

    env->ReleaseStringUTFChars(grammar, grammar_cstr);
    env->ReleaseStringUTFChars(grammar_root, grammar_root_cstr);
}
