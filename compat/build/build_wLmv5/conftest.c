/* confdefs.h.  */
/* confdefs.h */
#define PACKAGE_NAME "compat_mlnx"
#define PACKAGE_TARNAME "compat_mlnx"
#define PACKAGE_VERSION "2.3"
#define PACKAGE_STRING "compat_mlnx 2.3"
#define PACKAGE_BUGREPORT "http://support.mellanox.com/SupportWeb/service_center/SelfService"
#define PACKAGE_URL ""
#define PACKAGE "compat_mlnx"
#define VERSION "2.3"
#define STDC_HEADERS 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_UNISTD_H 1
#define SIZEOF_UNSIGNED_LONG_LONG 8
/* end confdefs.h.  */

#include <linux/kernel.h>

		#include <net/devlink.h>

int
main (void)
{

		struct devlink_fmsg *fmsg;
		int err;
		int value;

		err =  devlink_fmsg_binary_put(fmsg, &value, 2);

  ;
  return 0;
}
