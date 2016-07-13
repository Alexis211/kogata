local source = Collect('src/common/libc/*.c')

common_libc = Compile(common_settings, source)
