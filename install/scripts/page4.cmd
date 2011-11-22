/* REXX script to create fourth installer dialog page. */

if inst_packages() < 1 then
        exit

if arg(1) > 0 then do
        call inst_save_pos arg(1)
        call dw_window_destroy arg(1)
end

flStyle = inst_bitor(DW_FCF_SYSMENU, DW_FCF_TITLEBAR, DW_FCF_SHELLPOSITION, DW_FCF_TASKLIST, DW_FCF_DLGBORDER)

mainwindow = dw_window_new(HWND_DESKTOP, INSTALLER_TITLE, flStyle)

/* This number must corespond to a resource ID in the bound resources */
call dw_window_set_icon mainwindow, 2000

bigbox = dw_box_new(BOXVERT, 10)

call dw_box_pack_start mainwindow, bigbox, 0, 0, TRUE, TRUE, 0

mainbox = dw_box_new(BOXHORZ, 0)

call dw_box_pack_start bigbox, mainbox, 0, 0, TRUE, TRUE, 0

logo = dw_bitmap_new(1001)

/* This number must corespond to a resource ID in the bound resources */
call dw_window_set_bitmap logo, 2001, ""

call dw_box_pack_start mainbox, logo, 100, 275, FALSE, FALSE, 10

custombox = dw_box_new(BOXVERT, 0)

call dw_box_pack_start mainbox, custombox, 0, 0, TRUE, TRUE, 0

stext = dw_text_new("Please wait while installing " || INSTALLER_APPLICATION || " " || INSTALLER_VERSION || "...", 0)

call dw_window_set_style stext, DW_DT_VCENTER, DW_DT_VCENTER

call dw_box_pack_start custombox, stext, 300, 50, TRUE, TRUE, 10

groupbox = dw_groupbox_new(BOXHORZ, 10, "Progress")

call dw_box_pack_start custombox, groupbox, 0, 0, TRUE, FALSE, 10

slider = dw_percent_new(1010)

call dw_box_pack_start groupbox, slider, 300, 20, TRUE, FALSE, 10

status = dw_text_new("", 0)

call dw_box_pack_start custombox, status, 300, 50, TRUE, TRUE, 10

call dw_box_pack_start custombox, 0, 300, 100, TRUE, TRUE, 10

call inst_setslider slider, status

buttonbox = dw_box_new(BOXHORZ, 5)

call dw_box_pack_start bigbox, buttonbox, 0, 0, TRUE, FALSE, 0

exitbutton = dw_button_new("Exit Installer", 1003)

call inst_setbutton exitbutton, "exit"

call dw_box_pack_start buttonbox, exitbutton, 100, 30, TRUE, FALSE, 0

blanktext = dw_text_new("", 0)

call dw_box_pack_start buttonbox, blanktext, 150, 30, TRUE, FALSE, 0

backbutton = dw_button_new("<< Back", 1002)

call dw_window_disable backbutton

call dw_box_pack_start buttonbox, backbutton, 60, 30, TRUE, FALSE, 0

nextbutton = dw_button_new("Next >>", 1001)

call dw_window_disable nextbutton

call dw_box_pack_start buttonbox, nextbutton, 60, 30, TRUE, FALSE, 0

/* Set some nice fonts and colors */
if PLATFORM = "OS2" then do
   call dw_window_set_font stext, "9.WarpSans Bold"
end
if PLATFORM = "WIN32" then do
   call dw_window_set_font stext, "14.Arial Bold"
end

call inst_savevar "frompage", "4"

call inst_restore_pos mainwindow

call dw_window_show mainwindow

call inst_setwindow mainwindow

call inst_start "finished.cmd"

exit

