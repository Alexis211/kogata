return function(s, common)
	local kernel_settings = TableDeepCopy(s.common_settings)

	kernel_settings.cc.includes:Add("src/common/include/kogata",
									"src/kernel/include")

	kernel_settings.link.flags:Add("-T src/kernel/linker.ld")

	kernel_source = {
		Collect('src/kernel/core/*.s'),
		Collect('src/kernel/dev/*.s'),
		Collect('src/kernel/core/*.c'),
		Collect('src/kernel/dev/*.c'),
		Collect('src/kernel/fs/*.c'),
		Collect('src/kernel/user/*.c'),
	}
	obj = Compile(kernel_settings, kernel_source)

	return {
		obj = obj,
		bin = Link(kernel_settings, "kernel", {obj, common.libc, common.libkogata})
	}
end
