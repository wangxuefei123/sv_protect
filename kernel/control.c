#include <linux/fs.h>
#include <linux/ftrace.h>
#include <linux/kprobes.h>
#include <asm/unistd.h>

#include "priv.h"

MODULE_DESCRIPTION("defense and modify file to protect");
MODULE_AUTHOR("wang_xuefei <wxf_happylife@163.com>");
MODULE_LICENSE("GPL");


/* comment the following line to shut me up */
//#define INTERCEPT_DEBUG
//
//#ifdef INTERCEPT_DEBUG
//#define dbgprint(format,args...) \
//	printk("intercept: function:%s-L%d: "format, __FUNCTION__, __LINE__, ##args);
//#else
//#define dbgprint(format,args...) do {} while(0);
//#endif
//
//
///**
// * * the system call table
// * */
//void **my_table;
//
//unsigned int orig_cr0;
//
///**
// * * the original syscall functions
// * */
//asmlinkage long (*old_open) (char __user *filename, int flags, int mode);
//#ifdef __i386__
//asmlinkage int (*old_execve) (struct pt_regs regs);
//#else
//asmlinkage
//long old_execve(char __user *name, char __user * __user *argv,
//		char __user * __user *envp, struct pt_regs *regs);
//#endif
///** do_execve and do_fork */
//unsigned int can_exec_fork = 0;
//int (*new_do_execve) (char * filename,
//		char __user *__user *argv,
//		char __user *__user *envp,
//		struct pt_regs * regs);
//
//
//struct idtr {
//	unsigned short limit;
//	unsigned int base;
//} __attribute__ ((packed));
//
//
//struct idt {
//	unsigned short off1;
//	unsigned short sel;
//	unsigned char none, flags;
//	unsigned short off2;
//} __attribute__ ((packed));
//
//struct kprobe kp_exec;
//unsigned int can_intercept_fork_exec(void)
//{
//	int ret = 0;
//
//#ifndef CONFIG_KPROBES
//	return ret;
//#endif
//
//	kp_exec.symbol_name = "do_execve";
//
//	ret = register_kprobe(&kp_exec);
//	if (ret != 0 ) {
//		dbgprint("cannot find do_execve by kprobe.\n");
//		return 0;
//	}
//	new_do_execve = ( int (*)
//			(char *,
//			 char __user * __user *,
//			 char __user * __user *,
//			 struct pt_regs *
//			)
//			) kp_exec.addr;
//
//	dbgprint("do_execve at %p\n", (void *)kp_exec.addr);
//	unregister_kprobe(&kp_exec);
//
//	return 1;
//}
//
//
//
///**
// * * clear WP bit of CR0, and return the original value
// * */
//unsigned int clear_and_return_cr0(void)
//{
//	unsigned int cr0 = 0;
//	unsigned int ret;
//
//	asm volatile ("movl %%cr0, %%eax"
//			: "=a"(cr0)
//			);
//	ret = cr0;
//
//	/* clear the 20 bit of CR0, a.k.a WP bit */
//	cr0 &= 0xfffeffff;
//
//	asm volatile ("movl %%eax, %%cr0"
//			:
//			: "a"(cr0)
//			);
//	return ret;
//}
//
///** set CR0 with new value
// * *
// * * @val : new value to set in cr0
// * */
//void setback_cr0(unsigned int val)
//{
//	asm volatile ("movl %%eax, %%cr0"
//			:
//			: "a"(val)
//			);
//}
//
//
///**
// * * Return the first appearence of NEEDLE in HAYSTACK. 
// * * */
//static void *memmem(const void *haystack, size_t haystack_len,
//		const void *needle, size_t needle_len)
//{/*{{{*/
//	const char *begin;
//	const char *const last_possible
//		= (const char *) haystack + haystack_len - needle_len;
//
//	if (needle_len == 0)
//		/* The first occurrence of the empty string is deemed to occur at
//		 *            the beginning of the string. */
//		return (void *) haystack;
//
//	/* Sanity check, otherwise the loop might search through the whole
//	 *        memory. */
//	if (__builtin_expect(haystack_len < needle_len, 0))
//		return NULL;
//
//	for (begin = (const char *) haystack; begin <= last_possible;
//			++begin)
//		if (begin[0] == ((const char *) needle)[0]
//				&& !memcmp((const void *) &begin[1],
//					(const void *) ((const char *) needle + 1),
//					needle_len - 1))
//			return (void *) begin;
//
//	return NULL;
//}/*}}}*/
//
//
///**
// * * Find the location of sys_call_table
// * */
//static unsigned long get_sys_call_table(void)
//{/*{{{*/
//	/* we'll read first 100 bytes of int $0x80 */
//#define OFFSET_SYSCALL 100 
//
//	struct idtr idtr;
//	struct idt idt;
//	unsigned sys_call_off;
//	unsigned retval;
//	char sc_asm[OFFSET_SYSCALL], *p;
//
//	/* well, let's read IDTR */
//	asm("sidt %0":"=m"(idtr)
//			:
//			:"memory" );
//
//	dbgprint("idtr base at 0x%X\n", (unsigned int)idtr.base);
//
//	/* Read in IDT for vector 0x80 (syscall) */
//	memcpy(&idt, (char *) idtr.base + 8 * 0x80, sizeof(idt));
//
//	sys_call_off = (idt.off2 << 16) | idt.off1;
//
//	dbgprint("idt80: flags=%X sel=%X off=%X\n",
//			(unsigned) idt.flags, (unsigned) idt.sel, sys_call_off);
//
//	/* we have syscall routine address now, look for syscall table
//	 *        dispatch (indirect call) */
//	memcpy(sc_asm, (void *)sys_call_off, OFFSET_SYSCALL);
//
//	/**
//	 *      * Search opcode of `call sys_call_table(,eax,4)'
//	 *           */
//	p = (char *) memmem(sc_asm, OFFSET_SYSCALL, "\xff\x14\x85", 3);
//	if (p == NULL)
//		return 0;
//
//	retval = *(unsigned *) (p + 3);
//	if (p) {
//		dbgprint("sys_call_table at 0x%x, call "
//				"dispatch at 0x%x\n",retval, (unsigned int) p);
//	}
//
//	return retval;
//#undef OFFSET_SYSCALL
//}/*}}}*/
//
//
//
///**
// * * new_open - replace the original sys_open when initilazing,
// * * as well as be got rid of when removed
// * */
//asmlinkage long new_open(char *filename, int flags, int mode)
//{
//	dbgprint("hello\n");
//	return old_open (filename, flags, mode);
//}
//
//# ifdef __i386__
///**
// * * new_execve - you should change this function whenever the kernel's sys_execve()
// * * changes
// * */
//asmlinkage int new_execve(struct pt_regs regs)
//{
//	int error;
//	char *filename;
//
//	dbgprint("Hello\n");
//
//	filename = getname((char __user *)regs.bx );
//	if(strstr(filename,"vmware")){
//		error = PTR_ERR((void*)-EIO);
//		goto out;
//	}
//	error = PTR_ERR(filename);
//	if ( IS_ERR(filename) )
//		goto out;
//	dbgprint("file to execve: %s\n", filename);
//	error = new_do_execve(filename,
//			(char __user * __user *) regs.cx,
//			(char __user * __user *) regs.dx,
//			&regs);
//	if (error == 0) {
//		task_lock(current);
//		current->ptrace &= ~PT_DTRACE;
//		task_unlock(current);
//		set_thread_flag(TIF_IRET);
//	}
//	putname((const char*)filename);
//out:
//	return error;
//}
//#else
//	asmlinkage
//long new_execve(char __user *name, char __user * __user *argv,
//		char __user * __user *envp, struct pt_regs *regs)
//{
//	long error;
//	char *filename;
//
//	filename = getname(name);
//	error = PTR_ERR(filename);
//	if (IS_ERR(filename))
//		return error;
//	error = do_execve(filename, argv, envp, regs);
//	putname(filename);
//	return error;
//}
//#endif
//static int intercept_init(void)
//{
//	my_table = (void **)get_sys_call_table();
//	if (my_table == NULL)
//		return -1;
//
//	dbgprint("sys call table address %p\n", (void *) my_table);
//
//#define REPLACE(x) old_##x = my_table[__NR_##x];\
//	my_table[__NR_##x] = new_##x
//
//
//	//REPLACE(open);
//	can_exec_fork = can_intercept_fork_exec();
//	if(can_exec_fork == 1)
//		REPLACE(execve);
//
//#undef REPLACE
//	return 0;
//}

static int __init sv_protect_init(void)
{
	//int ret;
	printk("syscall intercept: Hi, poor linux!\n");
	file_init();
	process_init();

	//orig_cr0 = clear_and_return_cr0(); 
	//ret = intercept_init();
	//setback_cr0(orig_cr0);

	return 0;
}

static void __exit sv_protect_fini(void)
{
	file_fini();
	process_fini();
	printk("syscall intercept: bye, poor linux!\n");

#define RESTORE(x) my_table[__NR_##x] = old_##x

	//orig_cr0 = clear_and_return_cr0(); 
	//if(can_exec_fork == 1)
	//	RESTORE(execve);
	//setback_cr0(orig_cr0);

#undef RESTORE
}

module_init(sv_protect_init);
module_exit(sv_protect_fini);
