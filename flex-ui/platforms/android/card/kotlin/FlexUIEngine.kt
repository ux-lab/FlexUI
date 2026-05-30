package com.flexui

object FlexUIEngine {
    @JvmStatic external fun init(backend: Int, forceQuickJsForDebug: Boolean): Boolean
    @JvmStatic external fun shutdown()

    init {
        System.loadLibrary("flexui_jni")
    }

    fun install(plugin: Any): Boolean {
        // Plugin install lands in Phase 1+.
        return true
    }
}
