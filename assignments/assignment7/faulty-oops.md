# Faulty OOPS

## Log output
```
[  329.590954] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[  329.591698] Mem abort info:
[  329.592088]   ESR = 0x0000000096000045
[  329.592219]   EC = 0x25: DABT (current EL), IL = 32 bits
[  329.592366]   SET = 0, FnV = 0
[  329.592457]   EA = 0, S1PTW = 0
[  329.592556]   FSC = 0x05: level 1 translation fault
[  329.592754] Data abort info:
[  329.592860]   ISV = 0, ISS = 0x00000045
[  329.592980]   CM = 0, WnR = 1
[  329.593161] user pgtable: 4k pages, 39-bit VAs, pgdp=0000000043605000
[  329.593377] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[  329.593841] Internal error: Oops: 0000000096000045 [#1] PREEMPT SMP
[  329.594161] Modules linked in: scull(O) hello(O) faulty(O)
[  329.594524] CPU: 3 PID: 308 Comm: sh Tainted: G           O      5.15.194-yocto-standard #1
[  329.594759] Hardware name: linux,dummy-virt (DT)
[  329.595016] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[  329.595216] pc : faulty_write+0x18/0x20 [faulty]
[  329.595626] lr : vfs_write+0xf8/0x2a0
[  329.595737] sp : ffffffc00966bd80
[  329.595829] x29: ffffffc00966bd80 x28: ffffff8003d2e040 x27: 0000000000000000
[  329.596300] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[  329.596506] x23: 0000000000000000 x22: ffffffc00966bdc0 x21: 000000556aad7db0
[  329.596707] x20: ffffff800374fe00 x19: 0000000000000005 x18: 0000000000000000
[  329.596922] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[  329.597136] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[  329.597342] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008272108
[  329.597577] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[  329.597787] x5 : 0000000000000001 x4 : ffffffc000ba0000 x3 : ffffffc00966bdc0
[  329.597993] x2 : 0000000000000005 x1 : 0000000000000000 x0 : 0000000000000000
[  329.598292] Call trace:
[  329.598403]  faulty_write+0x18/0x20 [faulty]
[  329.598567]  ksys_write+0x74/0x110
[  329.598673]  __arm64_sys_write+0x24/0x30
[  329.598800]  invoke_syscall+0x5c/0x130
[  329.598921]  el0_svc_common.constprop.0+0x4c/0x100
[  329.599056]  do_el0_svc+0x4c/0xc0
[  329.599159]  el0_svc+0x28/0x80
[  329.599253]  el0t_64_sync_handler+0xa4/0x130
[  329.599380]  el0t_64_sync+0x1a0/0x1a4
[  329.599588] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[  329.600084] ---[ end trace 65db3bec0f96ee7f ]---
/bin/start_getty: line 20:   308 Segmentation fault      ${setsid:-} ${getty} -L $1 $2 $3
```

## Analysis

1. In the first line, "Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000" gives a strong indication about accessing NULL pointer.
2. PC (program counter) tells us that error happened in module faulty in function faulty_write 0x18 bytes into function which is 0x20 bytes long
3. At the end of the message is the call trace.
