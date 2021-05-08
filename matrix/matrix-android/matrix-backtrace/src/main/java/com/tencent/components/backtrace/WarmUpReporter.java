package com.tencent.components.backtrace;

public interface WarmUpReporter {

    enum ReportEvent {
        WarmedUp,
        CleanedUp,
        WarmUpThreadBlocked,
        WarmUpFailed,
        WarmUpDuration,
        ConsumeRequestDuration,
        DiskUsage,
    }

    void onReport(ReportEvent type, Object ... args);

}