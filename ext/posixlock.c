#ifdef _WIN32
#include "missing/file.h"
#endif

#include "ruby.h"
#include "rubyio.h"
#include "rubysig.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <errno.h>

extern VALUE rb_cFile;
static VALUE rb_cFile_F_LOCK;
static VALUE rb_cFile_F_LOCKR;
static VALUE rb_cFile_F_LOCKW;
static VALUE rb_cFile_F_TLOCK;
static VALUE rb_cFile_F_TLOCKR;
static VALUE rb_cFile_F_TLOCKW;
static VALUE rb_cFile_F_ULOCK;
static VALUE rb_cFile_F_TEST;
static VALUE rb_cFile_F_TESTR;
static VALUE rb_cFile_F_TESTW;

# ifndef LOCK_SH
#  define LOCK_SH 1
# endif
# ifndef LOCK_EX
#  define LOCK_EX 2
# endif
# ifndef LOCK_NB
#  define LOCK_NB 4
# endif
# ifndef LOCK_UN
#  define LOCK_UN 8
# endif

#ifndef F_LOCK
#define F_LOCK 1
#endif
#ifndef F_TLOCK
#define F_TLOCK 2
#endif
#ifndef F_ULOCK
#define F_ULOCK 4
#endif
#ifndef F_TEST
#define F_TEST 8
#endif
#ifndef F_LOCKR
#define F_LOCKR 16 
#endif
#ifndef F_LOCKW
#define F_LOCKW F_LOCK 
#endif
#ifndef F_TLOCKR
#define F_TLOCKR 32 
#endif
#ifndef F_TLOCKW
#define F_TLOCKW F_TLOCK 
#endif
#ifndef F_TESTR
#define F_TESTR 64 
#endif
#ifndef F_TESTW
#define F_TESTW F_TEST 
#endif



static int
posixlock (fd, operation)
     int fd;
     int operation;
{
  struct flock lock;

  switch (operation & ~LOCK_NB)
    {
    case LOCK_SH:
      lock.l_type = F_RDLCK;
      break;
    case LOCK_EX:
      lock.l_type = F_WRLCK;
      break;
    case LOCK_UN:
      lock.l_type = F_UNLCK;
      break;
    default:
      errno = EINVAL;
      return -1;
    }
  lock.l_whence = SEEK_SET;
  lock.l_start = lock.l_len = 0L;
  return fcntl (fd, (operation & LOCK_NB) ? F_SETLK : F_SETLKW, &lock);
}


static VALUE
rb_file_posixlock (obj, operation)
     VALUE obj;
     VALUE operation;
{
#ifndef __CHECKER__
  OpenFile *fptr;
  int ret;

  rb_secure (2);
  GetOpenFile (obj, fptr);

  if (fptr->mode & FMODE_WRITABLE)
    {
      fflush (GetWriteFile (fptr));
    }
retry:
  TRAP_BEG;
  ret = posixlock (fileno (fptr->f), NUM2INT (operation));
  TRAP_END;
  if (ret < 0)
    {
      switch (errno)
	{
	case EAGAIN:
	case EACCES:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:
#endif
	  return Qfalse;
	case EINTR:
#if defined(ERESTART)
	case ERESTART:
#endif
	  goto retry;
	}
      rb_sys_fail (fptr->path);
    }
#endif

  return INT2FIX (0);
}


