package com.tencent.matrix.hook.pthread;

import android.text.TextUtils;

import androidx.annotation.Nullable;

import com.tencent.matrix.hook.AbsHook;
import com.tencent.matrix.hook.HookManager;
import com.tencent.matrix.util.MatrixLog;

import java.util.HashSet;
import java.util.Set;

/**
 * Created by Yves on 2020-03-11
 */
public class PthreadHook extends AbsHook {
    private static final String TAG = "Matrix.Pthread";

    public static final PthreadHook INSTANCE = new PthreadHook();

    // private Set<String> mHookSoSet      = new HashSet<>();
    // private Set<String> mIgnoreSoSet    = new HashSet<>();
    private Set<String> mHookThreadName = new HashSet<>();
    private boolean mThreadTraceEnabled = false;
    private boolean mThreadStackShinkEnabled = false;

    private PthreadHook() {
    }

    // public PthreadHook addHookSo(String regex) {
    //     if (TextUtils.isEmpty(regex)) {
    //         MatrixLog.e(TAG, "so regex is empty");
    //     } else {
    //         mHookSoSet.add(regex);
    //     }
    //     return this;
    // }
    //
    // public PthreadHook addHookSo(String... regexArr) {
    //     for (String regex : regexArr) {
    //         addHookSo(regex);
    //     }
    //     return this;
    // }

    // public PthreadHook addIgnoreSo(String regex) {
    //     if (TextUtils.isEmpty(regex)) {
    //         return this;
    //     }
    //     mIgnoreSoSet.add(regex);
    //     return this;
    // }
    //
    // public PthreadHook addIgnoreSo(String... regexArr) {
    //     for (String regex : regexArr) {
    //         addIgnoreSo(regex);
    //     }
    //     return this;
    // }

    public PthreadHook addHookThread(String regex) {
        if (TextUtils.isEmpty(regex)) {
            MatrixLog.e(TAG, "thread regex is empty!!!");
        } else {
            mHookThreadName.add(regex);
        }
        return this;
    }

    public PthreadHook addHookThread(String... regexArr) {
        for (String regex : regexArr) {
            addHookThread(regex);
        }
        return this;
    }

    public PthreadHook setThreadTraceEnabled(boolean enabled) {
        mThreadTraceEnabled = enabled;
        return this;
    }

    public PthreadHook setThreadStackShinkEnabled(boolean enabled) {
        mThreadStackShinkEnabled = enabled;
        return this;
    }

    /**
     * notice: it is an exclusive interface
     */
    public void hook() throws HookManager.HookFailedException {
        HookManager.INSTANCE
                .clearHooks()
                .addHook(this)
                .commitHooks();
    }

    public void dump(String path) {
        if (HookManager.INSTANCE.hasHooked()) {
            dumpNative(path);
        }
    }

    @Nullable
    @Override
    protected String getNativeLibraryName() {
        return "matrix-pthreadhook";
    }

    @Override
    public void onConfigure() {
        addHookThreadNameNative(mHookThreadName.toArray(new String[0]));
        setThreadStackShinkEnabledNative(mThreadStackShinkEnabled);
        setThreadTraceEnabledNative(mThreadTraceEnabled);
    }

    @Override
    protected void onHook(boolean enableDebug) {
        // addHookSoNative(mHookSoSet.toArray(new String[0]));
        // addIgnoreSoNative(mIgnoreSoSet.toArray(new String[0]));
        if (mThreadTraceEnabled || mThreadStackShinkEnabled) {
            installHooksNative(enableDebug);
        }
    }

    // private native void addHookSoNative(String[] hookSoList);

    // private native void addIgnoreSoNative(String[] hookSoList);

    private native void setThreadTraceEnabledNative(boolean enabled);

    private native void setThreadStackShinkEnabledNative(boolean enabled);

    private native void addHookThreadNameNative(String[] threadNames);

    private native void dumpNative(String path);

    private native void installHooksNative(boolean enableDebug);
}
