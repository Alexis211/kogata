
return {
  -- Definitions from proto/fs.h

  -- FILE TYPES
  FT_REGULAR = 0,
  FT_DIR = 0x01,
  FT_DEV = 0x02,
  FT_BLOCKDEV = 0x04,
  FT_CHARDEV = 0x08,
  FT_CHANNEL = 0x10,
  FT_FRAMEBUFFER = 0x20,

  -- FILE MODES
  FM_READ = 0x01,
  FM_WRITE = 0x02,
  FM_READDIR = 0x04,
  FM_MMAP = 0x08,
  FM_CREATE = 0x10,
  FM_TRUNC = 0x20,
  FM_APPEND = 0x40,
  FM_IOCTL = 0x100,
  FM_BLOCKING = 0x200,
  FM_DCREATE = 0x1000,
  FM_DMOVE = 0x2000,
  FM_DDELETE = 0x4000,
  FM_ALL_MODES = 0xFFFF,

  -- IOCTL calls
  IOCTL_BLOCKDEV_GET_BLOCK_SIZE = 40,
  IOCTL_BLOCKDEV_GET_BLOCK_COUNT = 41,

  -- Modes for select call
  SEL_READ = 0x01,
  SEL_WRITE = 0x02,
  SEL_ERROR = 0x04,


  -- Definitions from proto/launch.h

  -- STANDARD FILE DESCRIPTORS
  STD_FD_TTY_STDIO = 1,
  STD_FD_STDIN = 2,
  STD_FD_STDOUT = 3,
  STD_FD_STDERR = 4,
  STD_FD_GIP = 5,
  STD_FD_GIOSRV = 10,
  STD_FD_TTYSRV = 11,

  -- Definitions from proto/fb.h

  -- FRAMEBUFFER TYPES : TODO

  -- FRAMEBUFFER IOCTLs
  IOCTL_FB_GET_INFO = 1,
  IOCTL_FBDEV_GET_MODE_INFO = 10,
  IOCTL_FBDEV_SET_MODE = 11,

  -- Definitions from proto/proc.h
  PS_LOADING = 1,
  PS_RUNNING = 2,
  PS_FINISHED = 3,
  PS_FAILURE = 4,
  PS_KILLED = 5,
}
