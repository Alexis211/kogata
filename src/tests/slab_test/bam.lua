return function(s)
	local source = {"src/common/libkogata/slab_alloc.c",
					"src/tests/slab_test/slab_test.c"}
	local obj = Compile(s.host_settings, source)

	local f = s.host_settings.cc.Output(s.host_settings, "tests/slab_test")
	local bin = Link(s.host_settings, f, obj)

	local slab_test = f .. ".log"
	AddJob(slab_test, "slab_test", "./" .. bin .. " > " .. slab_test)
	AddDependency(slab_test, bin)

	return slab_test
end
