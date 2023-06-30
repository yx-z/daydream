def uncurry(func):
    def uncurried(*args):
        res = func
        for arg in args:
            res = res(arg)
        return res

    return uncurried


print(uncurry(lambda x: lambda y: lambda z: x + (y * z))(1, 2, 3) == 7)
