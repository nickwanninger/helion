
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

    "typedef", # actually the 'type' symbol, but that's taken
    "extends",
    "def",
    "term",
    "indent",
    "dedent",
    "or",
    "and",
    "not",
    "let",
    "global",
    "const",
    "some",
    "is_type",
    "colon",
    "do",
    "if",
    "then",
    "else",
    "elif",
    "for",
    "while",
    "return",
    "nil",

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
    "end",
]



with open('include/helion/tokens.inc', 'w') as f:
    for i, tok in enumerate(tokens):
        f.write(f'TOKEN(tok_{tok}, {i}, "{tok}")\n')
