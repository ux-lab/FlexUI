package com.flexui

class FlexCardController(opts: Map<String, Any>) {
    private val id: Int

    private external fun createNative(optsJson: String): Int
    private external fun loadNative(id: Int): Boolean
    private external fun destroyNative(id: Int)

    init {
        id = createNative(opts.toString())
    }

    fun load(): Boolean = loadNative(id)
    fun destroy() = destroyNative(id)
}
