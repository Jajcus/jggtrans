:set tabstop=8
:set nocompatible
:%s/\s\+$//
:%s/)\s*\(\_$\|\s\+\)\_\s*{/){/g
:%s/else\s*\(\_$\|\s\+\)\_\s*{/else{/g
:retab!
:update
:q
:q!
