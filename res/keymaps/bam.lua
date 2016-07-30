return function(s)
	local keymaps = {}

	local BO = s.host_settings.cc.Output

	for _, file in pairs(Collect('res/keymaps/*.c')) do
		local obj = Compile(s.host_settings, file)
		local tgt = BO(s.host_settings, PathBase(file) .. '_tmp')
		local bin = Link(s.host_settings, tgt, obj)

		local out = BO(s.host_settings, PathBase(file)) .. '.km'
		AddJob(out, "call font " .. bin, "./" .. bin .. " > " .. out)
		AddDependency(out, bin)
		table.insert(keymaps, out)
	end

	return keymaps
end
