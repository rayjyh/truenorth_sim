#include <stdio.h>

void print_col(int* a){
    for (int i = 0; i < 3; ++i) {
        printf("%d\n", *(a+i));
    }
}

int main(){
    int a[2][3] = {{1,2,3},
                   {4,5,6}};
    for (int i = 0; i < 2; ++i) {
        int* b = &a[i][0];
        print_col(b);
    }
    return 0;
}
