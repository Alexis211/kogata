return function(s)
	local fonts = {}

	local BO = s.host_settings.cc.Output

	for _, file in pairs(Collect('res/fonts/*.s')) do
		local out = BO(s.host_settings, PathBase(file)) .. '.bf'
		AddJob(out, "nasm font " .. out, "nasm -o " .. out .. " " .. file)
		AddDependency(out, file)
		table.insert(fonts, out)
	end

	for _, file in pairs(Collect('res/fonts/*.c')) do
		local obj = Compile(s.host_settings, file)
		local tgt = BO(s.host_settings, PathBase(file) .. '_tmp')
		local bin = Link(s.host_settings, tgt, obj)

		local out = BO(s.host_settings, PathBase(file)) .. '.bf'
		AddJob(out, "call font " .. bin, "./" .. bin .. " > " .. out)
		AddDependency(out, bin)
		table.insert(fonts, out)
	end

	return fonts
end
