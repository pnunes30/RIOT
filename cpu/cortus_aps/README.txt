__20180601_____________RTT + new timer periph__________________________________________________
USEMODULE += rtt

rtt is required by some L2 MAC layers.
needs a set_value feature => counter
needs an overflow cb => timer
needs an up counter => timer
needs prescaling => timer
...
again, we need a new periph: mix of counter and timer...

__20180601_____________newlib-nano  and   VFS   and atomic_int_________________________________
TBI:
USEMODULE += newlib_nano
alternative to newlib and miniclib (from gnu c++ standard lib) 
VFS depends on it, posix_sockets.c, atomic_int... etc...

dirty FIX:
cpu/aps/include/sys/types.h  + #include_next

NOTE: fixed again missing miniclib atoi + snprintf
FIXME: find the right arch .h which is always included... in i.e. sys/shell/shell_commands.c

=> examples/posix_sockets OK 

__20180601_____________xtimer_mutex_timeout____________________________________________________

* WARNING-TBI: UART_TX_ISR must be avoided for now due to priority inversion problem
               and xtimer_mutex implementation issues.  
               *** use UART TX POLL mode (blocks all threads).****

*_XTIMER_FIX_12 : POK TBI STILL ISSUES due to priority inversion problem ????
                  solves nested interrupt thread_yield_higher invocation in xtimer_mutex_timeout
                  
                  thread_yield_higher should NEVER be invoked explicitly in ISR, it is the 
                  duty of _exit_isr(). 
                  sched_switch(mt->thread->priority) is there for that!
                  
The problem can be reproduced with a short UART TX ISR timeout.
If the tx isr timeouts just at the end of a cpu_switch_context_exit,
just before a context_restore... 
i.e.: frame#6 in thread4 below + cf __aps_debug.uart_tx_timeout

(gdb) p __aps_debug 
$1 = {
  irq_31 = 0x6d48, 
  irq_cnt = 0x6194, 
  irq_tim_overflow = 0x0, 
  irq_tim_cmp = 0x1e54, 
  irq_uart1_rx = 0x15, 
  irq_uart1_tx = 0x432b, 
  irq_uart2_rx = 0x0, 
  irq_uart2_tx = 0x0, 
  irq_nested = 0x2, 
  ctx_save = 0x6d48, 
  ctx_restore = 0x6d49, 
  tim_last_cap_loop_cnt = 0x1, 
  tim_max_cap_loop_cnt = 0x1, 
  uart_max_loop_cnt = 0xffffffbb, 
  uart_tx_timeout_cnt = 0x5
}
(gdb) c
Continuing.
^C
Program received signal SIGINT, Interrupt.
0x800012aa in pm_set_lowest ()
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/periph/cortus_pm.c:31
31	}
(gdb) p *timer1
$2 = {
  period = 0xffffffff, 
  prescaler = 0x0, 
  tclk_sel = 0x2, 
  enable = 0x1, 
  status = 0x0, 
  mask = 0x0
}
(gdb) p *timer1_cmpa
$3 = {
  compare = 0x1936f87f, 
  control = 0x0, 
  set_reset = 0x0, 
  enable = 0x0, 
  status = 0x0, 
  mask = 0x0
}
(gdb) p *timer1_capa
$4 = {
  in_cap_sel = 0x4, 
  edge_sel = 0x0, 
  value = 0x19372358, 
  enable = 0x1, 
  status = 0x0, 
  mask = 0x0, 
  capture = 0x0
}
(gdb) c
Continuing.
^C
Program received signal SIGINT, Interrupt.
0x800012aa in pm_set_lowest ()
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/periph/cortus_pm.c:31
31	}
(gdb) p __in_isr 
$5 = 0x1
(gdb) btt
===Thread#1=== r1=[0x00001170] pc=[0x800012aa] psr=[0x20000] mrk=[0] <idle>
#0  0x800012aa in pm_set_lowest ()
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/periph/cortus_pm.c:31
#1  0x80001588 in idle_thread (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/kernel_init.c:68
#2  0x80001470 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#3  0x77777777 in ?? ()

