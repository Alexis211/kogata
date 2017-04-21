return function(s, common)
	local function lib(name)
		local source = {Collect('src/lib/' .. name .. '/*.c'),
						Collect('src/lib/' .. name .. '/*.s')}
		return Compile(s.user_settings, source)
	end
	
	local libc = {lib('libc'), common.libc, common.libkogata}

	return {
		libc = libc,
		libkogata = {lib('libkogata')},
		liblua = {lib('lua')}
	}
end
