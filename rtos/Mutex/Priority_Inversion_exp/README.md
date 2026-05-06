Priority Inversion Experiment (Mutex + Priority Inheritance)

Purpose:
Purpose is to demostrate a classic reat-time system failure happened in MARS Pathfinder.
A LOW priority meteorological task held an information bus mutex.
A HIGH priority watchdog task needed the bus but was blocked.
A MEDIUM priority communications task preempted LOW and ran instead.
<b>Issue</b> The watchdog expired waiting for HIGH to complete → spacecraft reset.
<b>Rootcause</b> priority inheritance was disabled on the VxWorks mutex.
<b>Fix</b> one-line config change uploaded to spacecraft remotely.

In the code, it is simulated via delays. And created three tasks:
Task LOW  (priority 1): holds sensor mutex for 400ms (slow I2C simulation)
Task MED  (priority 2): does not use mutex, runs freely
Task HIGH (priority 3): urgently needs sensor mutex


Reproducing the experiment
1. Flash with xSemaphoreCreateMutex() → observe in TeraTerm
2. Replace with xSemaphoreCreateBinary(), initialise with Give → observe 
3. Switch back to mutex

Observation from O/P:
Without inheritance
t=100ms  LOW wakes → takes binary semaphore → starts 400ms work
t=150ms  MED wakes → preempts LOW (MED pri=2 > LOW pri=1) ← LOW frozen mid-work
t=200ms  HIGH wakes → tries Take → semaphore is 0 → HIGH BLOCKED
          MED is priority 2, LOW is priority 1
          Nobody boosts LOW
          MED keeps running in its while(1) loop
          LOW never gets CPU to finish and release the semaphore
          HIGH blocks forever waiting for a semaphore that LOW can never release
          because LOW never gets CPU because MED always preempts it


With inheritance
1. LOW wakes  → takes mutex → starts 400ms work
2. MED wakes  → starts running (LOW has mutex, MED doesn't need it)
3. HIGH wakes → tries to take mutex → BLOCKED
         FreeRTOS sees: HIGH(3) blocked on mutex held by LOW(1)
         FreeRTOS boosts LOW → priority 1→3 temporarily
4. MED is priority 2, LOW is now priority 3
         MED cannot preempt LOW anymore → MED goes to READY, waits
5. LOW finishes → releases mutex → priority restored to 1
6. HIGH gets mutex immediately (highest priority ready task)
7. HIGH finishes → releases mutex
8. MED finally runs ← MED completes AFTER HIGH