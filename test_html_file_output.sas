/* Test: Write ODS HTML to file AND stdout to compare */

/* First, write to file */
ods listing close;
ods html5 file="/tmp/sas_html_test.html"
    options(bitmap_mode='inline') device=svg style=HTMLBlue;
ods graphics on / outputfmt=png;

PROC PRINT DATA=sashelp.class;
RUN;

ods html5 close;
ods listing;

/* Now let's also check what we get with stdout */
%put HTML file written to /tmp/sas_html_test.html;