---Thread#2--- r1=[0x0000170c] pc=[0x80001bf2] psr=[0x20004] mrk=[27952] <main>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x800014f4 in _mutex_lock (mutex=0x1730 <main_stack+1412>, blocking=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/mutex.c:62
#2  0x8000263e in mutex_lock (mutex=0x1730 <main_stack+1412>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/include/mutex.h:113
#3  _xtimer_periodic_wakeup (last_wakeup=0x1760 <main_stack+1460>, period=0x1e848)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/xtimer/xtimer.c:144
#4  0x80000554 in xtimer_periodic_wakeup (period=0x3d09, last_wakeup=0x1760 <main_stack+1460>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/include/xtimer/implementation.h:240
#5  main () at /home/jmhemstedt/proj/d7/sw/riot/tests/xtimer_drift/main.c:239

---Thread#3--- r1=[0x00000efc] pc=[0x80001bf2] psr=[0x20004] mrk=[27926] <slacker1>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x800018b8 in _msg_receive (m=0xf3c <slacker_stack1+940>, block=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:330
#2  0x80001a7e in msg_receive (m=0xf3c <slacker_stack1+940>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:286
#3  0x80000354 in slacker_thread (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/tests/xtimer_drift/main.c:88
#4  0x80001470 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#5  0x77777777 in ?? ()

---Thread#4--- r1=[0x00000a90] pc=[0x80001bf2] psr=[0x0] mrk=[27958] <slacker2>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x800024cc in _mutex_timeout (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/xtimer/xtimer.c:259
#2  0x800020e6 in _shoot (timer=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/xtimer/xtimer_core.c:191
#3  _timer_callback () at /home/jmhemstedt/proj/d7/sw/riot/sys/xtimer/xtimer_core.c:653
#4  _periph_timer_callback (arg=<optimized out>, chan=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/xtimer/xtimer_core.c:183
#5  0x80000bc2 in timer_irq_handler (channel=0x0, tim=0x0)
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/periph/cortus_timer.c:483
#6  interrupt19_handler ()
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/periph/cortus_timer.c:523
#7  0x80001bf0 in __context_restore ()
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:208
#8  cpu_switch_context_exit () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:158
#9  0x800014f4 in _mutex_lock (mutex=0xb00 <slacker_stack2+880>, blocking=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/mutex.c:62
#10 0x80002584 in mutex_lock (mutex=0xb00 <slacker_stack2+880>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/include/mutex.h:113
#11 _xtimer_tsleep (offset=0x640, long_offset=0x0)
---Type <return> to continue, or q <return> to quit---
    at /home/jmhemstedt/proj/d7/sw/riot/sys/xtimer/xtimer.c:69
#12 0x8000036c in _xtimer_tsleep32 (ticks=0x640)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/include/xtimer/implementation.h:196
#13 xtimer_usleep (microseconds=0xc8)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/include/xtimer/implementation.h:210
#14 slacker_thread (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/tests/xtimer_drift/main.c:92
#15 0x80001470 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#16 0x77777777 in ?? ()

---Thread#5--- r1=[0x000006f8] pc=[0x80001bf2] psr=[0x20004] mrk=[27976] <worker>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x800018b8 in _msg_receive (m=0x720 <worker_stack+1424>, block=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:330
#2  0x80001a7e in msg_receive (m=0x720 <worker_stack+1424>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:286
#3  0x800001b6 in worker_thread (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/tests/xtimer_drift/main.c:119
#4  0x80001470 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#5  0x77777777 in ?? ()

(gdb) ptt
===Thread#1=== r1=[0x00001170] prio=[15] st=[ 9] sz[ 256] use[  60] <idle>
---Thread#2--- r1=[0x000016c8] prio=[ 7] st=[ 2] sz[1536] use[ 228] <main>
---Thread#3--- r1=[0x00000eb8] prio=[ 6] st=[ 3] sz[1024] use[ 216] <slacker1>
---Thread#4--- r1=[0x00000a4c] prio=[ 6] st=[ 2] sz[1024] use[ 324] <slacker2>
---Thread#5--- r1=[0x000006b4] prio=[ 6] st=[ 3] sz[1536] use[ 220] <worker>
 


