#!/usr/bin/env python3

import math
import operator as op

Symbol = str
Number = (int, float)
Atom   = (Symbol, Number)
List   = list
Expr   = (Atom, List)

class Env(dict):
    "An environment: a dict of {'var': val} pairs, with an outer env"
    def __init__(self, parms=(), args=(), outer=None):
        self.update(zip(parms, args))
        self.outer = outer
    def find(self, var):
        "Find the innermost Env where var appears"
        return self if (var in self) else self.outer.find(var)

class Procedure(object):
    "A user-defined Scheme procedure"
    def __init__(self, parms, body, env):
        self.parms, self.body, self.env = parms, body, env
    def __call__(self, *args):
        return eval(self.body, Env(self.parms, args, self.env))


def tokenize(chars: str) -> list:
    "Convert a string of characters into a list of tokens"
    return chars.replace('(', ' ( ').replace(')', ' ) ').split()

def parse(program: str) -> Expr:
    "Read a Scheme expression from a string"
    return read_from_tokens(tokenize(program))

def read_from_tokens(tokens: list) -> Expr:
    "Read an expression from a token sequence"
    if len(tokens) == 0:
        raise SyntaxError("Unexpected EOF")
    token = tokens.pop(0)
    if token == '(':
        L = []
        while tokens[0] != ')':
            L.append(read_from_tokens(tokens))
        tokens.pop(0)
        return L
    elif token == ')':
        raise SyntaxError("Unexpected )")
    else:
        return atom(token)

def atom(token: str) -> Atom:
    "Numbers become Number, anything else becomes a Symbol"
    try:
        return int(token)
    except ValueError:
        try:
            return float(token)
        except ValueError:
            return Symbol(token)

def standard_env() -> Env:
    "An environment with some Scheme standard procedures"
    env = Env()
    env.update(vars(math)) # Sin, cos, sqrt, etc
    env.update({
        '+': op.add, '-': op.sub, '*': op.mul, '/': op.truediv,
        '>': op.gt, '<': op.lt, '>=': op.ge, '<=': op.le, '=': op.eq,
        'abs':          abs,
        'append':       op.add,
        'apply':        lambda proc, args: proc(*args),
        'begin':        lambda *x: x[-1],
        'car':          lambda x: x[0],
        'cdr':          lambda x: x[1:],
        'cons':         lambda x, y: [x] + y,
        'eq?':          op.is_,
        'expt':         pow,
        'equal?':       op.eq,
        'length':       len,
        'list':         lambda *x: List(x),
        'list?':        lambda x: isinstance(x, List),
        'map':          map,
        'max':          max,
        'min':          min,
        'not':          op.not_,
        'null?':        lambda x: x == [],
        'number?':      lambda x: isinstance(x, Number),
        'print':        print,
        'procedure?':   callable,
        'round':        round,
        'symbol?':      lambda x: isinstance(x, Symbol),
    })
    return env

global_env = standard_env()

def eval(x: Expr, env=global_env) -> Expr:
    "Evaluate an expression in an environment"
    if isinstance(x, Symbol):           # Var
        return env.find(x)[x]
    elif isinstance(x, Number):         # Constant
        return x
    op, *args = x
    if op == 'quote':                   # Quotation
        return args[0]
    elif op == 'if':                    # Conditional
        (test, conseq, alt) = args
        exp = (conseq if eval(test, env) else alt)
        return eval(exp, env)
    elif op == 'define':                # Definition
        (symbol, exp) = args
        env[symbol] = eval(exp, env)
        return env[symbol]
    elif op == 'set!':                  # Assignment
        (symbol, exp) = args
        env.find(symbol)[symbol] = eval(exp, env)
        return env.find(symbol)[symbol]
    elif op == 'lambda':                # Procedure
        (parms, body) = args
        return Procedure(parms, body, env)
    else:                               # Procedure call
        proc = eval(op, env)
        vals = [eval(arg, env) for arg in args]
        return proc(*vals)

def repl(prompt='lis.py> '):
    "A prompt-read-eval-print loop"
    while True:
        val = eval(parse(input(prompt)))
        if val is not None:
            print(schemestr(val))

def schemestr(exp):
    "Convert a python object into a Scheme-readable string"
    if (isinstance(exp, List)):
        return '(' + ' '.join(map(schemestr, exp)) + ')'
    else:
        return str(exp)


if __name__ == '__main__':
    test = "(begin (define r 10) (* pi (* r r)))"
    print(parse(test))
    print(eval(parse(test)))
    repl()
