import kotlin.math.abs

/* Problem: Given `num`, how to estimate `sqrt(num)`?
 * We'll see where "fixed point" kicks into the process. */

// Fix the precision we care about throughout the problem.
val TOL = 0.001
// Will be used later. This is randomly picked as our interest is not particularly in finding the best starting point.
val START_POINT = 1.0

// Start with some helper functions.
fun isClose(num1: Double, num2: Double): Boolean {
    return abs(num1 - num2) < TOL
}

fun average(num1: Double, num2: Double): Double {
    return (num1 + num2) / 2
}

// We will talk about this later.
fun getNextGuess(currGuess: Double, num: Double): Double {
    return average(currGuess, num / currGuess)
}

// Heron's method
fun sqrtHeron(num: Double): Double {
    fun isGoodGuess(currGuess: Double): Boolean {
        // We don't know the true answer, `sqrt(num)`, but we can check with `num`.
        return isClose(currGuess * currGuess, num)
    }

    fun improve(currGuess: Double): Double {
        if (isGoodGuess(currGuess)) {
            return currGuess
        }
        val nextGuess = getNextGuess(currGuess, num)
        return improve(nextGuess)
    }

    return improve(currGuess = START_POINT)
}

println(sqrtHeron(2.0)) // 1.4142156862745097

/* Why is it correct?
 * It's an iterative process of keep `improve`-ing `currGuess` until it `isGoodGuess`.
 * If `currGuess` is not close enough, we improve it with the magic `getNextGuess`.
 *
 * So why does Heron came up with `getNextGuess`?
 * The idea is if `currGuess` is smaller than `sqrt(num)`, then `num / currGuess` is bigger than `sqrt(num)`.
 * Hence we take the average between them, squeezing the range and converging to `sqrt(num)`.
 *
 * Moreover, we see `getNextGuess( sqrt(num) ) == sqrt(num)`.
 * Hence `sqrt` is a, so-called, fixed-point of `getNextGuess`.
 * We can abstract out the process of "finding fixed-point of a function". */
fun getFixedPoint(func: (Double) -> Double): Double {
    fun improve(currPoint: Double): Double {
        val nextPoint = func(currPoint)
        if (isClose(currPoint, nextPoint)) {
            return currPoint
        }
        return improve(nextPoint)
    }
    return improve(currPoint = START_POINT)
}

/* According to line 53, we can define `sqrt = getFixedPoint(getNextGuess)`. */
fun sqrtFromFixedPoint(num: Double): Double {
    return getFixedPoint({ currGuess: Double -> getNextGuess(currGuess, num) })
    // And equivalently, if you are familiar with Kotlin:
    return getFixedPoint { getNextGuess(it, num) }
}

println(sqrtFromFixedPoint(2.0)) // 1.4142156862745097
