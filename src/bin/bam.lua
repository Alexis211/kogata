local function bin_settings(name)
	local s = TableDeepCopy(user_settings)
	s.link.flags:Add("-T src/bin/linker.ld",
					 "-Xlinker -Map=build/bin/" .. name .. ".map")
	return s
end

local function bin_exe(name, moredeps)
	local s = sysbin_settings(name)

	local src = Collect('src/bin/' .. name .. '/*.c')
	local obj = Compile(s, src)

	return Link(s, 'bin/' .. name .. ".bin", {obj, libkogata, moredeps})
end

bin = {
	-- bin_exe('lua', {liblua, liblualib}),
	-- bin_exe('luac', {liblua, liblualib}),
}
