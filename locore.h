extern int sys_gettime();
extern int sys_clock(int c);
extern int sys_status();
extern void sys_handler(int i, void* fn);
extern void sys_pscomm(int i);
extern void sys_save(int l, void* d);
extern int sys_appnum();
extern void sys_execset(int a, int b, int c);
extern void sys_exec();