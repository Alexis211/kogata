local function lib(name)
	local source = Collect('src/common/' .. name .. '/*.c')
	return Compile(common_settings, source) 
end

common_libc = lib('libc')
common_libkogata = lib('libkogata')
