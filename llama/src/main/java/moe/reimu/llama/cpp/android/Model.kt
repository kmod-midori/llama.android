package moe.reimu.llama.cpp.android

import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.Closeable

class Model private constructor(filename: String) : Closeable {
    private var pointer: Long

    private external fun load(filename: String): Long
    private external fun free(pointer: Long)

    override fun close() {
        runBlocking(Backend.dispatcher) {
            if (pointer == 0L) {
                return@runBlocking
            }

            free(pointer)
            pointer = 0L
        }
    }

    fun getPointer(): Long {
        return pointer
    }

    init {
        pointer = load(filename)
        if (pointer == 0L) {
            throw IllegalStateException("load() failed")
        }
    }

    companion object {
        suspend fun create(filename: String): Model = withContext(Backend.dispatcher) {
            Model(filename)
        }
    }
}
