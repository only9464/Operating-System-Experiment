/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */
  
#define __LIBRARY__		// 定义该变量是为了包括定义在 unistd.h 中的内嵌汇编代码等信息。
#include <unistd.h>		// *.h 头文件所在的默认目录是include/，则在代码中就不用明确指明位置。
						// 如果不是UNIX 的标准头文件，则需要指明所在的目录，并用双引号括住。
						// 标准符号常数与类型文件。定义了各种符号常数和类型，并申明了各种函数。
						// 如果定义了__LIBRARY__，则还包括系统调用号和内嵌汇编代码_syscall0()等。
#include <time.h>		// 时间类型头文件。其中最主要定义了tm 结构和一些有关时间的函数原形。
#include<sys/stat.h> 
/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!),  until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
/*
 * 我们需要下面这些内嵌语句 - 从内核空间创建进程(forking)将导致没有写时复制（COPY ON WRITE）!!!
 * 直到一个执行execve 调用。这对堆栈可能带来问题。处理的方法是在fork()调用之后不让main()使用
 * 任何堆栈。因此就不能有函数调用 - 这意味着fork 也要使用内嵌的代码，否则我们在从fork()退出
 * 时就要使用堆栈了。
 * 实际上只有pause 和fork 需要使用内嵌方式，以保证从main()中不会弄乱堆栈，但是我们同时还
 * 定义了其它一些函数。
 */
 
/*static*/ inline _syscall0(int,fork)				// 是unistd.h 中的内嵌宏代码。以嵌入汇编的形式调用
													// Linux 的系统调用中断0x80。该中断是所有系统调用的
													// 入口。该条语句实际上是int fork()创建进程系统调用。
													// syscall0 名称中最后的0 表示无参数，1 表示1 个参数。
/*static*/ inline _syscall0(int,pause)				// int pause()系统调用：暂停进程的执行，直到收到一个信号。
/*static*/ inline _syscall1(int,setup,void *,BIOS)	// int setup(void * BIOS)系统调用，仅用于
													// linux 初始化（仅在这个程序中被调用）。
/*static*/ inline _syscall0(int,sync)				// int sync()系统调用：更新文件系统。
_syscall2(int,mkdir,const char*,name,mode_t,mode) 
_syscall3(int,mknod,const char*,filename,mode_t,mode,dev_t,dev) 

#include <linux/tty.h>					// tty 头文件，定义了有关tty_io，串行通信方面的参数、常数。
#include <linux/sched.h>				// 调度程序头文件，定义了任务结构task_struct、第1 个初始任务
										// 的数据。还有一些以宏的形式定义的有关描述符参数设置和获取的
										// 嵌入式汇编函数程序。
#include <linux/head.h>					// head 头文件，定义了段描述符的简单结构，和几个选择符常量。
#include <asm/system.h>					// 系统头文件。以宏的形式定义了许多有关设置或修改
										// 描述符/中断门等的嵌入式汇编子程序。
#include <asm/io.h>						// io 头文件。以宏的嵌入汇编程序形式定义对io 端口操作的函数。
#include <stddef.h>						// 标准定义头文件。定义了NULL, offsetof(TYPE, MEMBER)。
#include <stdarg.h>						// 标准参数头文件。以宏的形式定义变量参数列表。主要说明了-个
										// 类型(va_list)和三个宏(va_start, va_arg 和va_end)，vsprintf、
										// vprintf、vfprintf。
#include <unistd.h>
#include <fcntl.h>						// 文件控制头文件。用于文件及其描述符的操作控制常数符号的定义。
#include <sys/types.h>					// 类型头文件。定义了基本的系统数据类型。
#include <linux/fs.h>					// 文件系统头文件。定义文件表结构（file,buffer_head,m_inode 等）。

static char printbuf[1024];

extern int vsprintf();					// 送格式化输出到一字符串中（在kernel/vsprintf.c)
extern void init(void);
extern void blk_dev_init(void);			// 块设备初始化子程序（kernel/blk_drv/ll_rw_blk.c）
extern void chr_dev_init(void);			// 字符设备初始化（kernel/chr_drv/tty_io.c）
extern void hd_init(void);				// 硬盘初始化程序（kernel/blk_drv/hd.c）
extern void floppy_init(void);			// 软驱初始化程序（kernel/blk_drv/floppy.c）
extern void mem_init(long start, long end);			// 内存管理初始化（mm/memory.c）
extern long rd_init(long mem_start, int length);	//虚拟盘初始化(kernel/blk_drv/ramdisk.c)
extern long kernel_mktime(struct tm * tm);			// 建立内核时间（秒）。
extern long startup_time;							// 内核启动时间（开机时间）（秒）。

