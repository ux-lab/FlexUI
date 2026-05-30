/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Android JNI stub. Real implementation lands in Phase 1+. Every entry point
 * logs NOT_IMPLEMENTED to verify the binary loads.
 */
#include <jni.h>

#include "flexui/common/log_tag.h"

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_flexui_FlexUIEngineNative_init(JNIEnv*, jclass, jint, jboolean) {
  FLEXUI_TLOG(Engine, JniInit, WARNING) << "NOT_IMPLEMENTED";
  return JNI_TRUE;  // stub success so caller can proceed in tests
}

JNIEXPORT void JNICALL
Java_com_flexui_FlexUIEngineNative_shutdown(JNIEnv*, jclass) {
  FLEXUI_TLOG(Engine, JniShutdown, WARNING) << "NOT_IMPLEMENTED";
}

JNIEXPORT jint JNICALL
Java_com_flexui_FlexCardControllerNative_create(JNIEnv*, jclass, jstring) {
  FLEXUI_TLOG(Card, JniCreate, WARNING) << "NOT_IMPLEMENTED";
  return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_flexui_FlexCardControllerNative_load(JNIEnv*, jclass, jint) {
  FLEXUI_TLOG(Card, JniLoad, WARNING) << "NOT_IMPLEMENTED";
  return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_flexui_FlexCardControllerNative_destroy(JNIEnv*, jclass, jint) {
  FLEXUI_TLOG(Card, JniDestroy, WARNING) << "NOT_IMPLEMENTED";
}

}  // extern "C"
