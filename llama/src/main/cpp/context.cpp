#include <vector>
#include <jni.h>
#include "llama.h"
#include "common.h"
#include <android/log.h>

extern "C"
JNIEXPORT jlong JNICALL
Java_moe_reimu_llama_cpp_android_Context_initFromModel(JNIEnv *env, jobject thiz, jlong model_pointer,
                                             jint ctx_size, jint batch_size) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *model = reinterpret_cast<llama_model *>(model_pointer);
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = ctx_size;
    ctx_params.n_batch = batch_size;

    llama_context *context = llama_init_from_model(model, ctx_params);
    return reinterpret_cast<jlong>(context);
}
extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Context_free(JNIEnv *env, jobject thiz, jlong pointer) {
    GGML_UNUSED(env);
    GGML_UNUSED(thiz);

    auto *context = reinterpret_cast<llama_context *>(pointer);
    llama_free(context);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_moe_reimu_llama_cpp_android_Context_generate(
        JNIEnv *env, jobject thiz, jlong ctx_pointer,
        jlong sampler_pointer,
        jobjectArray messages,
        jint max_tokens
) {
    GGML_UNUSED(thiz);

    auto *ctx = reinterpret_cast<llama_context *>(ctx_pointer);
    auto *sampler = reinterpret_cast<llama_sampler *>(sampler_pointer);
    const auto *model = llama_get_model(ctx);
    const auto *vocab = llama_model_get_vocab(model);

    std::vector<llama_chat_message> chat;
    auto count = env->GetArrayLength(messages);

    jclass message_class = env->FindClass("moe/reimu/llama/cpp/android/Message");
    jfieldID role_field = env->GetFieldID(message_class, "role", "Ljava/lang/String;");
    jfieldID content_field = env->GetFieldID(message_class, "content", "Ljava/lang/String;");

    for (jsize i = 0; i < count; ++i) {
        jobject msg_obj = env->GetObjectArrayElement(messages, i);
        auto *role_jstr = (jstring) env->GetObjectField(msg_obj, role_field);
        auto *content_jstr = (jstring) env->GetObjectField(msg_obj, content_field);
        const char *role_cstr = env->GetStringUTFChars(role_jstr, 0);
        const char *content_cstr = env->GetStringUTFChars(content_jstr, 0);
        chat.push_back({role_cstr, content_cstr});
    }

    std::vector<char> prompt_raw(llama_n_ctx(ctx));
    const char *tmpl = llama_model_chat_template(model, nullptr);
    int32_t prompt_len = llama_chat_apply_template(
            tmpl,
            chat.data(), chat.size(),
            true,
            prompt_raw.data(), (int32_t) prompt_raw.size()
    );
    if (prompt_len > prompt_raw.size()) {
        prompt_raw.resize(prompt_len);
        prompt_len = llama_chat_apply_template(
                tmpl,
                chat.data(), chat.size(),
                true,
                prompt_raw.data(), (int32_t) prompt_raw.size()
        );
    }

    __android_log_print(
            ANDROID_LOG_INFO,
            "Llama-Backend-Cpp",
            "Generated prompt: %.*s",
            prompt_len,
            prompt_raw.data()
    );

    for (jsize i = 0; i < count; ++i) {
        jobject msg_obj = env->GetObjectArrayElement(messages, i);
        auto *role_jstr = (jstring) env->GetObjectField(msg_obj, role_field);
        auto *content_jstr = (jstring) env->GetObjectField(msg_obj, content_field);
        auto message = chat[i];
        env->ReleaseStringUTFChars(role_jstr, message.role);
        env->ReleaseStringUTFChars(content_jstr, message.content);
    }
    if (prompt_len < 0) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalArgumentException"),
                "Failed to apply chat template"
        );
        return nullptr;
    }

    std::string prompt(prompt_raw.data(), prompt_len);
    auto tokens = common_tokenize(ctx, prompt, true, true);

    llama_batch batch = llama_batch_get_one(tokens.data(), (int32_t) tokens.size());
    std::string response;
    for (int i = 0; i < max_tokens; i++) {
        uint32_t n_ctx = llama_n_ctx(ctx);
        int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
        if (n_ctx_used + batch.n_tokens > n_ctx) {
            env->ThrowNew(
                    env->FindClass("java/lang/IllegalArgumentException"),
                    "Context size exceeded during generation"
            );
            return nullptr;
        }

        int decode_result = llama_decode(ctx, batch);
        if (decode_result != 0) {
            env->ThrowNew(
                    env->FindClass("java/lang/IllegalArgumentException"),
                    "Failed to decode batch"
            );
            return nullptr;
        }

        llama_token new_token_id = llama_sampler_sample(sampler, ctx, -1);
        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break;
        }

        auto new_token_chars = common_token_to_piece(ctx, new_token_id);
        response += new_token_chars;

        __android_log_print(
                ANDROID_LOG_INFO,
                "Llama-Backend-Cpp",
                "Sampled token: %s",
                new_token_chars.c_str()
        );

        // prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    return env->NewStringUTF(response.c_str());
}