__20180601_____________examples/gnrc_networking+ETHOS__________________________________________

* NESTED INTERRUPTS: __in_isr ++/-- : solves ethos deadlock and ping6 timeout
* APS_UART_RX_ALWAYS_ON: must be on for a network uart itf (ethos or slip)
*_XTIMER_FIX_11 : solves wrong tick conversion and _assert_failure in evtimer
* _assert_failure: breakpoint in gdbinit before core_panic to avoid stack trashing.
           

see examples/gnrc_networking/ReadMe.md

The app will enable the ethos interface by default on riot with an ipv6 
autoconfigured (NDP/nib) link local address. It embbeds a udp cl/srv.
All threads will msg_wait in an event_loop, except the main thread
running the shell and blocking on getchar. RX and TX isr will trigger
most probably in the idle loop. They'll be handled by the ethos driver
wich will mux io between the stdio (shell) and the ipv6 thread through
msg and mutex manipulation.

tests:
@host# make clean all term
@host# (dist/tools/ethos/start_network.sh)
@host# ifconfig					-->$ipv6_host (tap0)
@riot# ifconfig					-->$ipv6_riot
@host# ping6 $ipv6_riot%tap0	-->0% packet loss
@riot# ping6 $ipv6_host			-->0% packet loss

@riot# udp server start 8808
@host# nc -6uv fe80::24d:a9ff:fe5f:17c5%tap0 8808
@host# 33333333					-->@riot# pktsnip 33 33 33 33.....

@host# nc -6ul 8809
@riot# udp send $ipv6_host 8809 foobar

@host#--- fe80::24d:a9ff:fe5f:17c5%tap0 ping statistics ---
81222 packets transmitted, 81220 received, 0% packet loss, time 1295426ms
rtt min/avg/max/mdev = 21.948/33.833/47.947/4.616 ms, pipe 4, ipg/ewma 15.949/23.866 ms
@riot#--- fe80::c4ef:11ff:fe17:83a2 ping statistics ---
1000 packets transmitted, 1000 received, 0% packet loss, time 1687.06561328 s
rtt min/avg/max = 16.987/28.711/61.314 ms
(gdb) p __aps_debug 
$1 = {
  irq_31 = 0x3a3abb, 
  irq_cnt = 0x1391c5f, 
  irq_tim_overflow = 0x0, 
  irq_tim_cmp = 0x11ee, 
  irq_uart1_rx = 0xa4298a, 
  irq_uart1_tx = 0x94e0e7, 
  irq_uart2_rx = 0x0, 
  irq_uart2_tx = 0x0, 
  irq_nested = 0x1, 
  ctx_save = 0x3a3abb, 
  ctx_restore = 0x3a3abc, 
  tim_last_cap_loop_cnt = 0x1, 
  tim_max_cap_loop_cnt = 0x1, 
  uart_max_loop_cnt = 0xffffffcd, 
  uart_tx_timeout_cnt = 0xdd7
}
(gdb) btt
===Thread#1=== r1=[0x000012b8] pc=[0x80001212] psr=[0x20000] mrk=[0] <idle>
#0  0x80001212 in pm_set_lowest ()
    at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/periph/cortus_pm.c:31
#1  0x8000161c in idle_thread (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/kernel_init.c:68
#2  0x80001464 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#3  0x77777777 in ?? ()

---Thread#2--- r1=[0x000017ec] pc=[0x80001f4e] psr=[0x20004] mrk=[3808084] <main>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x800014e8 in _mutex_lock (mutex=0x440 <uart_stdio_isrpipe>, blocking=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/mutex.c:62
#2  0x80009cc2 in mutex_lock (mutex=0x440 <uart_stdio_isrpipe>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/include/mutex.h:113
#3  isrpipe_read (isrpipe=0x440 <uart_stdio_isrpipe>, buffer=0x1814 <main_stack+1312> " \030", 
    count=0x1) at /home/jmhemstedt/proj/d7/sw/riot/sys/isrpipe/isrpipe.c:48
