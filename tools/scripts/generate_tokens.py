
tokens = [
    "eof",
    "num",
    "var",
    "type",
    "self_var",
    "str",
    "keyword",

    "left_curly",
    "right_curly",
    "left_angle",
    "right_angle",
    "left_square",
    "right_square",
    "left_paren",
    "right_paren",

    "class",
    "def",
    "newline",
    "indent",
    "dedent",
    "or",
    "and",
    "not",
    "is_type",
    "do",
    "if",
    "then",
    "else",
    "for",
    "while",
    "return",

    "assign",
    "arrow",
    "pipe",
    "equal",
    "notequal",
    "gt",
    "gte",
    "lt",
    "lte",
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "dot",
    "comma",
    "comment",
]



with open('include/helion/tokens.inc', 'w') as f:
    for i, tok in enumerate(tokens):
        f.write(f'TOKEN(tok_{tok}, {i}, "{tok}")\n')
