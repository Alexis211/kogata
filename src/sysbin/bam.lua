return function(s, lib)
	local function sysbin_settings(name)
		local s = TableDeepCopy(s.user_settings)
		s.link.flags:Add("-T src/lib/linker.ld")
		return s
	end

	local function sysbin_exe(name, deps)
		local s = sysbin_settings(name)

		local src = Collect('src/sysbin/' .. name .. '/*.c')
		local obj = Compile(s, src)

		return Link(s, 'sysbin/' .. name .. ".bin", {obj, deps})
	end

	return {
		sysbin_exe('init', {lib.libkogata}),
		sysbin_exe('giosrv', {lib.libkogata}),
		sysbin_exe('login', {lib.libkogata}),
		sysbin_exe('terminal', {lib.libkogata}),
		sysbin_exe('shell', {lib.libkogata}),

		sysbin_exe('lua', {lib.liblua}),
		-- sysbin_exe('luac', {liblua}),
		sysbin_exe('lx', {lib.liblua}),
	}
end
