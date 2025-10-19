#include <utility>
#include <vector>
#include <jni.h>
#include "llama.h"
#include "common.h"
#include <android/log.h>

class LlamaContextWrapper {
public:
    std::string cached_prompt;
    llama_context *context;

    explicit LlamaContextWrapper(llama_context *ctx) : context(ctx) {}

    ~LlamaContextWrapper() {
        if (context != nullptr) {
            llama_free(context);
        }
        context = nullptr;
    }
};

class LlamaMessage {
public:
    std::string role;
    std::string content;

    LlamaMessage(std::string r, std::string c) : role(std::move(r)), content(std::move(c)) {}
};

LlamaContextWrapper *get_context_wrapper(JNIEnv *env, jobject thiz) {
    static jfieldID pointer_field = nullptr;
    if (pointer_field == nullptr) {
        jclass cls = env->GetObjectClass(thiz);
        pointer_field = env->GetFieldID(cls, "pointer", "J");
    }
    jlong pointer = env->GetLongField(thiz, pointer_field);
    if (pointer == 0) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalStateException"),
                "Context is not initialized"
        );
        return nullptr;
    }
    return reinterpret_cast<LlamaContextWrapper *>(pointer);
}

void set_context_wrapper(JNIEnv *env, jobject thiz, LlamaContextWrapper *wrapper) {
    static jfieldID pointer_field = nullptr;
    if (pointer_field == nullptr) {
        jclass cls = env->GetObjectClass(thiz);
        pointer_field = env->GetFieldID(cls, "pointer", "J");
    }
    env->SetLongField(thiz, pointer_field, reinterpret_cast<jlong>(wrapper));
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Context_initFromModel(
        JNIEnv *env, jobject thiz,
        jlong model_pointer,
        jint ctx_size, jint batch_size
) {
    auto *model = reinterpret_cast<llama_model *>(model_pointer);
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = ctx_size;
    ctx_params.n_batch = batch_size;

    llama_context *ctx = llama_init_from_model(model, ctx_params);
    if (ctx == nullptr) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalStateException"),
                "Failed to initialize llama_context from model"
        );
        return;
    }
    auto wrapper = new LlamaContextWrapper(ctx);
    set_context_wrapper(env, thiz, wrapper);
}

extern "C"
JNIEXPORT void JNICALL
Java_moe_reimu_llama_cpp_android_Context_free(JNIEnv *env, jobject thiz) {
    auto *ctx = get_context_wrapper(env, thiz);
    __android_log_print(
            ANDROID_LOG_INFO,
            "Llama-Backend-Cpp",
            "Freeing context at %p",
            ctx->context
    );
    delete ctx;
    set_context_wrapper(env, thiz, nullptr);
}

