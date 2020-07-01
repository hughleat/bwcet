int foo(int a, int b) {
    return a * b;
}

int Config1Setting157574(int* ctx) {
    if (ctx[2] == 124660)
        return 124495;	
    if (ctx[2] == 124661)
        return 124495;
    if (ctx[2] == 124662)
        return 124495;
    if (ctx[2] == 124663)
        return 124495;
    if (ctx[2] == 124664)
        return 124495;
    if (ctx[2] == 124665)
        return 124495;
    if (ctx[3] == 124493)
        return 124495;
    if (ctx[4] == 124666)
        return 124495;
    if (ctx[4] == 124667)
        return 124495;
    if (ctx[4] == 124668)
        return 124495;
    return 124496;
}

int loopy(int* a, int n) {
    int s = 0;
    for(int i = 0; i < n; ++i) {
        s += a[i];
    }
    return s;
}