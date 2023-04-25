import kotlin.math.abs

/* Problem: Given `num`, how to estimate `sqrt(num)`?
 * We'll see where "fixed point" kicks into the process. */

// Fix the precision we care about throughout the problem.
val TOL = 0.001
// Will be used later. This is randomly picked as our interest is not particularly in finding the best starting point.
val START_POINT = 1.0

// Helper functions
fun isClose(num1: Double, num2: Double) = abs(num1 - num2) < TOL
fun average(num1: Double, num2: Double) = (num1 + num2) / 2

// We will talk about this later.
fun getNextGuess(currGuess: Double, num: Double) = average(currGuess, num / currGuess)

// Heron's method
fun sqrtHeron(num: Double): Double {
    fun improve(currGuess: Double): Double {
        // We don't know the true answer, `sqrt(num)`, but we can check with `num`.
        if (isClose(currGuess * currGuess, num)) {
            return currGuess
        }
        return improve(getNextGuess(currGuess, num))
    }

    return improve(currGuess = START_POINT)
}

println(sqrtHeron(2.0)) // 1.4142156862745097

/* Why is it correct?
 * It's an iterative process of keep `improve`-ing `currGuess` until it `isGoodGuess`.
 * If `currGuess` is not close enough, we `improve` it with the magic `getNextGuess`.
 *
 * So why does Heron came up with `getNextGuess`?
 * The idea is if `currGuess` is smaller than `sqrt(num)`, then `num / currGuess` is bigger than `sqrt(num)`.
 * Hence we take the average between them, squeezing the range and converging to `sqrt(num)`.
 *
 * Moreover, we see `getNextGuess( sqrt(num) ) == sqrt(num)`.
 * Hence `sqrt` is a, so-called, fixed point of `getNextGuess`.
 * We can abstract out the process of "finding fixed point of `func`". */
fun getFixedPoint(func: (Double) -> Double): Double {
    fun improve(currPoint: Double): Double {
        val nextPoint = func(currPoint)
        // It's called "fixed" as we are applying `func` again and again, until the result is fixed/converged.
        if (isClose(currPoint, nextPoint)) {
            return currPoint
        }
        // Otherwise we continue applying `func`
        return improve(nextPoint)
    }
    return improve(currPoint = START_POINT)
}
/* This process requires `func` can be converged by continuous/repeated application.
 * It's a property of `func` itself, not magic of `getFixedPoint`. Examples of such function are
 * 0. fun half(num: Double) = num / 2, which converges to 0.0.
 * 1. Heron's method.
 * 2. We'll see one more (Newton's method). */

/* According to line 41, we can define `sqrt = getFixedPoint(getNextGuess)`. */
fun sqrtFromFixedPoint(num: Double) = getFixedPoint { getNextGuess(it, num) }

println(sqrtFromFixedPoint(2.0)) // 1.4142156862745097

/* Taking the "fixed point" idea to Newton's method.
 * Given `f: (Double) -> Double`, the root finding algorithm solves for `x` such that `f(x) == 0`.
 * i.e. `x == solveNewton(f) => f(solveNewton(f)) == f(x) == 0`.
 * It's done by `getNextSolution(currSolution) = currSolution - f(currSolution) / f'(currSolution)`,
 * where `f'` represents the derivative of `f` evaluated at `currSolution`. */

// This is a quick numerical estimation of derivative of `f` evaluated at `currSolution`.
// The method is called "finite difference".
fun getDerivative(f: (Double) -> Double, currSolution: Double) = (f(currSolution + TOL) - f(currSolution)) / TOL

fun getNextSolution(f: (Double) -> Double, currSolution: Double) =
        currSolution - f(currSolution) / getDerivative(f, currSolution)

fun solveNewton(f: (Double) -> Double): Double {
    fun improve(currSolution: Double): Double {
        if (isClose(f(currSolution), 0.0)) {
            return currSolution
        }
        return improve(getNextSolution(f, currSolution))
    }
    return improve(currSolution = START_POINT)
}
println(solveNewton { 2 * it + 3 }) // 2x + 3 == 0 => x == -1.5. My Kotlin actually says -1.5000000000002753.

/* We see that it's really a similar pattern to Heron's method of finding `sqrt(num)`.
 * Indeed Newton's method is a fixed point of `getNextSolution`.
 * `getNextSolution( solveNewton(f) ) == getNextSolution(x) == x - f(x) / f'(x) == x - 0 == x == solveNewton(f)`. */
fun solveNewtonFromFixedPoint(f: (Double) -> Double) = getFixedPoint { getNextSolution(f, it) }
println(solveNewtonFromFixedPoint { 2 * it + 3 }) // -1.5000000000002753

/* Finally, isn't `sqrt(num)` a root of `f(x) = x^2 - num == 0`? */
fun sqrtFromNewton(num: Double) = solveNewtonFromFixedPoint { it * it - num }
println(sqrtFromNewton(2.0)) // 1.4142165798805022
