local function lib(name)
	local source = {Collect('src/lib/' .. name .. '/*.c'),
					Collect('src/lib/' .. name .. '/*.s')}
	return Compile(user_settings, source)
end

libc = {lib('libc'), common_libc, common_libkogata}

libkogata = {lib('libkogata'), libc}

liblua = {lib('lua'), libc}
