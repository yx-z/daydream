"""
Python metaprogramming playground
"""
from __future__ import annotations
from typing import TypeVar, Any, List, Generic
import builtins

# a loose mockup of Haskell's `let ... in ...` notation
# restrict variable scope in the expression only (in `__rshift__`)
class let:
    def __init__(self: Self, **kwargs: Any) -> None:
        self.existing_globals = {k: globals()[k] for k in kwargs if k in globals()}
        self.temp_globals = {k for k in kwargs if k not in globals()}
        globals().update(kwargs)
    
    T = TypeVar("T")
    def __rshift__(self: Self, val: T) -> T:
        globals().update(self.existing_globals)
        for k in self.temp_globals:
            del globals()[k]
        return val

data = [10, 11, 12]
first = 13
print(let(first=data[0]) >> {"first": first, "first_plus_one": first+1}) # {"first": 10, "first_plus_one": 11}
print(first == 13) # True

# turn functions into post-fix notations
V = TypeVar("V")
class of(Generic[V]):
    def __init__(self: Self, val: V) -> None:
        self.val = val

    def __getattr__(self: Self, name: name) -> Callable[..., Any]:
        attr = getattr(builtins, name) if name in dir(builtins) else globals()[name]
        if callable(attr):
            return lambda *args, **kwargs: of(attr(self.val, *args, **kwargs))
        return of(attr)
    
    def take(self: Self) -> V:
        return self.val

T = TypeVar("T")
def first(ls: List[T]) -> Optional[T]:
    return next(iter(ls), None)

def add_one(i: int) -> int:
    return i + 1

print(of([1,2,3]).first().add_one().take() == 2)  # True
