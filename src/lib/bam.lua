local function lib(name)
	local source = Collect('src/lib/' .. name .. '/*.c')
	return Compile(user_settings, source)
end

libkogata = {lib('libkogata'), common_libc, common_libalgo, common_libkogata}

liblua = lib('lua')
liblualib = lib('lualib')