std::optional<std::string>
apply_prompt_template(const char *tmpl, std::vector<LlamaMessage> &messages) {
    std::vector<char> prompt_raw(2048);

    std::vector<llama_chat_message> llama_messages;
    for (auto &msg: messages) {
        llama_messages.push_back({msg.role.c_str(), msg.content.c_str()});
    }

    int32_t prompt_len = llama_chat_apply_template(
            tmpl,
            llama_messages.data(), llama_messages.size(),
            true,
            prompt_raw.data(), (int32_t) prompt_raw.size()
    );
    if (prompt_len > prompt_raw.size()) {
        prompt_raw.resize(prompt_len);
        prompt_len = llama_chat_apply_template(
                tmpl,
                llama_messages.data(), llama_messages.size(),
                true,
                prompt_raw.data(), (int32_t) prompt_raw.size()
        );
    }
    if (prompt_len < 0) {
        return std::nullopt;
    }
    return std::string(prompt_raw.data(), prompt_len);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_moe_reimu_llama_cpp_android_Context_generateNative(
        JNIEnv *env, jobject thiz,
        jlong sampler_pointer,
        jobjectArray messages,
        jint max_tokens
) {
    auto *ctx_wrapper = get_context_wrapper(env, thiz);
    auto *ctx = ctx_wrapper->context;

    auto *sampler = reinterpret_cast<llama_sampler *>(sampler_pointer);
    if (sampler == nullptr) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalStateException"),
                "llama_sampler is null"
        );
        return nullptr;
    }
    const auto *model = llama_get_model(ctx);
    const auto *vocab = llama_model_get_vocab(model);

    std::vector<LlamaMessage> chat;
    auto count = env->GetArrayLength(messages);

    jclass message_class = env->FindClass("moe/reimu/llama/cpp/android/Message");
    jfieldID role_field = env->GetFieldID(message_class, "role", "Ljava/lang/String;");
    jfieldID content_field = env->GetFieldID(message_class, "content", "Ljava/lang/String;");

    for (jsize i = 0; i < count; ++i) {
        jobject msg_obj = env->GetObjectArrayElement(messages, i);
        auto *role_jstr = (jstring) env->GetObjectField(msg_obj, role_field);
        auto *content_jstr = (jstring) env->GetObjectField(msg_obj, content_field);

        const char *role_cstr = env->GetStringUTFChars(role_jstr, nullptr);
        jsize role_len = env->GetStringUTFLength(role_jstr);
        auto role = std::string(role_cstr, role_len);
        env->ReleaseStringUTFChars(role_jstr, role_cstr);

        const char *content_cstr = env->GetStringUTFChars(content_jstr, nullptr);
        jsize content_len = env->GetStringUTFLength(content_jstr);
        auto content = std::string(content_cstr, content_len);
        env->ReleaseStringUTFChars(content_jstr, content_cstr);

        chat.emplace_back(std::move(role), std::move(content));
    }

    const char *tmpl = llama_model_chat_template(model, nullptr);
    auto prompt_opt = apply_prompt_template(tmpl, chat);
    if (!prompt_opt.has_value()) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalArgumentException"),
                "Failed to apply chat template"
        );
        return nullptr;
    }
    auto prompt = prompt_opt.value();

    __android_log_print(
            ANDROID_LOG_INFO,
            "Llama-Backend-Cpp",
            "Generated prompt: %.*s",
            (int) prompt.size(),
            prompt.data()
    );

    if (!string_starts_with(prompt, ctx_wrapper->cached_prompt) &&
        !ctx_wrapper->cached_prompt.empty()) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalArgumentException"),
                "Prompt does not start with cached prompt, cannot reuse context"
        );
        return nullptr;
    }
    // Strip the cached prompt from the prompt to be processed
    prompt = prompt.substr(ctx_wrapper->cached_prompt.size());

    auto tokens = common_tokenize(
            ctx, prompt,
            ctx_wrapper->cached_prompt.empty(), true
    );

    llama_batch batch = llama_batch_get_one(tokens.data(), (int32_t) tokens.size());

    if (llama_model_has_encoder(model)) {
        int32_t encode_result = llama_encode(ctx, batch);
        if (encode_result != 0) {
            env->ThrowNew(
                    env->FindClass("java/lang/IllegalStateException"),
                    "Failed to encode batch"
            );
            return nullptr;
        }

        llama_token decoder_start_token_id = llama_model_decoder_start_token(model);
        if (decoder_start_token_id == LLAMA_TOKEN_NULL) {
            decoder_start_token_id = llama_vocab_bos(vocab);
        }

        batch = llama_batch_get_one(&decoder_start_token_id, 1);
    }

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

    chat.emplace_back("assistant", response);
    auto new_prompt_opt = apply_prompt_template(tmpl, chat);
    if (!new_prompt_opt.has_value()) {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalArgumentException"),
                "Failed to apply chat template for caching"
        );
        return nullptr;
    }
    ctx_wrapper->cached_prompt = new_prompt_opt.value();

    return env->NewStringUTF(response.c_str());
}
