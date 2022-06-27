"---- Colors ------
"set Colorscheme
colorscheme delek

"---- Spaces, Tabs and Indentation -----
" nr of visual spaces per tab
set tabstop=3
" indent with tab size
set shiftwidth=3
" number of spaces in tab when editing
" tabs are spaces = expandtab
set softtabstop=3 expandtab
" autoindent lines
set autoindent
" indent braces automatically
set cindent

"---- UI Config -------
" show line number
set number
"show command line in bottom bar
set showcmd
"highligth current line
set cursorline
"redraw screen only when we need to, not in the middle of macros
set lazyredraw
"highlight matching parantheses
set showmatch
" use mouse as pointer
set mouse=a

"---- Search --------
"search as characters are entered, to disable nohlsearch
set incsearch
" highlight matches
set hlsearch
"turn off search higlight using :nohlsearch
"search case insensitive if no case letters are entered
set ignorecase
"search case sensitive if case letters are entered
set smartcase

"---- Swap line up and down ----
nnoremap <C-S-Up> <Up>"add"ap<Up>
nnoremap <C-S-Down> "add"ap

"---- Save and compile and execute ---
map <F8> :w <CR> :!gcc % -o eb <CR> :!read -n1<CR>
map <F9> :!./eb <CR>

"---- Auto close brackets
inoremap " ""<left>
inoremap ' ''<left>
inoremap ( ()<left>
inoremap [ []<left>
inoremap { {}<left>
inoremap {<CR> {<CR>}<ESC>O
inoremap {;<CR> {<CR>};<ESC>O
