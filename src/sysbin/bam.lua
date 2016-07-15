local function sysbin_settings(name)
	local s = TableDeepCopy(user_settings)
	s.link.flags:Add("-T src/sysbin/linker.ld",
					 "-Xlinker -Map=build/sysbin/" .. name .. ".map")
	return s
end

local function sysbin_exe(name, moredeps)
	local s = sysbin_settings(name)

	local src = Collect('src/sysbin/' .. name .. '/*.c')
	local obj = Compile(s, src)

	return Link(s, 'sysbin/' .. name .. ".bin", {obj, libkogata, moredeps})
end

sysbin = {
	sysbin_exe('init'),
	sysbin_exe('giosrv'),
	sysbin_exe('login'),
	sysbin_exe('terminal'),
	--sysbin_exe('shell', {liblua, liblualib}),
	sysbin_exe('shell'),
}
