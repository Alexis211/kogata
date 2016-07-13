local kernel_settings = TableDeepCopy(common_settings)

kernel_settings.cc.includes:Add("src/kernel/include")

kernel_settings.link.flags:Add("-T src/kernel/linker.ld",
							   "-Xlinker -Map=build/kernel.map")

kernel_source = {
	Collect('src/kernel/core/*.s'),
	Collect('src/kernel/dev/*.s'),
	Collect('src/kernel/core/*.c'),
	Collect('src/kernel/dev/*.c'),
	Collect('src/kernel/fs/*.c'),
	Collect('src/kernel/user/*.c'),
}
kernel_obj = Compile(kernel_settings, kernel_source)

kernel = Link(kernel_settings, "kernel", {kernel_obj, common_libkogata, 
										  common_libc, common_libalgo})