/*
 * This is set up by the setup-routine at boot-time
 */
/*
 * 以下这些数据是由setup.s 程序在引导时间设置的。
 */
#define EXT_MEM_K (*(unsigned short *)0x90002)
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */
/*
 * 是啊，是啊，下面这段程序很差劲，但我不知道如何正确地实现，而且好象它还能运行。如果有
 * 关于实时时钟更多的资料，那我很感兴趣。这些都是试探出来的，以及看了一些bios 程序，呵！
 */

/* 这段宏读取 CMOS 实时时钟信息。*/
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)	// 将BCD 码转换成数字。

// 该子程序取CMOS 时钟，并设置开机时间 startup_time(秒)。
static void time_init(void)
{
	struct tm time;

	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;							// 机器具有的内存（字节数）。
static long buffer_memory_end = 0;					// 高速缓冲区末端地址。
static long main_memory_start = 0;					// 主内存（将用于分页）开始的位置。

struct drive_info { char dummy[32]; } drive_info;	// 用于存放硬盘参数表信息。


void start(void)		/* This really IS void, no error here. */
{						/* The startup routine assumes (well, ...) this */
						/* 这里确实是void，并没错。在startup 程序(head.s)中就是这样假设的.
  			 			 * 参见head.s 程序
  			 			 */
  			 			 
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
/*
 * 此时中断仍被禁止着，做完必要的设置后就将其开启。
 */
  
    // 下面这段代码用于保存：
    // 根设备号 ROOT_DEV； 	  高速缓存末端地址 buffer_memory_end；
    // 机器内存数 memory_end；主内存开始地址 main_memory_start；
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	memory_end = (1<<20) + (EXT_MEM_K<<10);		// 内存大小=1Mb 字节+扩展内存(k)*1024 字节。
	memory_end &= 0xfffff000;					// 忽略不到4Kb（1 页）的内存数。
	if (memory_end > 16*1024*1024)				// 如果内存超过16Mb，则按16Mb 计。
		memory_end = 16*1024*1024;
	if (memory_end > 12*1024*1024) 				// 如果内存>12Mb，则设置缓冲区末端=4Mb
		buffer_memory_end = 4*1024*1024;
	else if (memory_end > 6*1024*1024)			// 否则如果内存>6Mb，则设置缓冲区末端=2Mb
		buffer_memory_end = 2*1024*1024;
	else
		buffer_memory_end = 1*1024*1024;		// 否则则设置缓冲区末端=1Mb
	main_memory_start = buffer_memory_end;		// 主内存起始位置=缓冲区末端；
#ifdef RAMDISK					// 如果定义了虚拟盘，则主内存将减少。
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);
#endif
	// 以下是内核进行所有方面的初始化工作。阅读时最好跟着调用的程序深入进去看，实在看
  	// 不下去了，就先放一放，看下一个初始化调用 -- 这是经验之谈。
	mem_init(main_memory_start,memory_end);
	trap_init();				// 陷阱门（硬件中断向量）初始化。（kernel/traps.c）
	blk_dev_init();				// 块设备初始化。 （kernel/blk_dev/ll_rw_blk.c）
	chr_dev_init();				// 字符设备初始化。 （kernel/chr_dev/tty_io.c）
	tty_init();					// tty 初始化。 （kernel/chr_dev/tty_io.c）
	time_init();				// 设置开机启动时间:startup_time。
	sched_init();				// 调度程序初始化(加载了任务0 的tr, ldtr) （kernel/sched.c）
	buffer_init(buffer_memory_end);	// 缓冲管理初始化，建内存链表等。（fs/buffer.c）
	hd_init();					// 硬盘初始化。 （kernel/blk_dev/hd.c）
	floppy_init();				// 软驱初始化。 （kernel/blk_dev/floppy.c）
	sti();						// 所有初始化工作都做完了，开启中断。
	// 下面过程通过在堆栈中设置的参数，利用中断返回指令切换到任务0。
	move_to_user_mode();		// 移到用户模式。 （include/asm/system.h）
	if (!fork()) {		/* we count on this going ok */
		init();
	}
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */
 /* 注意!! 对于任何其它的任务，'pause()'将意味着我们必须等待收到一个信号才会返回就
  * 绪运行态，但任务0（task0）是唯一的意外情况（参见'schedule()'），因为任务0 在任何
  * 空闲时间里都会被激活（当没有其它任务在运行时），因此对于任务0'pause()'仅意味着我们
  * 返回来查看是否有其它任务可以运行，如果没有的话我们就回到这里，一直循环执行'pause()'。
  */
	for(;;) pause();
}

