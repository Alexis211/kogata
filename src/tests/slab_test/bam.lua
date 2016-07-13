local source = {"src/common/libkogata/slab_alloc.c",
				"src/tests/slab_test/slab_test.c"}
local obj = Compile(host_settings, source)

local bin = Link(host_settings, "build/tests/slab_test", obj)

local slab_test = "build/tests/slab_test.log"
AddJob(slab_test, "slab_test", "./" .. bin .. " > " .. slab_test)
AddDependency(slab_test, bin)

table.insert(tests, slab_test)