#4  0x8000c898 in uart_stdio_read (buffer=0x1814 <main_stack+1312> " \030", count=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/uart_stdio/uart_stdio.c:112
#5  0x80002034 in getchar () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/aps_stdio.c:101
#6  0x8000a0b0 in readline (size=0x8000e399, buf=0x1838 <main_stack+1348> "udp")
    at /home/jmhemstedt/proj/d7/sw/riot/sys/shell/shell.c:229
#7  shell_run (shell_commands=0x8000e3c8 <shell_commands>, 
    line_buf=0x1838 <main_stack+1348> "udp", len=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/shell/shell.c:288
Backtrace stopped: frame did not save the PC

---Thread#3--- r1=[0x00004004] pc=[0x80001f4e] psr=[0x20004] mrk=[3816119] <pktdump>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x80001ad0 in _msg_receive (m=0x4064 <_stack+1468>, block=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:330
#2  0x80001dce in msg_receive (m=0x4064 <_stack+1468>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:286
#3  0x800089fa in _eventloop (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/net/gnrc/pktdump/gnrc_pktdump.c:142
#4  0x80001464 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#5  0x77777777 in ?? ()

---Thread#4--- r1=[0x00001c80] pc=[0x80001f4e] psr=[0x20004] mrk=[3816123] <ipv6>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x80001ad0 in _msg_receive (m=0x1cf0 <_stack+960>, block=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:330
#2  0x80001dce in msg_receive (m=0x1cf0 <_stack+960>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:286
#3  0x8000340a in _event_loop (args=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/net/gnrc/network_layer/ipv6/gnrc_ipv6.c:267
#4  0x80001464 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#5  0x77777777 in ?? ()

---Thread#5--- r1=[0x000043ec] pc=[0x80001f4e] psr=[0x20004] mrk=[3814553] <udp>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x80001ad0 in _msg_receive (m=0x445c <_stack+948>, block=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:330
#2  0x80001dce in msg_receive (m=0x445c <_stack+948>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:286
#3  0x80009166 in _event_loop (arg=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/net/gnrc/transport_layer/udp/gnrc_udp.c:237
#4  0x80001464 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#5  0x77777777 in ?? ()

---Thread#6--- r1=[0x00001028] pc=[0x80001f4e] psr=[0x20004] mrk=[3814549] <ethos>
#0  thread_yield_higher () at /home/jmhemstedt/proj/d7/sw/riot/cpu/aps/thread_arch.c:169
#1  0x80001ad0 in _msg_receive (m=0x1080 <_netdev_eth_stack+940>, block=0x1)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:330
#2  0x80001dce in msg_receive (m=0x1080 <_netdev_eth_stack+940>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/msg.c:286
#3  0x80007242 in _gnrc_netif_thread (args=0x20e0 <_netifs>)
    at /home/jmhemstedt/proj/d7/sw/riot/sys/net/gnrc/netif/gnrc_netif.c:1242
#4  0x80001464 in sched_switch (other_prio=<optimized out>)
    at /home/jmhemstedt/proj/d7/sw/riot/core/sched.c:209
#5  0x77777777 in ?? ()


__20180529_____________examples/gnrc_networking+slip___________________________________________

## Connecting RIOT w SLIP and the Linux host

SLIP is only a framing protocol (it is ipv4 or ipv6 compatible).
BUT slattach on Linux (used to create a slip interface) is, by limitation, only ipv4 compatible.
We must use tunslip6 from Contiki if we use a SLIP interface on Riot.

=> In Riot Makefile, enable a slip interface on the last (NUMOF-1 by default) UART:

	>> add slipdev_params.h to your project (anywhere in the include path if not already in the arch)
	>> USEMODULE += slipdev in the project's Makefile

=> On Riot, configure manually a local address in the shell:
	
	>Note: setting GNRC_IPV6_STATIC_LLADDR in Makefile has no effect anymore.
	@riot# ifconfig <if=pid of slipdev thread> add fe80::cafe:cafe:cafe:1

