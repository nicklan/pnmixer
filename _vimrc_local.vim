" To load this file you need 'local_vimrc'
" https://github.com/LucHermitte/local_vimrc

" ALE cflags support
" https://github.com/w0rp/ale
"
" You can use this with clang or gcc checker, e.g.:
"   let g:ale_linters = {'c':['clang', 'clangtidy', 'cppcheck']}
if exists("g:ale_enabled")
	let g:ale_c_clang_options = "-DHAVE_CONFIG_H -Wall -Wextra -std=ansi -pedantic"
	let g:ale_c_gcc_options = "-DHAVE_CONFIG_H -Wall -Wextra -std=ansi -pedantic"

	" Sets or appends pkg-config cflags to g:ale_c_gcc_options and
	" ale_c_clang_options. Takes a list as argument.
	function! SetProjectCflags(pkgDeps)
		let new_list = deepcopy(a:pkgDeps)
		call map(new_list, {_, val -> systemlist('pkg-config --cflags ' . val)[0] })
		let cflags = join(new_list, ' ')
		let g:ale_c_clang_options .= ' ' . cflags
		let g:ale_c_gcc_options .= ' ' . cflags
	endfunction

	call SetProjectCflags(["glib-2.0",
		\ "gtk+-3.0",
		\ "x11",
		\ "alsa",
		\ "libnotify"])
endif
