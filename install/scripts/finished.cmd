/* REXX script to create last installer dialog page. */

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

stext = dw_text_new(INSTALLER_APPLICATION || " " || INSTALLER_VERSION || " installation finished.", 0)

call dw_window_set_style stext, DW_DT_VCENTER, DW_DT_VCENTER

call dw_box_pack_start custombox, stext, 300, 15, TRUE, TRUE, 10

desctext = dw_text_new("Thanks for trying " || INSTALLER_APPLICATION || " " || INSTALLER_VERSION || "! Press finish to exit the installation program.",0)

call dw_window_set_style desctext, DW_DT_WORDBREAK, DW_DT_WORDBREAK

call dw_box_pack_start custombox, desctext, 300, 60, TRUE, TRUE, 10

groupbox = dw_groupbox_new(BOXVERT, 20, "Reboot?")

call dw_box_pack_start custombox, groupbox, 0, 0, TRUE, TRUE, 0

rnow = dw_radiobutton_new("Reboot Now", 0)

call dw_box_pack_start groupbox, rnow, 50, 25, TRUE, TRUE, 0

rlater = dw_radiobutton_new("Reboot Later", 0)

call dw_box_pack_start groupbox, rlater, 50, 25, TRUE, TRUE, 0

call inst_savevar "rnow", rnow
call inst_savevar "rlater", rlater

call dw_checkbox_set rnow, 1

call dw_box_pack_start custombox, 0, 300, 70, TRUE, TRUE, 10

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

nextbutton = dw_button_new("Finish", 1001)

call inst_setbutton nextbutton, "reboot.cmd"

call dw_box_pack_start buttonbox, nextbutton, 60, 30, TRUE, FALSE, 0

/* Set some nice fonts and colors */
if PLATFORM = "OS2" then do
   call dw_window_set_font stext, "9.WarpSans Bold"
end
if PLATFORM = "WIN32" then do
   call dw_window_set_font stext, "14.Arial Bold"
end

call inst_restore_pos mainwindow

call dw_window_show mainwindow

call inst_setwindow mainwindow 

exit

