/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-12-03
 * ================================
 */

/* postfix source, src/util/chroot_uid.c */

#include "zcc.h"
#include <pwd.h>
#include <grp.h>


namespace zcc
{

bool chroot_user(const char *root_dir, const char *user_name)
{
    struct passwd *pwd;
    uid_t uid;
    gid_t gid;

    /*
     * Look up the uid/gid before entering the jail, and save them so they
     * can't be clobbered. Set up the primary and secondary groups.
     */
    uid = 0;
    if (user_name != 0) {
        if ((pwd = getpwnam(user_name)) == 0) {
            zcc_fatal("chroot_user: unknown user: %s", user_name);
            errno = ENONET;
            return false;
        }
        uid = pwd->pw_uid;
        gid = pwd->pw_gid;
        if (setgid(gid) < 0) {
            zcc_fatal("chroot_user: setgid %ld : %m", (long)gid);
            return false;
        }
        if (initgroups(user_name, gid) < 0) {
            zcc_fatal("chroot_user: initgroups: %m");
            return false;
        }
    }

    /*
     * Enter the jail.
     */
    if (root_dir) {
        if (chroot(root_dir)) {
            zcc_fatal("chroot_user: chroot (%s) : %m", root_dir);
            return false;
        }
        if (chdir("/")) {
            zcc_fatal("chroot_user: chdir (/): %m");
            return false;
        }
    }

    /*
     * Drop the user privileges.
     */
    if (user_name != 0) {
        if (setuid(uid) < 0) {
            zcc_fatal("chroot_user: setuid %ld: %m", (long)uid);
            return false;
        }
    }

    return true;
}

}
