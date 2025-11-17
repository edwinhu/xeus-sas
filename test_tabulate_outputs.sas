* Test PROC TABULATE with both HTML and plain text output for comparison;

* First, generate plain text output to see what it SHOULD look like;
ods listing file="/tmp/tabulate_listing.txt";

proc tabulate data=sashelp.class;
  class sex;
  var height;
  table sex, height*sum / caption='Average Height by Sex';
run;

ods listing close;

* Now generate HTML output;
ods listing close;
ods html5 file="/tmp/tabulate_html.html" style=Journal2;

proc tabulate data=sashelp.class;
  class sex;
  var height;
  table sex, height*sum / caption='Average Height by Sex';
run;

ods html5 close;
ods listing;

* More complex example with multiple dimensions;
ods listing file="/tmp/tabulate_complex_listing.txt";

proc tabulate data=sashelp.class;
  class sex;
  var height weight;
  table sex, (height weight)*sum*f=8.2 / caption='Height and Weight by Sex';
run;

ods listing close;

ods listing close;
ods html5 file="/tmp/tabulate_complex_html.html" style=Journal2;

proc tabulate data=sashelp.class;
  class sex;
  var height weight;
  table sex, (height weight)*sum*f=8.2 / caption='Height and Weight by Sex';
run;

ods html5 close;
ods listing;
