/* REXX script to create last installer dialog page. */

rnow = inst_findvar("rnow")
rlater = inst_findvar("rlater")

rebootflag = dw_checkbox_get(rnow)

if arg(1) > 0 then
        call dw_window_destroy arg(1)

if rebootflag > 0 then
        call inst_reboot

call inst_exit

exit