=> On Linux Host, use tunslip6:

	@host# cd dist/tools/tunslip/ && make && sudo make install
	@host# (sudo) tunslip6 -s /dev/ttyUSB0 fe80::cafe:cafe:cafe:2

=> ping test (USEMODULE += gnrc_icmpv6_echo):

	@host# ping6 -f fe80::cafe:cafe:cafe:1%tun0
	@riot# ping6 fe80::cafe:cafe:cafe:1
	
> **Note:** MODULE_ETHOS (Ethernet over Serial) multiplexes, the same way SLIP does framing, 
several channels on a unique serial port. On Riot, it multiplexes the STDIO and an Ethernet
netdev. On the host side, the ethos application demux the 2 channels and creates a tun interface
the same way tunslip6 does. 
Ethos is an alternative when no additional UART is available for SLIP, but with a mux penalty. 

_______________________________________________________________________________________________

DEBUG flags are defined in cpu.h
XTIMER flags are defined in periph_conf.h

/*****************************************************************
 * (xtimer) configuration notes:
 * - the higher ARCH_TIMER_HZ, the shortest the capture time
 *   but the higher the risk of having an overflow in the
 *   usec to ticks conversion.
 * - the lower ARCH_TIMER_HZ, the longer the capture time,
 *   but the lower the risk of having an overflow in the
 *   usec to ticks conversion.
 * - (ARCH_TIMER_HZ/XTIMER_HZ) ratio should be selected to
 *   avoid float usec/ticks conversion overhead, hence an
 *   integer ratio or better a power of 2 (to replace divs
 *   by shifts.)
 *   Remember than one 32 bits integer mult/div instruction
 *   costs 31 cycles, and that float mult/div is lib/fpu
 *   dependent and could cost multiple div/mul instr.
 *   aps25 has no fpu.
 *
 * - the absolute XTIMER_BACKOFF/XTIMER_OVERHEAD constants
 *   must be carefully calibrated according to the
 *   XTIMER_HZ/CPU_CLOCK_FREQ ratio. Misconfiguration could
 *   cause overshoots, high spin ratios and past timers.
 *   APS_TIMER_CALIBRATE proposes some values automatically,
 *   but must not be used in production.
 *
 * - XTIMER_WIDTH is the hw timer width (32).
 *   It can be lowered to test torture the xtimer overflow
 *   detection mechanism, but has no impact on overclocking
 *   or on ticks conversion because xtimer uses internally
 *   a 32 bit tick representation whatever the XTIMER_WIDTH.
 *   usec are thus converted to 32 bits xticks and then mapped
 *   by xtimer to llticks on the defined width.
 *
 * - If ARCH_TIMER_MULT!=1 is to be implemented (not the case),
 *   XTIMER_WIDTH must be adapted.
 *   iow if we want to run the timer at a higher frequency than
 *   declared to xtimer without overclocking effect:
 *     hwtick=ARCH_TIMER_MULT*xtick
 *     XTIMER_WIDTH<=(32-ceil(log(ARCH_TIMER_MULT)/log(2))
 *   Internal mult can be used to avoid overflow effects
 *   on public usec to tick conversion api's, and also to
 *   decrease the capture latency.
 *
 * - APS_OVERCLOCK must be defined to avoid assert failure
 *   when XTIMER_HZ is not the timer_running_frequency.
 *   We can overclock the system by pretending a lower freq
 *   than real, or a lower ticks per usec than real, but the
 *   later has nasty side effects.
 */
 
