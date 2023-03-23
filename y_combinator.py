# keywords: lambda calculus, y combinator, functional programing

# in lambda calculus land, there is no variable. everything is (anonymous/lambda) function and its application
# and our goal is to write a recursive function without self referencing the function variable name

# the trick is to fake a recursive function parameter, so the parameter is used when doing recursive calls

# start with usual factorial
fact = lambda n: 1 if n < 2 else n * fact(n - 1)
print(fact(5))

# use the trick to remove recursive call with variable `fact`
# but ideally `fact` shall only take a single argument `n`
# so we first have an internal `help`, and a "public" `fact`
help = lambda f, n: 1 if n < 2 else n * f(n - 1)
fact = lambda n: help(help, n)
# but see we need to supply `help` with first argument as itself

# so we need to do the same for `f(n - 1)` -> `f(f, n - 1)`
help = lambda f, n: 1 if n < 2 else n * f(f, n - 1)
fact = lambda n: help(help, n)
print(fact(5))

# in lambda calculus land, every function only takes a single parameter
# but there's easy conversion between function taking two or more arguments to single-argument functions
# the process is called currying (google for more info), just as below:
help = lambda f: (lambda n: 1 if n < 2 else n * f(f)(n - 1))
fact = lambda n: help(help)(n)
print(fact(5))

# remove variable `help` by, literally, copy-pasting it, while constructing `fact`
fact = lambda n: (lambda f: lambda n: 1 if n < 2 else n * f(f)(n - 1))(
    lambda f: lambda n: 1 if n < 2 else n * f(f)(n - 1))(n)
print(fact(5))
# our first goal is achieved! this can be applied without variable name:
print((lambda n: (lambda f: lambda n: 1 if n < 2 else n * f(f)(n - 1))(
    lambda f: lambda n: 1 if n < 2 else n * f(f)(n - 1))(n))(5))

# but this is complex and doesn't work generally...
# so next goal is to construct a `Y` combinator that turns any such recursive function into the ones we are familiar with,
# at the same time, still not self referencing function variable name

# reconsider `help` on line 15, and curry it to facilitate later constructing `Y`
fact = (lambda f: lambda n: 1 if n < 2 else n * f(n - 1))(fact)
#                                                         ^^^ we need a hypothetical `fact` taking one true argument `n`. like equations in math, fact is the unknown but pretend we have fact. then left hand side is exactly what we want to figure out 
# try doing copy-paste trick (copy first half to second half)
fact = (lambda f: lambda n: 1 if n < 2 else n * f(n - 1))(lambda f: lambda n: 1 if n < 2 else n * f(n - 1))
# we find out that `f` needs to be applied with two arguments, first with itself. just as `help` does on line 19
fact = (lambda f: lambda n: 1 if n < 2 else n * f(f)(n - 1))(lambda f: lambda n: 1 if n < 2 else n * f(f)(n - 1))
# to show everything works still
print(fact(5))

# reconsider this from line 44: 
fact = (lambda f: (lambda n: 1 if n < 2 else n * f(n - 1)))(fact)
# the first part is understandable, mostly the "business logic" of the recursive function
# so extract out the first half, where `R_` stands for Raw - later becomes our input to `Y`
R_fact = lambda f: lambda n: 1 if n < 2 else n * f(n - 1)
# then fact = R_fact(fact) <-> `fact` is called a fixed point of `R_fact`
# our goal is to figure out `fact`

# suppose we have an oracle `Y`, so that fact = Y(R_fact). Then Y(R_fact) = fact = R_fact(fact) = R_fact( Y(R_fact) )
#                                                                     line61   line58         replace fact one more time
# Now, separately,  we see R_fact(t) = (lambda x: R_fact(x))(t),
# then Y(R_fact) = R_fact( Y(R_fact) ) = (lambda x: R_fact(x))( Y(R_fact) )
# here think `x` as something similar to `f` above, i.e. a function
# see `Y(R_fact)` is essentially an abstract `fact` as above on line 44
# and it's also under the same circumstance of self referencing variable name
# so we need copy-paste trick (copy first half to second half) which gives:
# Y(R_fact) = (lambda x: R_fact(x))( lambda x: R_fact(x) )
# now same as line 48, we need to call `x` with itself.
# this is because, see how `R_fact` uses the input parameter `f` on line 57:
# `R_fact` only takes `f` that can be supplied with economic arguments, e.g. `n`
# without partial applying `x` with itself, `x` still takes a function (itself) as first argument.
# so we have Y(R_fact) = ( lambda x: R_fact(x(x)) )( lambda x: R_fact(x(x)) )
# extract `R_fact` out on the right hand side
# Y(R_fact) = (  lambda f: ( lambda x: f(x(x)) )( lambda x: f(x(x)) )  )(R_fact)
# and finally dropping `R_fact` gives `Y` combinator in theory (textbook version)
Y = lambda f: (lambda x: f(x(x)))(lambda x: f(x(x)))
# make it actually work with lazy-evaluation, as python evaluates function arguments eagerly
Y = lambda f: (lambda x: f(lambda z: x(x)(z)))(lambda x: f(lambda z: x(x)(z)))
fact = Y(R_fact)
print(fact(5))

# and it can be applied generally
R_fib = lambda f: lambda n: 1 if n < 3 else f(n - 2) + f(n - 1)
fib = Y(R_fib)
print([fib(i) for i in range(1, 10)])

# even for functions taking more than one arguments: a functional and recursive `zip`
# note that everything is curried
R_zip = lambda f: lambda xs: lambda ys: [] if not xs else [(xs[0], ys[0])] + f(xs[1:])(ys[1:])
print(Y(R_zip)([1, 2])(['a', 'b']))
