from __future__ import annotations
from typing import TypeVar

# a loose mockup of Haskell's `let ... in ...` notation
# restrict variable scope in the expression only (in `__rshift__`)
class let:
    def __init__(self: Self, **kwargs: Any) -> None:
        self.prev_globals = {k: globals()[k] for k in kwargs if k in globals()}
        self.new_globals = {k for k in kwargs if k not in globals()}
        globals().update(kwargs)
    
    T = TypeVar("T")
    def __rshift__(self: Self, val: T) -> T:
        globals().update(self.prev_globals)
        for k in self.new_globals:
            del globals()[k]
        return val

data = [10, 11, 12]
first = 13
print(let(first=data[0]) >> {"first": first, "first_plus_one": first+1})
print(first == 13)
