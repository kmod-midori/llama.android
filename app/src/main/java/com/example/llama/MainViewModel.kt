package com.example.llama

import moe.reimu.llama.cpp.android.Message
import moe.reimu.llama.cpp.android.Sampler
import android.util.Log
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.launch
import moe.reimu.llama.cpp.android.Context
import moe.reimu.llama.cpp.android.Model

class MainViewModel : ViewModel() {
    private val tag: String? = this::class.simpleName

    private var model: Model? = null

    var messages by mutableStateOf(listOf("Initializing..."))
        private set

    var message by mutableStateOf("")
        private set

    override fun onCleared() {
        super.onCleared()

        try {
            model?.close()
        } catch (exc: Exception) {
            messages += exc.message!!
        }
    }

    fun send() {
        val text = message
        message = ""

        // Add to messages console.
        messages += text
        messages += ""

        viewModelScope.launch {
            val model = model
            if (model == null) {
                return@launch
            }

            try {
                val context = Context.create(model, 2048, 2048)
                val sampler = Sampler.create().apply {
                    addMinP(0.05f, 1)
                    addTemp(0.1f)
                    addDist(42)
                }
                val response = context.generate(
                    sampler, listOf(
                        Message("system", "/no_think\nYou are a helpful assistant."),
                        Message("user", text)
                    ),
                    256
                )
                messages += response
            } catch (exc: Exception) {
                Log.e(tag, "send() failed", exc)
                messages += exc.message!!
            }
        }
    }

    fun bench(pp: Int, tg: Int, pl: Int, nr: Int = 1) {
    }

    fun load(pathToModel: String) {
        viewModelScope.launch {
            try {
                model?.close()
                model = Model.create(pathToModel)

                messages += "Loaded $pathToModel"
            } catch (exc: IllegalStateException) {
                Log.e(tag, "load() failed", exc)
                messages += exc.message!!
            }
        }
    }

    fun updateMessage(newMessage: String) {
        message = newMessage
    }

    fun clear() {
        messages = listOf()
    }

    fun log(message: String) {
        messages += message
    }
}
