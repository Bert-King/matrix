//
// Created by YinSheng Tang on 2021/4/29.
//

#include <sys/types.h>
#include <Log.h>
#include <xhook.h>
#include <xhook_ext.h>
#include <common/HookCommon.h>
#include <common/Macros.h>
#include "PthreadHook.h"
#include "ThreadTrace.h"
#include "ThreadStackShink.h"

#define LOG_TAG "Matrix.PthreadHook"

static bool sThreadTraceEnabled = false;
static bool sThreadStackShinkEnabled = false;

DECLARE_HOOK_ORIG(int, pthread_create, pthread_t*, pthread_attr_t const*, pthread_hook::pthread_routine_t, void*);
DECLARE_HOOK_ORIG(int, pthread_setname_np, pthread_t, const char*);

DEFINE_HOOK_FUN(int, pthread_create,
        pthread_t* pthread, pthread_attr_t const* attr, pthread_hook::pthread_routine_t start_routine, void* args) {
    pthread_attr_t tmpAttr;
    if (LIKELY(attr == nullptr)) {
        int ret = pthread_attr_init(&tmpAttr);
        if (UNLIKELY(ret != 0)) {
            LOGE(LOG_TAG, "Fail to init new attr, ret: %d", ret);
        }
    } else {
        tmpAttr = *attr;
    }

    if (sThreadStackShinkEnabled) {
        thread_stack_shink::OnPThreadCreate(pthread, &tmpAttr, start_routine, args);
    }

    int ret = 0;
    if (sThreadTraceEnabled) {
        auto *routine_wrapper = thread_trace::wrap_pthread_routine(start_routine, args);
        CALL_ORIGIN_FUNC_RET(int, tmpRet, pthread_create, pthread, &tmpAttr, routine_wrapper->wrapped_func,
                             routine_wrapper);
        ret = tmpRet;
    } else {
        CALL_ORIGIN_FUNC_RET(int, tmpRet, pthread_create, pthread, &tmpAttr, start_routine, args);
        ret = tmpRet;
    }

    if (LIKELY(ret == 0) && sThreadTraceEnabled) {
        thread_trace::on_pthread_create(pthread, &tmpAttr, start_routine, args);
    }

    if (LIKELY(attr == nullptr)) {
        pthread_attr_destroy(&tmpAttr);
    }

    return ret;
}

DEFINE_HOOK_FUN(int, pthread_setname_np, pthread_t pthread, const char* name) {
    CALL_ORIGIN_FUNC_RET(int, ret, pthread_setname_np, pthread, name);
    if (LIKELY(ret == 0) && sThreadTraceEnabled) {
        thread_trace::on_pthread_setname_np(pthread, name);
    }
    return ret;
}

namespace pthread_hook {
    void SetThreadTraceEnabled(bool enabled) {
        LOGD(LOG_TAG, "[*] Calling SetThreadTraceEnabled, enabled: %d", enabled);
        sThreadTraceEnabled = enabled;
    }

    void SetThreadStackShinkEnabled(bool enabled) {
        LOGD(LOG_TAG, "[*] Calling SetThreadStackShinkEnabled, enabled: %d", enabled);
        sThreadStackShinkEnabled = enabled;
    }

    void InstallHooks(bool enable_debug) {
        LOGI(LOG_TAG, "[+] Calling InstallHooks, sThreadTraceEnabled: %d, sThreadStackShinkEnabled: %d",
                sThreadTraceEnabled, sThreadStackShinkEnabled);
        if (!sThreadTraceEnabled && !sThreadStackShinkEnabled) {
            LOGD(LOG_TAG, "[*] InstallHooks was ignored.");
            return;
        }

        FETCH_ORIGIN_FUNC(pthread_create)
        FETCH_ORIGIN_FUNC(pthread_setname_np)

        if (sThreadTraceEnabled) {
            thread_trace::thread_trace_init();
        }

        pause_dlopen();
        {
            int ret = xhook_export_symtable_hook("libc.so", "pthread_create",
                                                 (void *) HANDLER_FUNC_NAME(pthread_create), nullptr);
            LOGD(LOG_TAG, "export table hook sym: pthread_create, ret: %d", ret);
            ret = xhook_export_symtable_hook("libc.so", "pthread_setname_np",
                                             (void *) HANDLER_FUNC_NAME(pthread_setname_np), nullptr);
            LOGD(LOG_TAG, "export table hook sym: pthread_setname_np, ret: %d", ret);
            xhook_register(".*/.*\\.so$", "pthread_create",
                           (void *) HANDLER_FUNC_NAME(pthread_create), nullptr);
            xhook_register(".*/.*\\.so$", "pthread_setname_np",
                           (void *) HANDLER_FUNC_NAME(pthread_setname_np), nullptr);
        }

        xhook_enable_debug(enable_debug ? 1 : 0);
        xhook_enable_sigsegv_protection(0);
        xhook_refresh(0);

        resume_dlopen();
    }
}
