package com.n0n3m4.q3e.karin;

import android.os.Build;
import android.os.SystemClock;
import android.system.Os;
import android.system.OsConstants;

import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.util.HashMap;

public final class KCPUInfo
{
    // clock ticks per second of /proc/<pid>/stat's utime/stime, usually 100 on Android
    private static final long CLOCK_TICKS_PER_SECOND = GetClockTicksPerSecond();

    // number of configured CPU cores(including offline ones)
    private static final int NUM_CPUS = GetNumCPUs();

    private long lastProcessCpuTime = 0;
    private long lastWallTime = -1;

    // per-thread CPU times of last GetPerCPU() sampling, key is tid
    private HashMap<Integer, Long> lastThreadCpuTimes = null;
    private long lastPerCpuWallTime = -1;

    private static long GetClockTicksPerSecond()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            try {
                long v = Os.sysconf(OsConstants._SC_CLK_TCK);
                if (v > 0)
                    return v;
            } catch (Exception ignored) {
            }
        }
        return 100;
    }

    private static int GetNumCPUs()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            try {
                long v = Os.sysconf(OsConstants._SC_NPROCESSORS_CONF);
                if (v > 0)
                    return (int)v;
            } catch (Exception ignored) {
            }
        }
        return Math.max(Runtime.getRuntime().availableProcessors(), 1);
    }

    // returns current process CPU usage in percent(may exceed 100 on multi-core devices)
    public int Get() {
        return Math.round(Sample());
    }

    // returns current process CPU usage averaged over all CPU cores, in percent(0 - 100)
    public int GetAverage() {
        return Math.round(Sample() / (float)NUM_CPUS);
    }

    // sample process CPU usage since last call, in percent of a single core
    private float Sample() {
        try {
            long nowWall = SystemClock.elapsedRealtime(); // monotonic, unaffected by wall-clock changes
            long nowCpu = Q3EUtils.GetCPUTime(); // in clock ticks
            if (nowCpu < 0)
                return 0.0f;

            if (lastWallTime < 0) {
                lastProcessCpuTime = nowCpu;
                lastWallTime = nowWall;
                return 0.0f;
            }

            long cpuDiff = nowCpu - lastProcessCpuTime; // clock ticks
            long wallDiff = nowWall - lastWallTime; // milliseconds

            lastProcessCpuTime = nowCpu;
            lastWallTime = nowWall;

            if (wallDiff <= 0 || cpuDiff < 0)
                return 0.0f;

            // percent = (cpuDiff / CLK_TCK * 1000) / wallDiff * 100
            return (float)cpuDiff * 100000.0f / (float)(CLOCK_TICKS_PER_SECOND * wallDiff);
        } catch (Exception e) {
            e.printStackTrace();
            return 0.0f;
        }
    }

    // returns this process's CPU usage on every CPU core in percent, indexed by CPU id.
    // approximation: each thread's CPU time delta is attributed to the core it last ran on(/proc/self/task/<tid>/stat field 39),
    // so it is inaccurate when threads migrate between cores within the sampling interval
    public int[] GetPerCPU() {
        int[] result = new int[NUM_CPUS];
        try {
            long nowWall = SystemClock.elapsedRealtime();
            File[] tasks = new File("/proc/self/task").listFiles();
            if (null == tasks)
                return result;

            HashMap<Integer, Long> nowThreadCpuTimes = new HashMap<>();
            long[] coreTicks = new long[NUM_CPUS];
            for (File task : tasks) {
                try {
                    String content = Q3EUtils.file_get_contents(new File(task, "stat"));
                    if (KStr.IsEmpty(content))
                        continue;
                    int index = content.lastIndexOf(')');
                    if (index < 0 || index + 1 >= content.length())
                        continue;
                    String[] parts = content.substring(index + 1).trim().split("\\s+");
                    if (parts.length < 37) // need processor(field 39): index 36 after comm
                        continue;
                    int tid = Integer.parseInt(task.getName());
                    long cpuTicks = Long.parseLong(parts[11]) + Long.parseLong(parts[12]); // utime + stime
                    int cpu = Integer.parseInt(parts[36]); // core the thread last ran on

                    nowThreadCpuTimes.put(tid, cpuTicks);

                    if (null != lastThreadCpuTimes) {
                        Long last = lastThreadCpuTimes.get(tid);
                        // thread created after last sampling: all its CPU time is within this interval
                        long diff = null != last ? cpuTicks - last : cpuTicks;
                        if (diff > 0 && cpu >= 0 && cpu < NUM_CPUS)
                            coreTicks[cpu] += diff;
                    }
                } catch (Exception ignored) { // the thread may exit during sampling
                }
            }

            long wallDiff = lastPerCpuWallTime < 0 ? 0 : nowWall - lastPerCpuWallTime;
            lastThreadCpuTimes = nowThreadCpuTimes;
            lastPerCpuWallTime = nowWall;

            if (wallDiff <= 0)
                return result;

            for (int i = 0; i < NUM_CPUS; i++) {
                int percent = Math.round((float)coreTicks[i] * 100000.0f / (float)(CLOCK_TICKS_PER_SECOND * wallDiff));
                // migration attribution error may push a core over its physical limit
                result[i] = Math.min(percent, 100);
            }
            return result;
        } catch (Exception e) {
            e.printStackTrace();
            return result;
        }
    }
}
