local source = Collect('src/lib/libkogata/*.c')
local obj = Compile(user_settings, source)

libkogata = {obj, common_libc, common_libalgo, common_libkogata}
