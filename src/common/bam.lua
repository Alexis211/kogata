return function(s)
	local function lib(name)
		local source = Collect('src/common/' .. name .. '/*.c')
		return Compile(s.common_settings, source) 
	end

	return {
		libc = lib('libc'),
		libkogata = lib('libkogata')
	}
end
