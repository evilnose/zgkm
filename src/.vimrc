syntax on
colo elflord

au BufRead,BufNewFile *.py set expandtab
au BufRead,BufNewFile *.c set noexpandtab
au BufRead,BufNewFile *.h set noexpandtab
au BufRead,BufNewFile Makefile* set noexpandtab

set tabstop=4
set softtabstop=4
set shiftwidth=4
set expandtab
set shiftwidth=4
set autoindent
filetype plugin indent on
set nosmartindent
set cindent
set backspace=indent,eol,start
set noundofile
set textwidth=120

set number relativenumber

set wildmenu
set wildmode=longest:full,full

set ruler

set hidden

set incsearch
set hlsearch

set ignorecase
set smartcase

set showmatch

set noerrorbells
set novisualbell
set t_vb=
set tm=500

set encoding=utf-8

" In visual mode, pressing * or # searches for the current selection
vnoremap <silent> * :<C-u>call VisualSelection('', '')<CR>/<C-R>=@/<CR><CR>
vnoremap <silent> # :<C-u>call VisualSelection('', '')<CR>?<C-R>=@/<CR><CR>

map <space> /
map <c-space> ?

" Faster move between windows
map <C-j> <C-W>j
map <C-k> <C-W>k
map <C-h> <C-W>h
map <C-l> <C-W>l
