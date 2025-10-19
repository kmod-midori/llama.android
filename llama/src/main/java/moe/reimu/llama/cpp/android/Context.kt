package moe.reimu.llama.cpp.android

import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.Closeable

class Context private constructor(model: Model, ctxSize: Int, batchSize: Int) : Closeable {
    private var pointer: Long

    private external fun initFromModel(modelPointer: Long, ctxSize: Int, batchSize: Int): Long
    private external fun free(pointer: Long)
    private external fun generate(
        ctxPointer: Long,
        samplerPointer: Long,
        messages: Array<Message>,
        maxTokens: Int,
    ): String

    override fun close() {
        runBlocking(Backend.dispatcher) {
            if (pointer == 0L) {
                return@runBlocking
            }

            free(pointer)
            pointer = 0L
        }
    }

    init {
        if (ctxSize <= 0) {
            throw IllegalArgumentException("ctxSize must be positive")
        }
        if (batchSize <= 0) {
            throw IllegalArgumentException("batchSize must be positive")
        }

        pointer = initFromModel(model.getPointer(), ctxSize, batchSize)
        if (pointer == 0L) {
            throw IllegalStateException("initFromModel() failed")
        }
    }

    fun getPointer(): Long {
        return pointer
    }

    suspend fun generate(sampler: Sampler, messages: Iterable<Message>, maxTokens: Int): String =
        withContext(Backend.dispatcher) {
            generate(
                pointer,
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
