/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <grp.h>
#if ENABLE_GSHADOW
#  include <gshadow.h>
#endif
#include <pwd.h>
#include <shadow.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

/* Users managed by systemd-homed. See https://systemd.io/UIDS-GIDS for details how this range fits into the rest of the world */
#define HOME_UID_MIN 60001
#define HOME_UID_MAX 60513

/* Users mapped from host into a container */
#define MAP_UID_MIN 60514
#define MAP_UID_MAX 60577

bool uid_is_valid(uid_t uid);

static inline bool gid_is_valid(gid_t gid) {
        return uid_is_valid((uid_t) gid);
}

int parse_uid(const char *s, uid_t* ret_uid);
int parse_uid_range(const char *s, uid_t *ret_lower, uid_t *ret_upper);

static inline int parse_gid(const char *s, gid_t *ret_gid) {
        return parse_uid(s, (uid_t*) ret_gid);
}

char* getlogname_malloc(void);
char* getusername_malloc(void);

typedef enum UserCredsFlags {
        USER_CREDS_PREFER_NSS    = 1 << 0,  /* if set, only synthesize user records if database lacks them. Normally we bypass the userdb entirely for the records we can synthesize */
        USER_CREDS_ALLOW_MISSING = 1 << 1,  /* if a numeric UID string is resolved, be OK if there's no record for it */
        USER_CREDS_CLEAN         = 1 << 2,  /* try to clean up shell and home fields with invalid data */
} UserCredsFlags;

int get_user_creds(const char **username, uid_t *uid, gid_t *gid, const char **home, const char **shell, UserCredsFlags flags);
int get_group_creds(const char **groupname, gid_t *gid, UserCredsFlags flags);

char* uid_to_name(uid_t uid);
char* gid_to_name(gid_t gid);

int in_gid(gid_t gid);
int in_group(const char *name);

int merge_gid_lists(const gid_t *list1, size_t size1, const gid_t *list2, size_t size2, gid_t **result);
int getgroups_alloc(gid_t** gids);

int get_home_dir(char **ret);
int get_shell(char **ret);

int reset_uid_gid(void);

int take_etc_passwd_lock(const char *root);

#define UID_INVALID ((uid_t) -1)
#define GID_INVALID ((gid_t) -1)

#define UID_NOBODY ((uid_t) 65534U)
#define GID_NOBODY ((gid_t) 65534U)

/* If REMOUNT_IDMAP_HOST_ROOT is set for remount_idmap() we'll include a mapping here that maps the host root
 * user accessing the idmapped mount to the this user ID on the backing fs. This is the last valid UID in the
 * *signed* 32bit range. You might wonder why precisely use this specific UID for this purpose? Well, we
 * definitely cannot use the first 0…65536 UIDs for that, since in most cases that's precisely the file range
 * we intend to map to some high UID range, and since UID mappings have to be bijective we thus cannot use
 * them at all. Furthermore the UID range beyond INT32_MAX (i.e. the range above the signed 32bit range) is
 * icky, since many APIs cannot use it (example: setfsuid() returns the old UID as signed integer). Following
 * our usual logic of assigning a 16bit UID range to each container, so that the upper 16bit of a 32bit UID
 * value indicate kind of a "container ID" and the lower 16bit map directly to the intended user you can read
 * this specific UID as the "nobody" user of the container with ID 0x7FFF, which is kinda nice. */
#define UID_MAPPED_ROOT ((uid_t) (INT32_MAX-1))
#define GID_MAPPED_ROOT ((gid_t) (INT32_MAX-1))

#define ETC_PASSWD_LOCK_PATH "/etc/.pwd.lock"

/* The following macros add 1 when converting things, since UID 0 is a valid UID, while the pointer
 * NULL is special */
#define PTR_TO_UID(p) ((uid_t) (((uintptr_t) (p))-1))
#define UID_TO_PTR(u) ((void*) (((uintptr_t) (u))+1))

#define PTR_TO_GID(p) ((gid_t) (((uintptr_t) (p))-1))
#define GID_TO_PTR(u) ((void*) (((uintptr_t) (u))+1))

static inline bool userns_supported(void) {
        return access("/proc/self/uid_map", F_OK) >= 0;
}

typedef enum ValidUserFlags {
        VALID_USER_RELAX         = 1 << 0,
        VALID_USER_WARN          = 1 << 1,
        VALID_USER_ALLOW_NUMERIC = 1 << 2,
} ValidUserFlags;

bool valid_user_group_name(const char *u, ValidUserFlags flags);
bool valid_gecos(const char *d);
char *mangle_gecos(const char *d);
bool valid_home(const char *p);

static inline bool valid_shell(const char *p) {
        /* We have the same requirements, so just piggy-back on the home check.
         *
         * Let's ignore /etc/shells because this is only applicable to real and
         * not system users. It is also incompatible with the idea of empty /etc.
         */
        return valid_home(p);
}

int maybe_setgroups(size_t size, const gid_t *list);

bool synthesize_nobody(void);

int fgetpwent_sane(FILE *stream, struct passwd **pw);
int fgetspent_sane(FILE *stream, struct spwd **sp);
int fgetgrent_sane(FILE *stream, struct group **gr);
int putpwent_sane(const struct passwd *pw, FILE *stream);
int putspent_sane(const struct spwd *sp, FILE *stream);
int putgrent_sane(const struct group *gr, FILE *stream);
#if ENABLE_GSHADOW
int fgetsgent_sane(FILE *stream, struct sgrp **sg);
int putsgent_sane(const struct sgrp *sg, FILE *stream);
#endif

bool is_nologin_shell(const char *shell);
const char* default_root_shell(const char *root);

int is_this_me(const char *username);

const char *get_home_root(void);

static inline bool hashed_password_is_locked_or_invalid(const char *password) {
        return password && password[0] != '$';
}

/* A locked *and* invalid password for "struct spwd"'s .sp_pwdp and "struct passwd"'s .pw_passwd field */
#define PASSWORD_LOCKED_AND_INVALID "!*"

/* A password indicating "look in shadow file, please!" for "struct passwd"'s .pw_passwd */
#define PASSWORD_SEE_SHADOW "x"

/* A password indicating "hey, no password required for login" */
#define PASSWORD_NONE ""
