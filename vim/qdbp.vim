" TODO: This is a little scuffed. Should add smarter support for ?, etc
syntax match CustomSingleLineComment /;.*/
hi def link CustomSingleLineComment Comment

syntax region MyMultilineComment start=/(\*/ end=/\*)/
hi def link MyMultilineComment Comment

syntax match CustomInteger /\<[0-9]\+\>/
hi def link CustomInteger Number

syntax match CustomLowerVar /\<[a-z][a-zA-Z0-9_]*\>/
hi def link CustomLowerVar Identifier

syntax match UppercaseVariableHash /#\<[A-Z][a-zA-Z0-9_]*\>/
hi def link UppercaseVariableHash Type

syntax match MyString /"[^"\\]*\(\\"[^"\\]*\)*"/
highlight link MyString String

syntax match UppercaseVariable /\<[A-Z][a-zA-Z0-9_]*\>/
hi def link UppercaseVariable PreProc