static VALUE
rb_file_lockf (obj, cmd, len)
     VALUE obj;
     VALUE cmd;
     VALUE len;
{
#ifndef __CHECKER__
  OpenFile *fptr;
  int ret;
  int pid;
  int f_test;
  struct flock lock;
  char msg[1024];


  rb_secure (2);
  GetOpenFile (obj, fptr);

  snprintf (msg, 1024, "path <%s>", fptr->path);

  if (fptr->mode & FMODE_WRITABLE)
    {
      fflush (GetWriteFile (fptr));
    }
retry:
  TRAP_BEG;
  pid = -1;
  f_test = 0;
  lock.l_whence = SEEK_CUR;
  lock.l_start = 0L;
  lock.l_len = NUM2INT (len);
  switch (FIX2INT (cmd))
    {
    case F_LOCK:
      lock.l_type = F_WRLCK;
      ret = fcntl (fileno (fptr->f), F_SETLKW, &lock);
      break;
    case F_LOCKR:
      lock.l_type = F_RDLCK;
      ret = fcntl (fileno (fptr->f), F_SETLKW, &lock);
      break;
    case F_TLOCK:
      lock.l_type = F_WRLCK;
      ret = fcntl (fileno (fptr->f), F_SETLK, &lock);
      break;
    case F_TLOCKR:
      lock.l_type = F_RDLCK;
      ret = fcntl (fileno (fptr->f), F_SETLK, &lock);
      break;
    case F_ULOCK:
      lock.l_type = F_UNLCK;
      ret = fcntl (fileno (fptr->f), F_SETLK, &lock);
      break;
    case F_TEST:
      f_test = 1;
      lock.l_type = F_WRLCK;
      ret = fcntl (fileno (fptr->f), F_GETLK, &lock);
      if (ret == 0 && lock.l_type != F_UNLCK)
	{
	  pid = lock.l_pid;
	}
      break;
    case F_TESTR:
      f_test = 1;
      lock.l_type = F_RDLCK;
      ret = fcntl (fileno (fptr->f), F_GETLK, &lock);
      if (ret == 0 && lock.l_type != F_UNLCK)
	{
	  pid = lock.l_pid;
	}
      break;
    default:
      errno = EINVAL;
      snprintf (msg, 1024, "invalid cmd <%d>", FIX2INT (cmd));
      ret = -1;
    }
  TRAP_END;
  if (ret < 0)
    {
      switch (errno)
	{
	case EAGAIN:
	case EACCES:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:
#endif
	  return Qfalse;
	case EINTR:
#if defined(ERESTART)
	case ERESTART:
#endif
	  goto retry;
	}
      rb_sys_fail (msg);
    }
#endif

  if (f_test)
    {
      if (pid != -1)
	{
	  return INT2FIX (pid);
	}
      else
	{
	  return Qnil;
	}
    }
  else
    {
      return INT2FIX (0);
    }
}


void
Init_posixlock ()
{
  rb_cFile_F_LOCK = INT2FIX (F_LOCK);
  rb_cFile_F_LOCKR = INT2FIX (F_LOCKR);
  rb_cFile_F_LOCKW = INT2FIX (F_LOCKW);
  rb_cFile_F_TLOCK = INT2FIX (F_TLOCK);
  rb_cFile_F_TLOCKR = INT2FIX (F_TLOCKR);
  rb_cFile_F_TLOCKW = INT2FIX (F_TLOCKW);
  rb_cFile_F_ULOCK = INT2FIX (F_ULOCK);
  rb_cFile_F_TEST = INT2FIX (F_TEST);
  rb_cFile_F_TESTR = INT2FIX (F_TESTR);
  rb_cFile_F_TESTW = INT2FIX (F_TESTW);
  rb_define_const (rb_cFile, "F_LOCK", rb_cFile_F_LOCK);
  rb_define_const (rb_cFile, "F_LOCKR", rb_cFile_F_LOCKR);
  rb_define_const (rb_cFile, "F_LOCKW", rb_cFile_F_LOCKW);
  rb_define_const (rb_cFile, "F_TLOCK", rb_cFile_F_TLOCK);
  rb_define_const (rb_cFile, "F_TLOCKR", rb_cFile_F_TLOCKR);
  rb_define_const (rb_cFile, "F_TLOCKW", rb_cFile_F_TLOCKW);
  rb_define_const (rb_cFile, "F_ULOCK", rb_cFile_F_ULOCK);
  rb_define_const (rb_cFile, "F_TEST", rb_cFile_F_TEST);
  rb_define_const (rb_cFile, "F_TESTR", rb_cFile_F_TESTR);
  rb_define_const (rb_cFile, "F_TESTW", rb_cFile_F_TESTW);
  rb_define_method (rb_cFile, "lockf", rb_file_lockf, 2);
  rb_define_method (rb_cFile, "posixlock", rb_file_posixlock, 1);
}
