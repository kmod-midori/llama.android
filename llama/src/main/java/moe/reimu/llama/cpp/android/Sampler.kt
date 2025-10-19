package moe.reimu.llama.cpp.android

import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.Closeable

class Sampler private constructor() : Closeable {
    private var pointer: Long

    private external fun samplerChainInit(): Long

    private external fun samplerFree(pointer: Long)

    private external fun addMinP(pointer: Long, p: Float, minKeep: Long)

    private external fun addTemp(pointer: Long, temp: Float)

    private external fun addDist(pointer: Long, seed: Int)

    private external fun addGrammar(
        pointer: Long,
        modelPointer: Long,
        grammar: String,
        grammarRoot: String
    )

    suspend fun addMinP(p: Float, minKeep: Long) = withContext(Backend.dispatcher) {
        addMinP(pointer, p, minKeep)
    }

    suspend fun addTemp(temp: Float) = withContext(Backend.dispatcher) {
        addTemp(pointer, temp)
    }

    suspend fun addDist(seed: Int) = withContext(Backend.dispatcher) {
        addDist(pointer, seed)
    }

    suspend fun addGrammar(
        model: Model,
        grammar: String,
        grammarRoot: String
    ) = withContext(Backend.dispatcher) {
        addGrammar(pointer, model.getPointer(), grammar, grammarRoot)
    }

    override fun close() {
        runBlocking(Backend.dispatcher) {
            if (pointer != 0L) {
                samplerFree(pointer)
                pointer = 0L
            }
        }
    }

    fun getPointer(): Long {
        return pointer
    }

    init {
        pointer = samplerChainInit()
        if (pointer == 0L) {
            throw IllegalStateException("samplerChainInit() failed")
        }
    }

    companion object {
        suspend fun create(): Sampler = withContext(Backend.dispatcher) {
            Sampler()
        }
    }
}
