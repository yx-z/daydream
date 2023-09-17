"""
Python metaprogramming playground
"""
from __future__ import annotations
from typing import TypeVar, Any, Iterable, Generic
import builtins

# =====
# 1. a loose mockup of Haskell's `let ... in ...` notation
#    to restrict variable scope in the expression only (in `__rshift__`)
class let:
    def __init__(self: Self, **kwargs: Any) -> None:
        self.existing_globals = {k: globals()[k] for k in kwargs if k in globals()}
        self.temp_globals = {k for k in kwargs if k not in globals()}
        globals().update({k: v for k, v in kwargs.items() if not isinstance(v, lazy)})
        for k, v in filter(lambda t: isinstance(t[1], lazy), kwargs.items()):
            globals()[k] = eval(v.expr)
    
    T = TypeVar("T")
    def __rshift__(self: Self, val: T) -> T:
        globals().update(self.existing_globals)
        for k in self.temp_globals:
            del globals()[k]
        return val

# sentinel class to lazy eval `kwargs` in `let`
class lazy:
    def __init__(self: Self, expr: str) -> None:
        if not isinstance(expr, str):
            raise RuntimeError(f"<class lazy> expects expression str, got {expr}")
        self.expr = expr

data = [10, 11, 12]
first = 13
print(let(first=data[0],
          second=lazy("first+1"),
          summed=lazy("first+second"),
         ) >> {"first": first, "second": second, "summed": summed}
     ) # {"first": 10, "second": 11, "summed": 21}
print(first == 13) # True


# =====
# 2. turn functions into post-fix notations
V = TypeVar("V")
class of(Generic[V]):
    def __init__(self: Self, val: V) -> None:
        self.val = val

    def __getattr__(self: Self, name: name) -> Any:
        attr = getattr(builtins, name) if name in dir(builtins) else globals()[name]
        if callable(attr):
            return lambda *args, **kwargs: of(attr(self.val, *args, **kwargs))
        return of(attr)
    
    def take(self: Self) -> V:
        return self.val

T = TypeVar("T")
def first(ls: Iterable[T]) -> Optional[T]:
    return next(iter(ls), None)

def add(i1: int, i2: int) -> int:
    return i1 + i2

print(of([1,2,3]).first().add(3).take() == 4)  # True
