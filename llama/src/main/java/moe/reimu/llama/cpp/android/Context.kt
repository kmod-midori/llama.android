package moe.reimu.llama.cpp.android

import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.Closeable

class Context private constructor(model: Model, ctxSize: Int, batchSize: Int) : Closeable {
    private var pointer: Long = 0

    private external fun initFromModel(modelPointer: Long, ctxSize: Int, batchSize: Int)
    private external fun free()
    private external fun generateNative(
        samplerPointer: Long,
        messages: Array<Message>,
        maxTokens: Int,
    ): String

    override fun close() {
        runBlocking(Backend.dispatcher) {
            free()
        }
    }

    init {
        if (ctxSize <= 0) {
            throw IllegalArgumentException("ctxSize must be positive")
        }
        if (batchSize <= 0) {
            throw IllegalArgumentException("batchSize must be positive")
        }

        initFromModel(model.getPointer(), ctxSize, batchSize)
    }

    fun getPointer(): Long {
        return pointer
    }

    suspend fun generate(sampler: Sampler, messages: Iterable<Message>, maxTokens: Int): String =
        withContext(Backend.dispatcher) {
            generateNative(
                sampler.getPointer(),
                messages.toList().toTypedArray(),
                maxTokens,
            )
        }

    companion object {
        suspend fun create(model: Model, ctxSize: Int, batchSize: Int) =
            withContext(Backend.dispatcher) {
                Context(model, ctxSize, batchSize)
            }
    }
}
