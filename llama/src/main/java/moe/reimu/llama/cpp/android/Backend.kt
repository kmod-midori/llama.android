package moe.reimu.llama.cpp.android

import android.util.Log
import kotlinx.coroutines.asCoroutineDispatcher
import java.util.concurrent.Executors
import kotlin.concurrent.thread

object Backend {
    private const val TAG = "LlamaBackend"

    private external fun init()
    private external fun free()

    val dispatcher = Executors.newSingleThreadExecutor {
        thread(start = false, name = "LlamaBackend") {
            System.loadLibrary("llama-android")

            init()

            it.run()
        }.apply {
            uncaughtExceptionHandler = Thread.UncaughtExceptionHandler { _, exception: Throwable ->
                Log.e(TAG, "Unhandled exception", exception)
            }
        }
    }.asCoroutineDispatcher()
}
