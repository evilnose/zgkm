# ZGKM Chess Engine

## Notes
* How do the main and worker threads interact? Probably, have the worker thread "push" updates..
onto the main thread whenever results become available (e.g. finished searching one depth in..
iterative deepening). For fixed-time searches, the main thread simply waits x amount of time..
and then picks the best thread result and return. For time management, probably check after some..
amount of time the values and quiescence from the threads, and if we deem it helpful we'll..
continue searching and check again after a while; otherwise we'll return.
* `cout` is *not* synchronized across threads. This is probably not a big deal, but if there is..
time in the future, I should write a synchronized one that is lock-based.