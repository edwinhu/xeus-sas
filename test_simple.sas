DATA work.test;
    INPUT name $ age;
    DATALINES;
Alice 25
Bob 30
Charlie 35
;
RUN;

PROC PRINT DATA=work.test;
RUN;
