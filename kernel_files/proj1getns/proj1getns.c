#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
/* syscall number 336 */
SYSCALL_DEFINE1(proj1getns,
		struct timespec __user *, dst)
{
	struct timespec ts;	
	getnstimeofday(&ts);
	copy_to_user(dst, &ts, sizeof(ts));
	return 0;
}
