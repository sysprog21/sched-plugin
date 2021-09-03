## sched-plugin

`sched-plugin` allows user processes being handed out an option to use the
existing Linux scheduler or to use Linux kernel module (LKM) based one.

## Features
- The user processes initially writes its process ID (PID) to the file `/proc/process_sched_add`
  which corresponds to the kernel module `proc_set`. This procedure completes
  the registration of a process to the LKM Scheduler.
- The LKM based scheduler is executed internally via the kernel module `proc_sched`.
  The module executes a work queue construct for every time quanta.
- The `proc_set` and `proc_sched` modules are coupled through the kernel module
  `process_queue`. The `process_queue` module handles the internal details of all
  the processes associated with the LKM scheduler. It stores the process info a
  simple linked list nodes.
- Various interfaces are defined within the `proc_queue` to perform add, remove,
  get\_first, print operations on the queue. The scheduler performs an add and
  remove based on the context switch operation being triggered for every time quantum.
- On every time quanta, the scheduler pushes the currently executing PID to the
  `proc_queue` via `add_to_process` interface. And change its execution from
  Running to wait via `task` based interfaces. Once the currently executing process
  is added successfully into the queue, the process in the front of the queue is
  selected. The selected process state is changed to running and also removed
  from the queue.

## License

`sched-plugin` is released under the GNU GPL. Use of this source code is governed by
a copyleft-style license that can be found in the LICENSE file.
