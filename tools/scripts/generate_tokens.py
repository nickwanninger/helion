
tokens = [
"eof",
#literals
"num",
"id",
"type",
"str",
"keyword",
#grouping
"leftc",
"rightc",
"lefta",
"righta",
"lefts",
"rights",
#reserved
"def",
"indent",
"dedent",
"or", "and", "not",
"do",
"if",
"then",
"else",
"for",
"while",
"return",
#binary operations
"equal", "notequal",
"gt", "gte",
"lt", "lte",
"add", "sub", "mul", "div", "mod",
#misc
"comment",
]



with open('include/helion/tokens.inc', 'w') as f:
    for i, tok in enumerate(tokens):
        f.write(f'TOKEN(tok_{tok}, {i}, "{tok}")\n')