static int printf(const char *fmt, ...)
{
	// 产生格式化信息并输出到标准输出设备stdout(1)，这里是指屏幕上显示。参数'*fmt'指定输出将
	// 采用的格式，参见各种标准C 语言书籍。该子程序正好是vsprintf 如何使用的一个例子。该程
	// 序使用vsprintf()将格式化的字符串放入printbuf 缓冲区，然后用write()将缓冲区的内容输出
	// 到标准设备（1--stdout）。
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

static char * argv_rc[] = { "/bin/sh", NULL };	// 调用执行程序时参数的字符串数组。
static char * envp_rc[] = { "HOME=/", NULL };	// 调用执行程序时的环境字符串数组。

static char * argv[] = { "-/bin/sh",NULL };		// 同上
static char * envp[] = { "HOME=/usr/root", NULL };

void init(void)
{
	int pid,i;

	// 读取硬盘参数包括分区表信息并建立虚拟盘和安装根文件系统设备。
    // 该函数是在上面宏定义的，对应函数是sys_setup()，在kernel/blk_drv/hd.c。
	setup((void *) &drive_info);
	(void) open("/dev/tty0",O_RDWR,0);	// 用读写访问方式打开设备“/dev/tty0”
	(void) dup(0);						// 复制句柄，产生句柄1 号 -- stdout 标准输出设备。
	(void) dup(0);						// 复制句柄，产生句柄2 号 -- stderr 标准出错输出设备。
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);
	mkdir("/proc",0755); 
	mknod("/proc/psinfo",S_IFPROC|0444,0); 
	 // 下面fork()用于创建一个子进程(子任务)。对于被创建的子进程，fork()将返回0 值，对于
  	 // 原(父进程)将返回子进程的进程号。下面是子进程执行的内容。该子进程关闭了句柄0(stdin)，
  	 // 以只读方式打开/etc/rc 文件，并执行/bin/sh 程序，所带参数和环境变量分别由argv_rc 
 	 // 和envp_rc 数组给出。关闭句柄0并立刻打开/etc/rc文件的作用是把标准输入stdin 重定向
 	 // 到/etc/rc 文件。这样shell程序/bin/sh就可以运行rc文件中设置命令。由于这里sh的运行
 	 // 方式是非交互式的，因此在执行完rc文件中的命令后就会立刻退出进程2也随之结束。关于
 	 // execve（）函数的说明请参见fs/exec.c 程序。函数 _exit（）退出时的出错码：1-操作未
 	 // 许可；2-文件或目录不存在
	if (!(pid=fork())) {
		close(0);
		if (open("/etc/rc",O_RDONLY,0))
			_exit(1);				// 如果打开文件失败，则退出(/lib/_exit.c)。
		execve("/bin/sh",argv_rc,envp_rc);	// 装入/bin/sh 程序并执行。
		_exit(2);					// 若execve()执行失败则退出(出错码2,“文件或目录不存在”)。
	}
	
	// 下面是父进程执行的语句。wait()是等待子进程停止或终止，其返回值应是子进程的进程号(pid)。
 	// 这三句的作用是父进程等待子进程的结束。&i 是存放返回状态信息的位置。如果wait()返回值不
  	// 等于子进程号，则继续等待。
	if (pid>0)
		while (pid != wait(&i))
			/* nothing */;
			
	// 如果执行到这里，说明刚创建的子进程的执行已停止或终止了。下面循环中首先再创建一个子进程，
  	// 如果出错，则显示“初始化程序创建子进程失败”的信息并继续执行。对于所创建的子进程关闭所有
 	// 以前还遗留的句柄(stdin, stdout, stderr)，新创建一个会话并设置进程组号，然后重新打开
 	// /dev/tty0 作为stdin，并复制成stdout 和stderr。再次执行系统解释程序/bin/sh。但这次执行所
  	// 选用的参数和环境数组另选了一套。然后父进程再次运行wait()等待。如果子进程又停止了执行，则
  	// 在标准输出上显示出错信息“子进程pid 停止了运行，返回码是i”，然后继续重试下去…，形成“大”死循环。
	while (1) {
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) {
			close(0);close(1);close(2);
			setsid();
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();		// 同步操作，刷新缓冲区
	}
	_exit(0);		/* NOTE! _exit, not exit() */
	// _exit() 和 exit（）都用于正常终止一个函数。但 _exit（）直接是一个sys_exit系统调用，而 exit（）
	// 通常是普通函数库中的一个函数。它会先执行一些清除操作，例如调用执行各终止处理程序。关闭所有标准IO
	// 等，然后调用 sys_exit.
}
