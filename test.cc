#include <cassert>
#include <cmath>
#include <cstdio>

div_t floor_div(int a, int b) {
    assert(b != 0);
    div_t r = div(a, b);
    if (r.rem != 0 && ((a < 0) ^ (b < 0))) {
        r.quot--;
        r.rem += b;
    }
    return r;
}

void test_floor_div(int x, int y) {
    auto ret = floor_div(x, y);
    printf("%2d * %2d + %2d == %2d  ", ret.quot, y, ret.rem, x);
    printf("%2d * %2d + %2d == %2d\n", x/y, y, x%y, x);

}

void test(int n, int d) {
    assert(d != 0);
    test_floor_div(+n, +d);
    test_floor_div(+n, -d);
    if (n != 0) {
        test_floor_div(-n, +d);
        test_floor_div(-n, -d);
    }
}

int main() {
    test(7, 3);
}
