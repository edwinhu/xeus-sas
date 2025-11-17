/* Test 1: Create a dataset - should persist */
DATA work.mydata;
    INPUT x y;
    DATALINES;
1 10
2 20
3 30
;
RUN;
