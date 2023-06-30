def uncurry(func):
    def uncurried(*args):
        res = func
        for arg in args:
            res = res(arg)
        return res

    return uncurried
