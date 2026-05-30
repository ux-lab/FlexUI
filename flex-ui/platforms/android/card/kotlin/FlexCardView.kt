package com.flexui

import android.content.Context
import android.util.AttributeSet
import android.view.ViewGroup

class FlexCardView(context: Context, attrs: AttributeSet? = null)
    : ViewGroup(context, attrs) {

    var controller: FlexCardController? = null

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        // PoC stub: no children. Real layout lands in Phase 1+ when JNI Attach
        // forwards the native view tree.
    }
}
