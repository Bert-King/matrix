//
// Created by Yves on 2020-03-17.
//

#include <jni.h>
#include <xhook.h>
#include <sys/stat.h>
#include "HookCommon.h"
#include "Log.h"
#include "JNICommon.h"
#include "Backtrace.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "Matrix.HookCommon"

std::vector<dlopen_callback_t> m_dlopen_callbacks;
std::vector<hook_init_callback_t> m_init_callbacks;

static std::atomic_bool dlopen_pausing_mark(false);
static std::recursive_mutex dlopen_mutex;

DEFINE_HOOK_FUN(void *, __loader_android_dlopen_ext, const char *file_name,
                int                                             flag,
                const void                                      *extinfo,
                const void                                      *caller_addr) {
    if (dlopen_pausing_mark.load()) {
        std::lock_guard<std::recursive_mutex> dlopen_lock(dlopen_mutex);
    }

    void *ret = (*ORIGINAL_FUNC_NAME(__loader_android_dlopen_ext))(file_name, flag, extinfo,
                                                                   caller_addr);

    LOGD(TAG, "call into dlopen hook");

//    NanoSeconds_Start(TAG, begin);

    for (auto &callback : m_dlopen_callbacks) {
        callback(file_name);
    }

    // This line only refresh xhook in matrix-hookcommon library now.
    xhook_refresh(false);

//    NanoSeconds_End(TAG, begin, "refresh");

//    LOGD(TAG, "xhook_refresh cost : %lld", cost);

    return ret;
}

void add_dlopen_hook_callback(dlopen_callback_t callback) {
    m_dlopen_callbacks.push_back(callback);
}

void pause_dlopen() {
    LOGD(TAG, "pause_dlopen called.");
    dlopen_mutex.lock();
    dlopen_pausing_mark.store(true);
}

void resume_dlopen() {
    LOGD(TAG, "resume_dlopen called.");
    dlopen_pausing_mark.store(false);
    dlopen_mutex.unlock();
}

void add_hook_init_callback(hook_init_callback_t callback) {
    m_init_callbacks.push_back(callback);
}

void test_log_to_file(const char *ch) {
    const char *dir = "/sdcard/Android/data/com.tencent.mm/MicroMsg/Diagnostic";
    const char *path = "/sdcard/Android/data/com.tencent.mm/MicroMsg/Diagnostic/log";

    mkdir(dir, S_IRWXU|S_IRWXG|S_IROTH);

    FILE *log_file = fopen(path, "a+");

    if (!log_file) {
        return;
    }

    fprintf(log_file, "%p:%s\n", ch, ch);

    fflush(log_file);
    fclose(log_file);

}

bool get_java_stacktrace(char *stack_dst, size_t size) {
    JNIEnv *env = nullptr;

    if (!stack_dst) {
        return false;
    }

    if (m_java_vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK) {
        LOGD(TAG, "get_java_stacktrace call");

        jstring j_stacktrace = (jstring) env->CallStaticObjectMethod(m_class_HookManager, m_method_getStack);

        LOGD(TAG, "get_java_stacktrace called");
        const char *stacktrace = env->GetStringUTFChars(j_stacktrace, NULL);
        if (stacktrace) {
            const size_t stack_len = strlen(stacktrace);
            const size_t cpy_len = std::min(stack_len, size - 1);
            memcpy(stack_dst, stacktrace, cpy_len);
            stack_dst[cpy_len] = '\0';
        } else {
            strncpy(stack_dst, "\tget java stacktrace failed", size);
        }
        env->ReleaseStringUTFChars(j_stacktrace, stacktrace);
        env->DeleteLocalRef(j_stacktrace);
        return true;
    }

    strncpy(stack_dst, "\tnull", size);
    return false;
}

#ifdef __cplusplus
}

#undef TAG

#endif