//Xtimer calibration figures for 12.5MHz CPU clock:
// clocks close to or below 1MHz suffer from the capture toggle restriction.
// tests/xtimer_usleep_short allows to verify the overhead/backoff constants validity
// tests/xtimer_usleep       allows to verify the constant drift time (due more to xtimer code and cpu speed than to timer precision).
// tests/xtimer_periodic_wakeup allows to verify the min/max drift times due to spin or irq 
// tests/xtimer_drift        allows to verify the overflow detection validity
2018-05-18 13:03:47,141 - INFO # APS Timer calibration[0,0] constants: raw(freq=50000000 read=156 isr=264) overhead= 266 ( 5/MHz) backoff=1330 (26/MHz) isr-backoff=1596 (31/MHz)
2018-05-18 13:03:47,158 - INFO # APS Timer calibration[0,1] constants: raw(freq=25000000 read= 92 isr=120) overhead= 122 ( 4/MHz) backoff= 610 (24/MHz) isr-backoff= 732 (29/MHz)
2018-05-18 13:03:47,189 - INFO # APS Timer calibration[0,2] constants: raw(freq=12500000 read= 46 isr= 62) overhead=  64 ( 5/MHz) backoff= 320 (25/MHz) isr-backoff= 384 (30/MHz)
2018-05-18 13:03:47,206 - INFO # APS Timer calibration[0,3] constants: raw(freq= 6250000 read= 23 isr= 32) overhead=  34 ( 5/MHz) backoff= 170 (27/MHz) isr-backoff= 204 (32/MHz)
2018-05-18 13:03:47,237 - INFO # APS Timer calibration[1,0] constants: raw(freq=25000000 read= 92 isr=120) overhead= 122 ( 4/MHz) backoff= 610 (24/MHz) isr-backoff= 732 (29/MHz)
2018-05-18 13:03:47,254 - INFO # APS Timer calibration[1,1] constants: raw(freq=12500000 read= 46 isr= 62) overhead=  64 ( 5/MHz) backoff= 320 (25/MHz) isr-backoff= 384 (30/MHz)
2018-05-18 13:03:47,285 - INFO # APS Timer calibration[1,2] constants: raw(freq= 6250000 read= 23 isr= 32) overhead=  34 ( 5/MHz) backoff= 170 (27/MHz) isr-backoff= 204 (32/MHz)
2018-05-18 13:03:47,302 - INFO # APS Timer calibration[1,3] constants: raw(freq= 3125000 read= 13 isr= 15) overhead=  17 ( 5/MHz) backoff=  85 (27/MHz) isr-backoff= 102 (32/MHz)
2018-05-18 13:03:47,333 - INFO # APS Timer calibration[2,0] constants: raw(freq=12500000 read= 46 isr= 62) overhead=  64 ( 5/MHz) backoff= 320 (25/MHz) isr-backoff= 384 (30/MHz)
2018-05-18 13:03:47,350 - INFO # APS Timer calibration[2,1] constants: raw(freq= 6250000 read= 23 isr= 32) overhead=  34 ( 5/MHz) backoff= 170 (27/MHz) isr-backoff= 204 (32/MHz)
2018-05-18 13:03:47,381 - INFO # APS Timer calibration[2,2] constants: raw(freq= 3125000 read= 13 isr= 15) overhead=  17 ( 5/MHz) backoff=  85 (27/MHz) isr-backoff= 102 (32/MHz)
2018-05-18 13:03:47,398 - INFO # APS Timer calibration[2,3] constants: raw(freq= 1562500 read=  8 isr=  8) overhead=  10 ( 6/MHz) backoff=  50 (32/MHz) isr-backoff=  60 (38/MHz)
2018-05-18 13:03:47,429 - INFO # APS Timer calibration[3,0] constants: raw(freq= 3125000 read= 13 isr= 15) overhead=  17 ( 5/MHz) backoff=  85 (27/MHz) isr-backoff= 102 (32/MHz)
2018-05-18 13:03:47,446 - INFO # APS Timer calibration[3,1] constants: raw(freq= 1562500 read=  8 isr=  8) overhead=  10 ( 6/MHz) backoff=  50 (32/MHz) isr-backoff=  60 (38/MHz)
2018-05-18 13:03:47,526 - INFO # APS Timer calibration[3,2] constants: raw(freq=  781250 read=3536 isr=429493202) overhead=429493204 (549751301/MHz) backoff=2147466020 (2748756505/MHz) isr-backoff=2576959224 (3298507806/MHz)
2018-05-18 13:03:47,653 - INFO # APS Timer calibration[3,3] constants: raw(freq=  390625 read=1867 isr=429494867) overhead=429494869 (1099506864/MHz) backoff=2147474345 (4294967295/MHz) isr-backoff=2576969214 (4294967295/MHz)
