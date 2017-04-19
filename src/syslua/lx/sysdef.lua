-- Constant definitions based on common/include/proto/fs.h

return {
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
}
