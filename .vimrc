autocmd VimEnter * NERDTree
autocmd VimEnter * wincmd p

map <C-F12> :!ctags -R --sort=yes --c-kinds=+pl --fields=+iaS --extra=+q kernel<CR>


let g:syntastic_always_populate_loc_list = 1
let g:syntastic_auto_loc_list = 1
let g:syntastic_check_on_open = 1
let g:syntastic_check_on_wq = 0

let g:syntastic_c_gcc_exec = 'i586-elf-gcc'
let g:syntastic_c_include_dirs = [ 'src/kernel/include', 'src/kernel', 'src/common/include' ]
let g:syntastic_c_compiler_options = '-ffreestanding -Wall -Wextra -std=gnu99 -Wno-unused-parameter'
