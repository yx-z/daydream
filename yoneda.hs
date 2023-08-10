{-# LANGUAGE RankNTypes #-}

import Data.Functor (fmap)

{-
Yoneda Lemma in Haskell loosely says:
`forall x . (a -> x) -> F x` is "isomorhpic" to `F a`
, where `a` is a type, `F` is a functor, and `forall x` means it's a polymorphic/generic function
, that behaves universally for all type `x`.

You can think Functor as a generic container type on `a`.
e.g. `[Int]` is a functor where `F` is `[]` (`List`) and `a` is `Int`.

In a concrete example, suppose `a` is `Int` and `F` is `[]`, aka "List".
Note that `x` cannot be fixed in the setup, because we want a generic "for all x".
-}

-- Now given `F a` is
fa :: [Int]
fa = [1, 3, 5, 7, 9]

-- We construct `(a -> x) -> F x` as
ax2fx :: forall x . (Int -> x) -> [x]
ax2fx intToX = fmap intToX fa -- where `fa` is captured like a closure
{-
Such definition already gives us a "cookbook" to go from `F a` to the polymorphic function.
Now for the other direction (given `ax2fx`), how can we get `F a`?

Note that `x` can be any type, so let `x = a = Int`. And we define `intToX` to be
-}
identity :: Int -> Int
identity a = a

-- Hence `ax2fx identity` gives back `fa = [1, 3, 5, 7, 9]`
main = putStrLn (show (ax2fx identity))
