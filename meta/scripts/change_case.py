import enum
from typing import Callable, Any

SEPARATORS = {' ', '\t', '.', '_', '/', '-'}

CaseFn = Callable[[str, int, int], str]

def _change_case(text: str, fn: CaseFn) -> str:
    if not text:
        return ""

    was_lower = False
    si = 0  # string index
    wi = 0  # word index
    result = []
    
    i = 0
    # Equivalent to s.eat(RE_SEP) before the loop
    if i < len(text) and text[i] in SEPARATORS:
        i += 1

    while i < len(text):
        char = text[i]
        
        if char in SEPARATORS:
            sep = fn(' ', si, wi)
            if sep:
                result.append(sep)
            wi = 0
            was_lower = False
            i += 1
        else:
            # Handle CamelCase/PascalCase boundaries
            if was_lower and char.isupper():
                sep = fn(' ', si, wi)
                if sep:
                    result.append(sep)
                wi = 0

            was_lower = char.islower()
            res_char = fn(char, si, wi)
            if res_char:
                result.append(res_char)
                
            si += 1
            wi += 1
            i += 1

    return "".join(result)

# --- Case mapping functions ---

def _to_no_case(char: str, si: int, wi: int) -> str:
    return char.lower()

def _to_pascal_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return "" # Equivalent to C++ '\0'
    if wi == 0:
        return char.upper()
    return char.lower()

def _to_camel_case(char: str, si: int, wi: int) -> str:
    if si == 0:
        return char.lower()
    return _to_pascal_case(char, si, wi)

def _to_title_case(char: str, si: int, wi: int) -> str:
    if wi == 0:
        return char.upper()
    return char.lower()

def _to_constant_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return '_'
    return char.upper()

def _to_dot_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return '.'
    return char

def _to_header_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return '-'
    if wi == 0:
        return char.upper()
    return char.lower()

def _to_param_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return '-'
    return char.lower()

def _to_path_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return '/'
    return char

def _to_sentence_case(char: str, si: int, wi: int) -> str:
    if si == 0:
        return char.upper()
    return char.lower()

def _to_snake_case(char: str, si: int, wi: int) -> str:
    if char == ' ':
        return '_'
    return char.lower()

def _to_swap_case(char: str, si: int, wi: int) -> str:
    if char.islower():
        return char.upper()
    if char.isupper():
        return char.lower()
    return char

def _to_lower_case(char: str, si: int, wi: int) -> str:
    return char.lower()

def _to_lower_first_case(char: str, si: int, wi: int) -> str:
    if si == 0:
        return char.lower()
    return char

def _to_upper_case(char: str, si: int, wi: int) -> str:
    return char.upper()

def _to_upper_first_case(char: str, si: int, wi: int) -> str:
    if si == 0:
        return char.upper()
    return char

def _to_sponge_case(char: str, si: int, wi: int) -> str:
    if si % 2 == 0:
        return char.upper()
    return char.lower()

# --- Enum Definition ---

class Case(enum.Enum):
    DEFAULT = enum.auto()
    CAMEL = enum.auto()
    CAPITAL = enum.auto()
    CONSTANT = enum.auto()
    DOT = enum.auto()
    HEADER = enum.auto()
    NO = enum.auto()
    PARAM = enum.auto()
    PASCAL = enum.auto()
    PATH = enum.auto()
    SENTENCE = enum.auto()
    SNAKE = enum.auto()
    TITLE = enum.auto()
    SWAP = enum.auto()
    LOWER = enum.auto()
    LOWER_FIRST = enum.auto()
    UPPER = enum.auto()
    UPPER_FIRST = enum.auto()
    SPONGE = enum.auto()

# --- Exported Functions ---

def to_default_case(text: str) -> str: return text
def to_camel_case(text: str) -> str: return _change_case(text, _to_camel_case)
def to_capital_case(text: str) -> str: return _change_case(text, _to_title_case)
def to_constant_case(text: str) -> str: return _change_case(text, _to_constant_case)
def to_dot_case(text: str) -> str: return _change_case(text, _to_dot_case)
def to_header_case(text: str) -> str: return _change_case(text, _to_header_case)
def to_no_case(text: str) -> str: return _change_case(text, _to_no_case)
def to_param_case(text: str) -> str: return _change_case(text, _to_param_case)
def to_pascal_case(text: str) -> str: return _change_case(text, _to_pascal_case)
def to_path_case(text: str) -> str: return _change_case(text, _to_path_case)
def to_sentence_case(text: str) -> str: return _change_case(text, _to_sentence_case)
def to_snake_case(text: str) -> str: return _change_case(text, _to_snake_case)
def to_title_case(text: str) -> str: return _change_case(text, _to_title_case)
def to_swap_case(text: str) -> str: return _change_case(text, _to_swap_case)
def to_lower_case(text: str) -> str: return _change_case(text, _to_lower_case)
def to_lower_first_case(text: str) -> str: return _change_case(text, _to_lower_first_case)
def to_upper_case(text: str) -> str: return _change_case(text, _to_upper_case)
def to_upper_first_case(text: str) -> str: return _change_case(text, _to_upper_first_case)
def to_sponge_case(text: str) -> str: return _change_case(text, _to_sponge_case)

def change_case(text: str, target_case: Case) -> str:
    dispatch = {
        Case.CAMEL: to_camel_case,
        Case.CAPITAL: to_capital_case,
        Case.CONSTANT: to_constant_case,
        Case.DOT: to_dot_case,
        Case.HEADER: to_header_case,
        Case.NO: to_no_case,
        Case.PARAM: to_param_case,
        Case.PASCAL: to_pascal_case,
        Case.PATH: to_path_case,
        Case.SENTENCE: to_sentence_case,
        Case.SNAKE: to_snake_case,
        Case.TITLE: to_title_case,
        Case.SWAP: to_swap_case,
        Case.LOWER: to_lower_case,
        Case.LOWER_FIRST: to_lower_first_case,
        Case.UPPER: to_upper_case,
        Case.UPPER_FIRST: to_upper_first_case,
        Case.SPONGE: to_sponge_case,
    }

    return dispatch.get(target_case, to_default_case)(text)

# --- Formatting Utilities ---

class Cased:
    """
    Equivalent to the C++ Cased struct and its Formatter.
    Utilizes Python's dunder __format__ method.
    """
    def __init__(self, inner: Any, target_case: Case):
        self._inner = inner
        self._case = target_case

    def __format__(self, format_spec: str) -> str:
        inner_str = format(self._inner, format_spec)
        return change_case(inner_str, self._case)

def cased(inner: Any, target_case: Case) -> Cased:
    return Cased(inner, target_case)
