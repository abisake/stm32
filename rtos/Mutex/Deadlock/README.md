Demostrating Deadlock using 2 mutexes and 2 tasks

There is 4 conditions for deadlock to occur-> called as Coffman conditions
1. MUTUAL EXCLUSION
   At least one resource must be non-shareable.
   Only one task can hold the mutex at a time.
   In FreeRTOS: always true for mutexes by design.

2. HOLD AND WAIT
   A task holds one resource while waiting for another.
   Task A holds M1, waits for M2.
   The dangerous pattern: take M1, then try to take M2.

3. NO PREEMPTION
   A held resource cannot be forcibly taken away.
   FreeRTOS cannot grab M1 back from Task A.
   True for mutexes — only the holder can Give.

4. CIRCULAR WAIT
   Task A waits for Task B's resource.
   Task B waits for Task A's resource.
   The killing condition: A→B→A circular dependency.


If we can break any of the above conditions, deadlock wont happen.

We can fix the deadlock in two ways.
Fix1:
    Using Consistent ordering, breaks the circular wait condition

Fix 2:
    Using timeout -> breaks Hold & Wait condition.


Simulate Deadlock:
    Create 2 mutexex and 2 task
    Call task 1 to accquire M1 and wait 100ms
    Call task 2 to accquire M2.
    Now, in task 1 accquire M2 and in task 2 accquire M1, creates circular wait
    In terminal there wont be any logs, system hangs

Implemented Fix1:
    accquire both mutexes in the same order
    task 1 accquire M1 and M2
    task 2 accqyure M1 and M2
    when task 2 accquire M1, but it is blovcked by task 1, so it waits for task 1 to complete.

Code snippet of Fix2:
    SOmetimes we cannot follow the same order everywhere, so we can use timeout
    task_1()
    {
        while(1)
        {
            // take M1 we can use pdMax

            // take M2 with short timeout
            if(xSemaphoreTake(xM2, pdMs_TO_Ticks(50)) == pdTrue)
            {
                // do the task
                ....

                // realese the mutex
                xSemaphoreGive(xM2);
                xSemaphoreGive(xM1);  /* give in reverse order */
            }
            else
            {
                /* so unable to get M2, instead of holding M1,
                 realese the M1 as well, because we are simple 
                 waiting for M2 relase */
                xSemaphoreGive(M1);

                /*Wait for some time, maybe some random timeout or some benchmarked time*/
                vtaskDelay(pdMs_TO_Ticks(10 + (xTaskGetTickCount() & 0xF)));
            }
        }
    }