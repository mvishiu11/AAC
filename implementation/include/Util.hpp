#ifndef _H_UTIL_
#define _H_UTIL_

template<typename T>
T factorial(T first, T last) {
    T ret = 1;
    for (T i = first; i<=last; i++) {
        ret *= i;
    }
    return ret;
}

template<typename T>
T choose(T n, T k) {
    T k2 = n-k;
    return factorial(k2+1, n)/factorial(1, k);
}


template<typename T>
T count_assignments(T from, T to) {
    if (to<from) {
        return 0;
    }
    T diff = to-from;
    return factorial(diff+1, to);
}

#endif