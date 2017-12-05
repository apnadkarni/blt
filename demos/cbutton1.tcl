
package require BLT
set x {
/usr/share/icons/oxygen/22x22/
/usr/share/icons/oxygen/22x22/devices
/usr/share/icons/oxygen/22x22/devices/scanner.png
/usr/share/icons/oxygen/22x22/devices/drive-removable-media-usb-pendrive.png
/usr/share/icons/oxygen/22x22/devices/media-optical-blu-ray.png
/usr/share/icons/oxygen/22x22/devices/drive-optical.png
/usr/share/icons/oxygen/22x22/devices/media-tape.png
/usr/share/icons/oxygen/22x22/devices/audio-input-line.png
/usr/share/icons/oxygen/22x22/devices/network-wireless.png
/usr/share/icons/oxygen/22x22/devices/input-mouse.png
/usr/share/icons/oxygen/22x22/devices/network-wired.png
/usr/share/icons/oxygen/22x22/devices/computer.png
/usr/share/icons/oxygen/22x22/devices/drive-removable-media.png
/usr/share/icons/oxygen/22x22/devices/video-television.png
/usr/share/icons/oxygen/22x22/devices/media-optical-dvd.png
/usr/share/icons/oxygen/22x22/devices/drive-harddisk.png
/usr/share/icons/oxygen/22x22/devices/media-flash-sd-mmc.png
/usr/share/icons/oxygen/22x22/devices/network-wireless-connected-50.png
/usr/share/icons/oxygen/22x22/devices/infrared-remote.png
/usr/share/icons/oxygen/22x22/devices/input-tablet.png
/usr/share/icons/oxygen/22x22/devices/computer-laptop.png
/usr/share/icons/oxygen/22x22/devices/battery.png
/usr/share/icons/oxygen/22x22/devices/audio-card.png
/usr/share/icons/oxygen/22x22/devices/video-display.png
/usr/share/icons/oxygen/22x22/devices/input-keyboard.png
/usr/share/icons/oxygen/22x22/devices/camera-web.png
/usr/share/icons/oxygen/22x22/devices/media-flash.png
/usr/share/icons/oxygen/22x22/devices/camera-photo.png
/usr/share/icons/oxygen/22x22/devices/network-wireless-connected-75.png
/usr/share/icons/oxygen/22x22/devices/media-optical-recordable.png
/usr/share/icons/oxygen/22x22/devices/input-gaming.png
/usr/share/icons/oxygen/22x22/devices/phone.png
/usr/share/icons/oxygen/22x22/devices/media-flash-smart-media.png
/usr/share/icons/oxygen/22x22/devices/audio-input-microphone.png
/usr/share/icons/oxygen/22x22/devices/media-flash-memory-stick.png
/usr/share/icons/oxygen/22x22/devices/modem.png
/usr/share/icons/oxygen/22x22/devices/media-optical-audio.png
/usr/share/icons/oxygen/22x22/devices/video-projector.png
/usr/share/icons/oxygen/22x22/devices/pda.png
/usr/share/icons/oxygen/22x22/devices/media-floppy.png
/usr/share/icons/oxygen/22x22/devices/media-optical.png
/usr/share/icons/oxygen/22x22/devices/network-wireless-disconnected.png
/usr/share/icons/oxygen/22x22/devices/media-optical-video.png
/usr/share/icons/oxygen/22x22/devices/multimedia-player.png
/usr/share/icons/oxygen/22x22/devices/printer.png
/usr/share/icons/oxygen/22x22/devices/multimedia-player-apple-ipod.png
/usr/share/icons/oxygen/22x22/devices/audio-headset.png
/usr/share/icons/oxygen/22x22/devices/cpu.png
/usr/share/icons/oxygen/22x22/devices/network-wireless-connected-25.png
/usr/share/icons/oxygen/22x22/devices/drive-removable-media-usb.png
/usr/share/icons/oxygen/22x22/devices/network-wireless-connected-00.png
/usr/share/icons/oxygen/22x22/devices/network-wireless-connected-100.png
/usr/share/icons/oxygen/22x22/devices/phone-openmoko-freerunner.png
/usr/share/icons/oxygen/22x22/actions
/usr/share/icons/oxygen/22x22/actions/edit-select-all.png
/usr/share/icons/oxygen/22x22/actions/irc-remove-operator.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-center.png
/usr/share/icons/oxygen/22x22/actions/arrow-up-double.png
/usr/share/icons/oxygen/22x22/actions/format-list-unordered.png
/usr/share/icons/oxygen/22x22/actions/fork.png
/usr/share/icons/oxygen/22x22/actions/resource-calendar-insert.png
/usr/share/icons/oxygen/22x22/actions/games-highscores.png
/usr/share/icons/oxygen/22x22/actions/mail-mark-unread.png
/usr/share/icons/oxygen/22x22/actions/bookmark-new-list.png
/usr/share/icons/oxygen/22x22/actions/layer-visible-on.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-list.png
/usr/share/icons/oxygen/22x22/actions/view-fullscreen.png
/usr/share/icons/oxygen/22x22/actions/go-previous.png
/usr/share/icons/oxygen/22x22/actions/go-jump-today.png
/usr/share/icons/oxygen/22x22/actions/im-ban-kick-user.png
/usr/share/icons/oxygen/22x22/actions/mixer-microphone.png
/usr/share/icons/oxygen/22x22/actions/edit-clear-locationbar-rtl.png
/usr/share/icons/oxygen/22x22/actions/meeting-attending-tentative.png
/usr/share/icons/oxygen/22x22/actions/transform-scale.png
/usr/share/icons/oxygen/22x22/actions/configure.png
/usr/share/icons/oxygen/22x22/actions/news-subscribe.png
/usr/share/icons/oxygen/22x22/actions/view-media-equalizer.png
/usr/share/icons/oxygen/22x22/actions/edit-find.png
/usr/share/icons/oxygen/22x22/actions/color-picker-white.png
/usr/share/icons/oxygen/22x22/actions/view-media-artist.png
/usr/share/icons/oxygen/22x22/actions/draw-polygon.png
/usr/share/icons/oxygen/22x22/actions/draw-triangle2.png
/usr/share/icons/oxygen/22x22/actions/vcs_add.png
/usr/share/icons/oxygen/22x22/actions/edit-find-replace.png
/usr/share/icons/oxygen/22x22/actions/edit-clear-locationbar-ltr.png
/usr/share/icons/oxygen/22x22/actions/debug-step-into.png
/usr/share/icons/oxygen/22x22/actions/view-process-users.png
/usr/share/icons/oxygen/22x22/actions/format-text-direction-rtl.png
/usr/share/icons/oxygen/22x22/actions/mixer-pc-speaker.png
/usr/share/icons/oxygen/22x22/actions/document-open-folder.png
/usr/share/icons/oxygen/22x22/actions/page-3sides.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-birthday.png
/usr/share/icons/oxygen/22x22/actions/mixer-capture-secondary.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule-baselined.png
/usr/share/icons/oxygen/22x22/actions/format-justify-center.png
/usr/share/icons/oxygen/22x22/actions/mail-receive.png
/usr/share/icons/oxygen/22x22/actions/arrow-down.png
/usr/share/icons/oxygen/22x22/actions/mail-signature-unknown.png
/usr/share/icons/oxygen/22x22/actions/go-next.png
/usr/share/icons/oxygen/22x22/actions/mail-tagged.png
/usr/share/icons/oxygen/22x22/actions/services.png
/usr/share/icons/oxygen/22x22/actions/document-import.png
/usr/share/icons/oxygen/22x22/actions/list-add.png
/usr/share/icons/oxygen/22x22/actions/bookmark-toolbar.png
/usr/share/icons/oxygen/22x22/actions/imagegallery.png
/usr/share/icons/oxygen/22x22/actions/go-last-view-page.png
/usr/share/icons/oxygen/22x22/actions/view-task-child.png
/usr/share/icons/oxygen/22x22/actions/document-open-recent.png
/usr/share/icons/oxygen/22x22/actions/align-vertical-center.png
/usr/share/icons/oxygen/22x22/actions/transform-shear-up.png
/usr/share/icons/oxygen/22x22/actions/mixer-master.png
/usr/share/icons/oxygen/22x22/actions/system-reboot.png
/usr/share/icons/oxygen/22x22/actions/games-solve.png
/usr/share/icons/oxygen/22x22/actions/draw-text.png
/usr/share/icons/oxygen/22x22/actions/document-close.png
/usr/share/icons/oxygen/22x22/actions/text-field.png
/usr/share/icons/oxygen/22x22/actions/edit-bomb.png
/usr/share/icons/oxygen/22x22/actions/draw-triangle4.png
/usr/share/icons/oxygen/22x22/actions/document-preview.png
/usr/share/icons/oxygen/22x22/actions/vcs_update.png
/usr/share/icons/oxygen/22x22/actions/arrow-up.png
/usr/share/icons/oxygen/22x22/actions/user-group-delete.png
/usr/share/icons/oxygen/22x22/actions/view-table-of-contents-rtl.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule-baselined-remove.png
/usr/share/icons/oxygen/22x22/actions/draw-path.png
/usr/share/icons/oxygen/22x22/actions/media-skip-forward.png
/usr/share/icons/oxygen/22x22/actions/mixer-digital.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-tasks.png
/usr/share/icons/oxygen/22x22/actions/format-indent-more.png
/usr/share/icons/oxygen/22x22/actions/download-later.png
/usr/share/icons/oxygen/22x22/actions/edit-find-user.png
/usr/share/icons/oxygen/22x22/actions/transform-shear-down.png
/usr/share/icons/oxygen/22x22/actions/align-horizontal-left-out.png
/usr/share/icons/oxygen/22x22/actions/format-text-underline.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_collaboration.png
/usr/share/icons/oxygen/22x22/actions/trash-empty.png
/usr/share/icons/oxygen/22x22/actions/player-time.png
/usr/share/icons/oxygen/22x22/actions/draw-polyline.png
/usr/share/icons/oxygen/22x22/actions/draw-freehand.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-whatsnext.png
/usr/share/icons/oxygen/22x22/actions/stroke-cap-butt.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_class.png
/usr/share/icons/oxygen/22x22/actions/dashboard-show.png
/usr/share/icons/oxygen/22x22/actions/go-down.png
/usr/share/icons/oxygen/22x22/actions/resource-group-new.png
/usr/share/icons/oxygen/22x22/actions/process-stop.png
/usr/share/icons/oxygen/22x22/actions/system-run.png
/usr/share/icons/oxygen/22x22/actions/draw-star.png
/usr/share/icons/oxygen/22x22/actions/mail-signed-verified.png
/usr/share/icons/oxygen/22x22/actions/show-menu.png
/usr/share/icons/oxygen/22x22/actions/text-speak.png
/usr/share/icons/oxygen/22x22/actions/edit-rename.png
/usr/share/icons/oxygen/22x22/actions/office-chart-line-percentage.png
/usr/share/icons/oxygen/22x22/actions/page-simple.png
/usr/share/icons/oxygen/22x22/actions/view-pim-tasks.png
/usr/share/icons/oxygen/22x22/actions/office-chart-polar.png
/usr/share/icons/oxygen/22x22/actions/mail-signed.png
/usr/share/icons/oxygen/22x22/actions/draw-arrow-down.png
/usr/share/icons/oxygen/22x22/actions/draw-halfcircle1.png
/usr/share/icons/oxygen/22x22/actions/project-development.png
/usr/share/icons/oxygen/22x22/actions/chronometer.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-right.png
/usr/share/icons/oxygen/22x22/actions/view-split-top-bottom.png
/usr/share/icons/oxygen/22x22/actions/format-connect-node.png
/usr/share/icons/oxygen/22x22/actions/quickopen.png
/usr/share/icons/oxygen/22x22/actions/media-skip-backward.png
/usr/share/icons/oxygen/22x22/actions/office-chart-scatter.png
/usr/share/icons/oxygen/22x22/actions/draw-triangle.png
/usr/share/icons/oxygen/22x22/actions/tools-media-optical-format.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-journal.png
/usr/share/icons/oxygen/22x22/actions/view-statistics.png
/usr/share/icons/oxygen/22x22/actions/configure-shortcuts.png
/usr/share/icons/oxygen/22x22/actions/network-disconnect.png
/usr/share/icons/oxygen/22x22/actions/im-kick-user.png
/usr/share/icons/oxygen/22x22/actions/view-list-details.png
/usr/share/icons/oxygen/22x22/actions/view-pim-mail.png
/usr/share/icons/oxygen/22x22/actions/office-chart-ring.png
/usr/share/icons/oxygen/22x22/actions/go-top.png
/usr/share/icons/oxygen/22x22/actions/mail-reply-custom-all.png
/usr/share/icons/oxygen/22x22/actions/office-chart-area-percentage.png
/usr/share/icons/oxygen/22x22/actions/resource-group.png
/usr/share/icons/oxygen/22x22/actions/debug-execute-from-cursor.png
/usr/share/icons/oxygen/22x22/actions/network-connect.png
/usr/share/icons/oxygen/22x22/actions/irkickoff.png
/usr/share/icons/oxygen/22x22/actions/measure.png
/usr/share/icons/oxygen/22x22/actions/run-build-install.png
/usr/share/icons/oxygen/22x22/actions/transform-crop.png
/usr/share/icons/oxygen/22x22/actions/format-indent-less.png
/usr/share/icons/oxygen/22x22/actions/view-list-text.png
/usr/share/icons/oxygen/22x22/actions/format-line-spacing-normal.png
/usr/share/icons/oxygen/22x22/actions/mixer-midi.png
/usr/share/icons/oxygen/22x22/actions/view-pim-tasks-pending.png
/usr/share/icons/oxygen/22x22/actions/mixer-ac97.png
/usr/share/icons/oxygen/22x22/actions/media-playback-stop.png
/usr/share/icons/oxygen/22x22/actions/document-save-as.png
/usr/share/icons/oxygen/22x22/actions/view-presentation.png
/usr/share/icons/oxygen/22x22/actions/view-split-left-right.png
/usr/share/icons/oxygen/22x22/actions/im-status-message-edit.png
/usr/share/icons/oxygen/22x22/actions/voicecall.png
/usr/share/icons/oxygen/22x22/actions/view-object-histogram-logarithmic.png
/usr/share/icons/oxygen/22x22/actions/office-chart-bar.png
/usr/share/icons/oxygen/22x22/actions/dialog-ok.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-special-occasion.png
/usr/share/icons/oxygen/22x22/actions/run-build-install-root.png
/usr/share/icons/oxygen/22x22/actions/draw-bezier-curves.png
/usr/share/icons/oxygen/22x22/actions/archive-insert-directory.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-holiday.png
/usr/share/icons/oxygen/22x22/actions/edit-undo.png
/usr/share/icons/oxygen/22x22/actions/zoom-out.png
/usr/share/icons/oxygen/22x22/actions/svn-commit.png
/usr/share/icons/oxygen/22x22/actions/align-horizontal-left.png
/usr/share/icons/oxygen/22x22/actions/color-picker-black.png
/usr/share/icons/oxygen/22x22/actions/text-frame-link.png
/usr/share/icons/oxygen/22x22/actions/format-font-size-less.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_sequence.png
/usr/share/icons/oxygen/22x22/actions/list-remove.png
/usr/share/icons/oxygen/22x22/actions/run-build-prune.png
/usr/share/icons/oxygen/22x22/actions/im-user-busy.png
/usr/share/icons/oxygen/22x22/actions/debug-step-over.png
/usr/share/icons/oxygen/22x22/actions/code-typedef.png
/usr/share/icons/oxygen/22x22/actions/view-media-visualization.png
/usr/share/icons/oxygen/22x22/actions/view-process-own.png
/usr/share/icons/oxygen/22x22/actions/webcamreceive.png
/usr/share/icons/oxygen/22x22/actions/layer-visible-off.png
/usr/share/icons/oxygen/22x22/actions/svn_branch.png
/usr/share/icons/oxygen/22x22/actions/application-exit.png
/usr/share/icons/oxygen/22x22/actions/go-first.png
/usr/share/icons/oxygen/22x22/actions/edit-find-project.png
/usr/share/icons/oxygen/22x22/actions/games-difficult.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-day.png
/usr/share/icons/oxygen/22x22/actions/project-development-new-template.png
/usr/share/icons/oxygen/22x22/actions/office-chart-pie.png
/usr/share/icons/oxygen/22x22/actions/go-last.png
/usr/share/icons/oxygen/22x22/actions/view-filter.png
/usr/share/icons/oxygen/22x22/actions/format-justify-left.png
/usr/share/icons/oxygen/22x22/actions/system-suspend-hibernate.png
/usr/share/icons/oxygen/22x22/actions/debug-execute-to-cursor.png
/usr/share/icons/oxygen/22x22/actions/format-text-superscript.png
/usr/share/icons/oxygen/22x22/actions/media-playback-pause.png
/usr/share/icons/oxygen/22x22/actions/document-print.png
/usr/share/icons/oxygen/22x22/actions/project-development-close-all.png
/usr/share/icons/oxygen/22x22/actions/view-right-close.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule.png
/usr/share/icons/oxygen/22x22/actions/irkickflash.png
/usr/share/icons/oxygen/22x22/actions/format-text-bold.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-margin.png
/usr/share/icons/oxygen/22x22/actions/document-open.png
/usr/share/icons/oxygen/22x22/actions/align-vertical-top.png
/usr/share/icons/oxygen/22x22/actions/get-hot-new-stuff.png
/usr/share/icons/oxygen/22x22/actions/draw-rectangle.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-timeline.png
/usr/share/icons/oxygen/22x22/actions/document-revert.png
/usr/share/icons/oxygen/22x22/actions/webcamsend.png
/usr/share/icons/oxygen/22x22/actions/htmlvalidator.png
/usr/share/icons/oxygen/22x22/actions/view-web-browser-dom-tree.png
/usr/share/icons/oxygen/22x22/actions/go-down-search.png
/usr/share/icons/oxygen/22x22/actions/format-stroke-color.png
/usr/share/icons/oxygen/22x22/actions/office-chart-area.png
/usr/share/icons/oxygen/22x22/actions/debug-step-out.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule-child-insert.png
/usr/share/icons/oxygen/22x22/actions/insert-link.png
/usr/share/icons/oxygen/22x22/actions/view-object-histogram-linear.png
/usr/share/icons/oxygen/22x22/actions/mixer-headset.png
/usr/share/icons/oxygen/22x22/actions/zoom-fit-best.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_deployment.png
/usr/share/icons/oxygen/22x22/actions/vcs_diff.png
/usr/share/icons/oxygen/22x22/actions/list-add-font.png
/usr/share/icons/oxygen/22x22/actions/flag-green.png
/usr/share/icons/oxygen/22x22/actions/format-list-ordered.png
/usr/share/icons/oxygen/22x22/actions/webarchiver.png
/usr/share/icons/oxygen/22x22/actions/format-remove-node.png
/usr/share/icons/oxygen/22x22/actions/im-user-offline.png
/usr/share/icons/oxygen/22x22/actions/mixer-cd.png
/usr/share/icons/oxygen/22x22/actions/mixer-microphone-boost.png
/usr/share/icons/oxygen/22x22/actions/tools-media-optical-erase.png
/usr/share/icons/oxygen/22x22/actions/draw-triangle3.png
/usr/share/icons/oxygen/22x22/actions/align-horizontal-center.png
/usr/share/icons/oxygen/22x22/actions/roll.png
/usr/share/icons/oxygen/22x22/actions/mail-replied.png
/usr/share/icons/oxygen/22x22/actions/arrow-right-double.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-left.png
/usr/share/icons/oxygen/22x22/actions/document-new.png
/usr/share/icons/oxygen/22x22/actions/view-process-all.png
/usr/share/icons/oxygen/22x22/actions/mixer-front.png
/usr/share/icons/oxygen/22x22/actions/flag-black.png
/usr/share/icons/oxygen/22x22/actions/mail-mark-read.png
/usr/share/icons/oxygen/22x22/actions/milestone.png
/usr/share/icons/oxygen/22x22/actions/system-switch-user.png
/usr/share/icons/oxygen/22x22/actions/draw-arrow-up.png
/usr/share/icons/oxygen/22x22/actions/view-pim-calendar.png
/usr/share/icons/oxygen/22x22/actions/vcs_status.png
/usr/share/icons/oxygen/22x22/actions/project-open.png
/usr/share/icons/oxygen/22x22/actions/news-unsubscribe.png
/usr/share/icons/oxygen/22x22/actions/system-shutdown.png
/usr/share/icons/oxygen/22x22/actions/page-4sides.png
/usr/share/icons/oxygen/22x22/actions/tab-close-other.png
/usr/share/icons/oxygen/22x22/actions/view-task.png
/usr/share/icons/oxygen/22x22/actions/mail-mark-notjunk.png
/usr/share/icons/oxygen/22x22/actions/feed-subscribe.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-wedding-anniversary.png
/usr/share/icons/oxygen/22x22/actions/tools-report-bug.png
/usr/share/icons/oxygen/22x22/actions/tab-duplicate.png
/usr/share/icons/oxygen/22x22/actions/document-save.png
/usr/share/icons/oxygen/22x22/actions/office-chart-line.png
/usr/share/icons/oxygen/22x22/actions/office-chart-line-stacked.png
/usr/share/icons/oxygen/22x22/actions/edit-redo.png
/usr/share/icons/oxygen/22x22/actions/page-2sides.png
/usr/share/icons/oxygen/22x22/actions/mail-message-new.png
/usr/share/icons/oxygen/22x22/actions/transform-crop-and-resize.png
/usr/share/icons/oxygen/22x22/actions/games-endturn.png
/usr/share/icons/oxygen/22x22/actions/format-text-subscript.png
/usr/share/icons/oxygen/22x22/actions/align-vertical-bottom-out.png
/usr/share/icons/oxygen/22x22/actions/view-pim-news.png
/usr/share/icons/oxygen/22x22/actions/flag-yellow.png
/usr/share/icons/oxygen/22x22/actions/character-set.png
/usr/share/icons/oxygen/22x22/actions/mixer-pcm-default.png
/usr/share/icons/oxygen/22x22/actions/go-previous-view.png
/usr/share/icons/oxygen/22x22/actions/flag.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_component.png
/usr/share/icons/oxygen/22x22/actions/view-process-all-tree.png
/usr/share/icons/oxygen/22x22/actions/flag-blue.png
/usr/share/icons/oxygen/22x22/actions/view-table-of-contents-ltr.png
/usr/share/icons/oxygen/22x22/actions/transform-move.png
/usr/share/icons/oxygen/22x22/actions/run-build-configure.png
/usr/share/icons/oxygen/22x22/actions/mail-forwarded.png
/usr/share/icons/oxygen/22x22/actions/stroke-cap-round.png
/usr/share/icons/oxygen/22x22/actions/go-first-view-page.png
/usr/share/icons/oxygen/22x22/actions/draw-spiral.png
/usr/share/icons/oxygen/22x22/actions/view-pim-summary.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-time-spent.png
/usr/share/icons/oxygen/22x22/actions/mail-encrypted-part.png
/usr/share/icons/oxygen/22x22/actions/user-properties.png
/usr/share/icons/oxygen/22x22/actions/tab-close.png
/usr/share/icons/oxygen/22x22/actions/edit-cut.png
/usr/share/icons/oxygen/22x22/actions/view-media-lyrics.png
/usr/share/icons/oxygen/22x22/actions/archive-insert.png
/usr/share/icons/oxygen/22x22/actions/svn_merge.png
/usr/share/icons/oxygen/22x22/actions/zoom-original.png
/usr/share/icons/oxygen/22x22/actions/svn-update.png
/usr/share/icons/oxygen/22x22/actions/media-seek-forward.png
/usr/share/icons/oxygen/22x22/actions/view-sidetree.png
/usr/share/icons/oxygen/22x22/actions/irc-close-channel.png
/usr/share/icons/oxygen/22x22/actions/transform-shear-left.png
/usr/share/icons/oxygen/22x22/actions/im-user.png
/usr/share/icons/oxygen/22x22/actions/mail-signed-full.png
/usr/share/icons/oxygen/22x22/actions/configure-toolbars.png
/usr/share/icons/oxygen/22x22/actions/document-edit.png
/usr/share/icons/oxygen/22x22/actions/media-playback-start.png
/usr/share/icons/oxygen/22x22/actions/mail-mark-task.png
/usr/share/icons/oxygen/22x22/actions/mail-forward.png
/usr/share/icons/oxygen/22x22/actions/arrow-down-double.png
/usr/share/icons/oxygen/22x22/actions/dialog-cancel.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule-edit.png
/usr/share/icons/oxygen/22x22/actions/mail-queue.png
/usr/share/icons/oxygen/22x22/actions/view-restore.png
/usr/share/icons/oxygen/22x22/actions/edit-delete-shred.png
/usr/share/icons/oxygen/22x22/actions/mail-reply-all.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_state.png
/usr/share/icons/oxygen/22x22/actions/view-choose.png
/usr/share/icons/oxygen/22x22/actions/vcs_remove.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-x.png
/usr/share/icons/oxygen/22x22/actions/quickopen-function.png
/usr/share/icons/oxygen/22x22/actions/mixer-pcm.png
/usr/share/icons/oxygen/22x22/actions/media-record.png
/usr/share/icons/oxygen/22x22/actions/lastmoves.png
/usr/share/icons/oxygen/22x22/actions/speaker.png
/usr/share/icons/oxygen/22x22/actions/debug-step-instruction.png
/usr/share/icons/oxygen/22x22/actions/quickopen-file.png
/usr/share/icons/oxygen/22x22/actions/document-sign.png
/usr/share/icons/oxygen/22x22/actions/mixer-video.png
/usr/share/icons/oxygen/22x22/actions/zoom-in.png
/usr/share/icons/oxygen/22x22/actions/irc-operator.png
/usr/share/icons/oxygen/22x22/actions/appointment-new.png
/usr/share/icons/oxygen/22x22/actions/transform-rotate.png
/usr/share/icons/oxygen/22x22/actions/story-editor.png
/usr/share/icons/oxygen/22x22/actions/format-font-size-more.png
/usr/share/icons/oxygen/22x22/actions/help-contextual.png
/usr/share/icons/oxygen/22x22/actions/draw-arrow-back.png
/usr/share/icons/oxygen/22x22/actions/help-hint.png
/usr/share/icons/oxygen/22x22/actions/view-pim-journal.png
/usr/share/icons/oxygen/22x22/actions/view-list-tree.png
/usr/share/icons/oxygen/22x22/actions/office-chart-area-stacked.png
/usr/share/icons/oxygen/22x22/actions/system-search.png
/usr/share/icons/oxygen/22x22/actions/system-lock-screen.png
/usr/share/icons/oxygen/22x22/actions/go-previous-use.png
/usr/share/icons/oxygen/22x22/actions/bookmarks.png
/usr/share/icons/oxygen/22x22/actions/format-text-direction-ltr.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-month.png
/usr/share/icons/oxygen/22x22/actions/align-horizontal-right-out.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-page.png
/usr/share/icons/oxygen/22x22/actions/go-next-context.png
/usr/share/icons/oxygen/22x22/actions/go-up-search.png
/usr/share/icons/oxygen/22x22/actions/tools-media-optical-burn.png
/usr/share/icons/oxygen/22x22/actions/draw-brush.png
/usr/share/icons/oxygen/22x22/actions/edit-clear.png
/usr/share/icons/oxygen/22x22/actions/player-volume.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-page.png
/usr/share/icons/oxygen/22x22/actions/window-duplicate.png
/usr/share/icons/oxygen/22x22/actions/object-flip-horizontal.png
/usr/share/icons/oxygen/22x22/actions/mixer-microphone-front.png
/usr/share/icons/oxygen/22x22/actions/view-history.png
/usr/share/icons/oxygen/22x22/actions/color-picker.png
/usr/share/icons/oxygen/22x22/actions/tab-detach.png
/usr/share/icons/oxygen/22x22/actions/list-add-user.png
/usr/share/icons/oxygen/22x22/actions/mixer-capture.png
/usr/share/icons/oxygen/22x22/actions/insert-text.png
/usr/share/icons/oxygen/22x22/actions/mail-flag.png
/usr/share/icons/oxygen/22x22/actions/mixer-microphone-front-boost.png
/usr/share/icons/oxygen/22x22/actions/mail-send.png
/usr/share/icons/oxygen/22x22/actions/mail-mark-important.png
/usr/share/icons/oxygen/22x22/actions/games-config-tiles.png
/usr/share/icons/oxygen/22x22/actions/format-line-spacing-double.png
/usr/share/icons/oxygen/22x22/actions/document-print-direct.png
/usr/share/icons/oxygen/22x22/actions/mixer-line.png
/usr/share/icons/oxygen/22x22/actions/acrobat.png
/usr/share/icons/oxygen/22x22/actions/go-up.png
/usr/share/icons/oxygen/22x22/actions/bookmarks-organize.png
/usr/share/icons/oxygen/22x22/actions/project-development-close.png
/usr/share/icons/oxygen/22x22/actions/format-justify-right.png
/usr/share/icons/oxygen/22x22/actions/games-config-background.png
/usr/share/icons/oxygen/22x22/actions/im-invisible-user.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-workweek.png
/usr/share/icons/oxygen/22x22/actions/view-preview.png
/usr/share/icons/oxygen/22x22/actions/document-preview-archive.png
/usr/share/icons/oxygen/22x22/actions/view-calendar.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-upcoming-days.png
/usr/share/icons/oxygen/22x22/actions/rating.png
/usr/share/icons/oxygen/22x22/actions/format-fill-color.png
/usr/share/icons/oxygen/22x22/actions/help-contents.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_activity.png
/usr/share/icons/oxygen/22x22/actions/edit-select.png
/usr/share/icons/oxygen/22x22/actions/view-media-playlist.png
/usr/share/icons/oxygen/22x22/actions/draw-ellipse.png
/usr/share/icons/oxygen/22x22/actions/vcs_commit.png
/usr/share/icons/oxygen/22x22/actions/mail-mark-unread-new.png
/usr/share/icons/oxygen/22x22/actions/media-eject.png
/usr/share/icons/oxygen/22x22/actions/go-previous-context.png
/usr/share/icons/oxygen/22x22/actions/fill-color.png
/usr/share/icons/oxygen/22x22/actions/format-justify-fill.png
/usr/share/icons/oxygen/22x22/actions/view-close.png
/usr/share/icons/oxygen/22x22/actions/code-function.png
/usr/share/icons/oxygen/22x22/actions/help-about.png
/usr/share/icons/oxygen/22x22/actions/object-flip-vertical.png
/usr/share/icons/oxygen/22x22/actions/edit-node.png
/usr/share/icons/oxygen/22x22/actions/draw-square-inverted-corners.png
/usr/share/icons/oxygen/22x22/actions/transform-shear-right.png
/usr/share/icons/oxygen/22x22/actions/object-rotate-right.png
/usr/share/icons/oxygen/22x22/actions/select-rectangular.png
/usr/share/icons/oxygen/22x22/actions/go-jump-declaration.png
/usr/share/icons/oxygen/22x22/actions/draw-triangle1.png
/usr/share/icons/oxygen/22x22/actions/insert-horizontal-rule.png
/usr/share/icons/oxygen/22x22/actions/games-config-theme.png
/usr/share/icons/oxygen/22x22/actions/view-user-offline-kopete.png
/usr/share/icons/oxygen/22x22/actions/window-close.png
/usr/share/icons/oxygen/22x22/actions/dialog-ok-apply.png
/usr/share/icons/oxygen/22x22/actions/view-sort-descending.png
/usr/share/icons/oxygen/22x22/actions/go-home.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-equal.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-equal.png
/usr/share/icons/oxygen/22x22/actions/format-text-color.png
/usr/share/icons/oxygen/22x22/actions/object-rotate-left.png
/usr/share/icons/oxygen/22x22/actions/address-book-new.png
/usr/share/icons/oxygen/22x22/actions/cssvalidator.png
/usr/share/icons/oxygen/22x22/actions/resource-calendar-child-insert.png
/usr/share/icons/oxygen/22x22/actions/debug-step-into-instruction.png
/usr/share/icons/oxygen/22x22/actions/tool-animator.png
/usr/share/icons/oxygen/22x22/actions/draw-halfcircle4.png
/usr/share/icons/oxygen/22x22/actions/draw-circle.png
/usr/share/icons/oxygen/22x22/actions/view-task-add.png
/usr/share/icons/oxygen/22x22/actions/user-group-properties.png
/usr/share/icons/oxygen/22x22/actions/go-first-view.png
/usr/share/icons/oxygen/22x22/actions/document-decrypt.png
/usr/share/icons/oxygen/22x22/actions/mail-signed-part.png
/usr/share/icons/oxygen/22x22/actions/svn_remove.png
/usr/share/icons/oxygen/22x22/actions/code-context.png
/usr/share/icons/oxygen/22x22/actions/office-chart-bar-percentage.png
/usr/share/icons/oxygen/22x22/actions/format-text-italic.png
/usr/share/icons/oxygen/22x22/actions/document-save-all.png
/usr/share/icons/oxygen/22x22/actions/bookmark-new.png
/usr/share/icons/oxygen/22x22/actions/go-jump-locationbar.png
/usr/share/icons/oxygen/22x22/actions/edit-copy.png
/usr/share/icons/oxygen/22x22/actions/arrow-left-double.png
/usr/share/icons/oxygen/22x22/actions/code-variable.png
/usr/share/icons/oxygen/22x22/actions/edit-find-mail.png
/usr/share/icons/oxygen/22x22/actions/quickopen-class.png
/usr/share/icons/oxygen/22x22/actions/document-open-remote.png
/usr/share/icons/oxygen/22x22/actions/document-properties.png
/usr/share/icons/oxygen/22x22/actions/edit-guides.png
/usr/share/icons/oxygen/22x22/actions/stroke-cap-miter.png
/usr/share/icons/oxygen/22x22/actions/mail-reply-sender.png
/usr/share/icons/oxygen/22x22/actions/arrow-right.png
/usr/share/icons/oxygen/22x22/actions/document-print-preview.png
/usr/share/icons/oxygen/22x22/actions/run-build.png
/usr/share/icons/oxygen/22x22/actions/go-next-view.png
/usr/share/icons/oxygen/22x22/actions/distribute-horizontal-margin.png
/usr/share/icons/oxygen/22x22/actions/media-seek-backward.png
/usr/share/icons/oxygen/22x22/actions/irc-unvoice.png
/usr/share/icons/oxygen/22x22/actions/format-join-node.png
/usr/share/icons/oxygen/22x22/actions/stroke-cap-square.png
/usr/share/icons/oxygen/22x22/actions/system-suspend.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule-calculus.png
/usr/share/icons/oxygen/22x22/actions/im-ban-user.png
/usr/share/icons/oxygen/22x22/actions/insert-table.png
/usr/share/icons/oxygen/22x22/actions/archive-extract.png
/usr/share/icons/oxygen/22x22/actions/tools-wizard.png
/usr/share/icons/oxygen/22x22/actions/align-horizontal-right.png
/usr/share/icons/oxygen/22x22/actions/mixer-surround-center.png
/usr/share/icons/oxygen/22x22/actions/dialog-close.png
/usr/share/icons/oxygen/22x22/actions/stroke-cap-bevel.png
/usr/share/icons/oxygen/22x22/actions/im-user-away.png
/usr/share/icons/oxygen/22x22/actions/page-zoom.png
/usr/share/icons/oxygen/22x22/actions/folder-sync.png
/usr/share/icons/oxygen/22x22/actions/babelfish.png
/usr/share/icons/oxygen/22x22/actions/legalmoves.png
/usr/share/icons/oxygen/22x22/actions/mail-reply-list.png
/usr/share/icons/oxygen/22x22/actions/arrow-left.png
/usr/share/icons/oxygen/22x22/actions/svn_status.png
/usr/share/icons/oxygen/22x22/actions/edit-clear-list.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-agenda.png
/usr/share/icons/oxygen/22x22/actions/svn_switch.png
/usr/share/icons/oxygen/22x22/actions/games-config-options.png
/usr/share/icons/oxygen/22x22/actions/view-refresh.png
/usr/share/icons/oxygen/22x22/actions/preflight-verifier.png
/usr/share/icons/oxygen/22x22/actions/mail-encrypted.png
/usr/share/icons/oxygen/22x22/actions/zoom-fit-height.png
/usr/share/icons/oxygen/22x22/actions/edit-paste.png
/usr/share/icons/oxygen/22x22/actions/edit-clear-history.png
/usr/share/icons/oxygen/22x22/actions/irc-join-channel.png
/usr/share/icons/oxygen/22x22/actions/tools-check-spelling.png
/usr/share/icons/oxygen/22x22/actions/documentation.png
/usr/share/icons/oxygen/22x22/actions/view-pim-contacts.png
/usr/share/icons/oxygen/22x22/actions/svn_add.png
/usr/share/icons/oxygen/22x22/actions/view-left-close.png
/usr/share/icons/oxygen/22x22/actions/tab-new-background.png
/usr/share/icons/oxygen/22x22/actions/account_offline_overlay.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_usecase.png
/usr/share/icons/oxygen/22x22/actions/insert-image.png
/usr/share/icons/oxygen/22x22/actions/document-encrypt.png
/usr/share/icons/oxygen/22x22/actions/folder-new.png
/usr/share/icons/oxygen/22x22/actions/mail-encrypted-full.png
/usr/share/icons/oxygen/22x22/actions/zoom-fit-width.png
/usr/share/icons/oxygen/22x22/actions/download.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-y.png
/usr/share/icons/oxygen/22x22/actions/meeting-attending.png
/usr/share/icons/oxygen/22x22/actions/edit-delete.png
/usr/share/icons/oxygen/22x22/actions/umbrello_diagram_entityrelationship.png
/usr/share/icons/oxygen/22x22/actions/draw-donut.png
/usr/share/icons/oxygen/22x22/actions/office-chart-bar-stacked.png
/usr/share/icons/oxygen/22x22/actions/user-group-new.png
/usr/share/icons/oxygen/22x22/actions/tab-new.png
/usr/share/icons/oxygen/22x22/actions/mail-reply-custom.png
/usr/share/icons/oxygen/22x22/actions/format-add-node.png
/usr/share/icons/oxygen/22x22/actions/code-class.png
/usr/share/icons/oxygen/22x22/actions/view-list-icons.png
/usr/share/icons/oxygen/22x22/actions/window-new.png
/usr/share/icons/oxygen/22x22/actions/draw-arrow-forward.png
/usr/share/icons/oxygen/22x22/actions/go-previous-view-page.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-top.png
/usr/share/icons/oxygen/22x22/actions/run-build-file.png
/usr/share/icons/oxygen/22x22/actions/archive-remove.png
/usr/share/icons/oxygen/22x22/actions/go-jump-definition.png
/usr/share/icons/oxygen/22x22/actions/stroke-join-round.png
/usr/share/icons/oxygen/22x22/actions/view-process-system.png
/usr/share/icons/oxygen/22x22/actions/draw-cross.png
/usr/share/icons/oxygen/22x22/actions/align-vertical-bottom.png
/usr/share/icons/oxygen/22x22/actions/view-right-new.png
/usr/share/icons/oxygen/22x22/actions/games-config-board.png
/usr/share/icons/oxygen/22x22/actions/draw-halfcircle2.png
/usr/share/icons/oxygen/22x22/actions/view-sort-ascending.png
/usr/share/icons/oxygen/22x22/actions/documentinfo.png
/usr/share/icons/oxygen/22x22/actions/format-line-spacing-triple.png
/usr/share/icons/oxygen/22x22/actions/go-jump.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-bottom.png
/usr/share/icons/oxygen/22x22/actions/document-export.png
/usr/share/icons/oxygen/22x22/actions/contact-new.png
/usr/share/icons/oxygen/22x22/actions/player-volume-muted.png
/usr/share/icons/oxygen/22x22/actions/mixer-surround.png
/usr/share/icons/oxygen/22x22/actions/tools-media-optical-copy.png
/usr/share/icons/oxygen/22x22/actions/go-next-view-page.png
/usr/share/icons/oxygen/22x22/actions/code-block.png
/usr/share/icons/oxygen/22x22/actions/go-next-use.png
/usr/share/icons/oxygen/22x22/actions/run-build-clean.png
/usr/share/icons/oxygen/22x22/actions/list-remove-user.png
/usr/share/icons/oxygen/22x22/actions/office-chart-polar-stacked.png
/usr/share/icons/oxygen/22x22/actions/debug-run-cursor.png
/usr/share/icons/oxygen/22x22/actions/mixer-lfe.png
/usr/share/icons/oxygen/22x22/actions/draw-eraser.png
/usr/share/icons/oxygen/22x22/actions/edit-text-frame-update.png
/usr/share/icons/oxygen/22x22/actions/checkbox.png
/usr/share/icons/oxygen/22x22/actions/distribute-vertical-center.png
/usr/share/icons/oxygen/22x22/actions/view-resource-calendar.png
/usr/share/icons/oxygen/22x22/actions/format-break-node.png
/usr/share/icons/oxygen/22x22/actions/document-print-frame.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-week.png
/usr/share/icons/oxygen/22x22/actions/system-log-out.png
/usr/share/icons/oxygen/22x22/actions/games-config-custom.png
/usr/share/icons/oxygen/22x22/actions/format-disconnect-node.png
/usr/share/icons/oxygen/22x22/actions/view-time-schedule-baselined-add.png
/usr/share/icons/oxygen/22x22/actions/irc-voice.png
/usr/share/icons/oxygen/22x22/actions/view-pim-notes.png
/usr/share/icons/oxygen/22x22/actions/go-last-view.png
/usr/share/icons/oxygen/22x22/actions/flag-red.png
/usr/share/icons/oxygen/22x22/actions/tools-media-optical-burn-image.png
/usr/share/icons/oxygen/22x22/actions/go-bottom.png
/usr/share/icons/oxygen/22x22/actions/text-frame-unlink.png
/usr/share/icons/oxygen/22x22/actions/validators.png
/usr/share/icons/oxygen/22x22/actions/draw-halfcircle3.png
/usr/share/icons/oxygen/22x22/actions/games-hint.png
/usr/share/icons/oxygen/22x22/actions/format-text-strikethrough.png
/usr/share/icons/oxygen/22x22/actions/align-vertical-top-out.png
/usr/share/icons/oxygen/22x22/actions/list-resource-add.png
/usr/share/icons/oxygen/22x22/actions/color-picker-grey.png
/usr/share/icons/oxygen/22x22/actions/view-file-columns.png
/usr/share/icons/oxygen/22x22/actions/view-calendar-upcoming-events.png
/usr/share/icons/oxygen/22x22/emblems
/usr/share/icons/oxygen/22x22/emblems/emblem-favorite.png
/usr/share/icons/oxygen/22x22/emblems/emblem-mounted.png
/usr/share/icons/oxygen/22x22/emblems/emblem-symbolic-link.png
/usr/share/icons/oxygen/22x22/emblems/emblem-important.png
/usr/share/icons/oxygen/22x22/emblems/emblem-unmounted.png
/usr/share/icons/oxygen/22x22/filesystems
/usr/share/icons/oxygen/22x22/places
/usr/share/icons/oxygen/22x22/places/folder-tar.png
/usr/share/icons/oxygen/22x22/places/network-server.png
/usr/share/icons/oxygen/22x22/places/folder-violet.png
/usr/share/icons/oxygen/22x22/places/folder-html.png
/usr/share/icons/oxygen/22x22/places/mail-message.png
/usr/share/icons/oxygen/22x22/places/folder-important.png
/usr/share/icons/oxygen/22x22/places/folder-blue.png
/usr/share/icons/oxygen/22x22/places/network-server-database.png
/usr/share/icons/oxygen/22x22/places/server-database.png
/usr/share/icons/oxygen/22x22/places/repository.png
/usr/share/icons/oxygen/22x22/places/folder-video.png
/usr/share/icons/oxygen/22x22/places/folder-grey.png
/usr/share/icons/oxygen/22x22/places/folder-yellow.png
/usr/share/icons/oxygen/22x22/places/mail-folder-sent.png
/usr/share/icons/oxygen/22x22/places/document-multiple.png
/usr/share/icons/oxygen/22x22/places/folder-cyan.png
/usr/share/icons/oxygen/22x22/places/start-here-kde.png
/usr/share/icons/oxygen/22x22/places/folder-bookmark.png
/usr/share/icons/oxygen/22x22/places/folder-documents.png
/usr/share/icons/oxygen/22x22/places/user-trash.png
/usr/share/icons/oxygen/22x22/places/folder-development.png
/usr/share/icons/oxygen/22x22/places/folder.png
/usr/share/icons/oxygen/22x22/places/user-desktop.png
/usr/share/icons/oxygen/22x22/places/bookmarks.png
/usr/share/icons/oxygen/22x22/places/folder-remote.png
/usr/share/icons/oxygen/22x22/places/user-home.png
/usr/share/icons/oxygen/22x22/places/folder-green.png
/usr/share/icons/oxygen/22x22/places/start-here.png
/usr/share/icons/oxygen/22x22/places/mail-folder-inbox.png
/usr/share/icons/oxygen/22x22/places/folder-favorites.png
/usr/share/icons/oxygen/22x22/places/folder-red.png
/usr/share/icons/oxygen/22x22/places/folder-txt.png
/usr/share/icons/oxygen/22x22/places/user-identity.png
/usr/share/icons/oxygen/22x22/places/folder-brown.png
/usr/share/icons/oxygen/22x22/places/folder-downloads.png
/usr/share/icons/oxygen/22x22/places/folder-print.png
/usr/share/icons/oxygen/22x22/places/folder-orange.png
/usr/share/icons/oxygen/22x22/places/favorites.png
/usr/share/icons/oxygen/22x22/places/folder-locked.png
/usr/share/icons/oxygen/22x22/places/network-workgroup.png
/usr/share/icons/oxygen/22x22/places/mail-folder-outbox.png
/usr/share/icons/oxygen/22x22/places/folder-sound.png
/usr/share/icons/oxygen/22x22/places/folder-image.png
/usr/share/icons/oxygen/22x22/animations
/usr/share/icons/oxygen/22x22/animations/process-working-kde.png
/usr/share/icons/oxygen/22x22/animations/process-idle-kde.png
/usr/share/icons/oxygen/22x22/animations/process-idle.png
/usr/share/icons/oxygen/22x22/animations/process-working.png
/usr/share/icons/oxygen/22x22/status
/usr/share/icons/oxygen/22x22/status/battery-missing.png
/usr/share/icons/oxygen/22x22/status/mail-unread-new.png
/usr/share/icons/oxygen/22x22/status/media-playlist-shuffle.png
/usr/share/icons/oxygen/22x22/status/weather-showers-night.png
/usr/share/icons/oxygen/22x22/status/dialog-information.png
/usr/share/icons/oxygen/22x22/status/window-suppressed.png
/usr/share/icons/oxygen/22x22/status/task-recurring.png
/usr/share/icons/oxygen/22x22/status/weather-showers.png
/usr/share/icons/oxygen/22x22/status/printer-error.png
/usr/share/icons/oxygen/22x22/status/object-locked.png
/usr/share/icons/oxygen/22x22/status/weather-showers-scattered.png
/usr/share/icons/oxygen/22x22/status/weather-freezing-rain.png
/usr/share/icons/oxygen/22x22/status/battery-charging-060.png
/usr/share/icons/oxygen/22x22/status/security-low.png
/usr/share/icons/oxygen/22x22/status/user-away.png
/usr/share/icons/oxygen/22x22/status/weather-few-clouds.png
/usr/share/icons/oxygen/22x22/status/weather-showers-scattered-day.png
/usr/share/icons/oxygen/22x22/status/appointment-reminder.png
/usr/share/icons/oxygen/22x22/status/audio-volume-medium.png
/usr/share/icons/oxygen/22x22/status/user-offline.png
/usr/share/icons/oxygen/22x22/status/task-reminder.png
/usr/share/icons/oxygen/22x22/status/user-online.png
/usr/share/icons/oxygen/22x22/status/weather-snow-scattered-day.png
/usr/share/icons/oxygen/22x22/status/weather-snow.png
/usr/share/icons/oxygen/22x22/status/battery-040.png
/usr/share/icons/oxygen/22x22/status/dialog-error.png
/usr/share/icons/oxygen/22x22/status/battery-060.png
/usr/share/icons/oxygen/22x22/status/battery-charging-caution.png
/usr/share/icons/oxygen/22x22/status/user-busy.png
/usr/share/icons/oxygen/22x22/status/folder-open.png
/usr/share/icons/oxygen/22x22/status/weather-clear.png
/usr/share/icons/oxygen/22x22/status/printer-printing.png
/usr/share/icons/oxygen/22x22/status/audio-volume-muted.png
/usr/share/icons/oxygen/22x22/status/mail-replied.png
/usr/share/icons/oxygen/22x22/status/dialog-warning.png
/usr/share/icons/oxygen/22x22/status/mail-task.png
/usr/share/icons/oxygen/22x22/status/battery-charging-low.png
/usr/share/icons/oxygen/22x22/status/mail-queued.png
/usr/share/icons/oxygen/22x22/status/mail-attachment.png
/usr/share/icons/oxygen/22x22/status/wallet-closed.png
/usr/share/icons/oxygen/22x22/status/security-medium.png
/usr/share/icons/oxygen/22x22/status/weather-hail.png
/usr/share/icons/oxygen/22x22/status/image-loading.png
/usr/share/icons/oxygen/22x22/status/battery-100.png
/usr/share/icons/oxygen/22x22/status/battery-charging.png
/usr/share/icons/oxygen/22x22/status/weather-storm.png
/usr/share/icons/oxygen/22x22/status/weather-snow-rain.png
/usr/share/icons/oxygen/22x22/status/appointment-recurring.png
/usr/share/icons/oxygen/22x22/status/weather-mist.png
/usr/share/icons/oxygen/22x22/status/user-trash-full.png
/usr/share/icons/oxygen/22x22/status/user-away-extended.png
/usr/share/icons/oxygen/22x22/status/weather-snow-scattered.png
/usr/share/icons/oxygen/22x22/status/mail-unread.png
/usr/share/icons/oxygen/22x22/status/meeting-organizer.png
/usr/share/icons/oxygen/22x22/status/weather-showers-day.png
/usr/share/icons/oxygen/22x22/status/script-error.png
/usr/share/icons/oxygen/22x22/status/battery-charging-040.png
/usr/share/icons/oxygen/22x22/status/weather-few-clouds-night.png
/usr/share/icons/oxygen/22x22/status/audio-volume-low.png
/usr/share/icons/oxygen/22x22/status/security-high.png
/usr/share/icons/oxygen/22x22/status/battery-caution.png
/usr/share/icons/oxygen/22x22/status/media-playlist-repeat.png
/usr/share/icons/oxygen/22x22/status/battery-charging-080.png
/usr/share/icons/oxygen/22x22/status/weather-snow-scattered-night.png
/usr/share/icons/oxygen/22x22/status/object-unlocked.png
/usr/share/icons/oxygen/22x22/status/battery-low.png
/usr/share/icons/oxygen/22x22/status/image-missing.png
/usr/share/icons/oxygen/22x22/status/wallet-open.png
/usr/share/icons/oxygen/22x22/status/weather-clear-night.png
/usr/share/icons/oxygen/22x22/status/mail-read.png
/usr/share/icons/oxygen/22x22/status/user-invisible.png
/usr/share/icons/oxygen/22x22/status/weather-showers-scattered-night.png
/usr/share/icons/oxygen/22x22/status/weather-clouds.png
/usr/share/icons/oxygen/22x22/status/task-complete.png
/usr/share/icons/oxygen/22x22/status/weather-many-clouds.png
/usr/share/icons/oxygen/22x22/status/dialog-password.png
/usr/share/icons/oxygen/22x22/status/weather-clouds-night.png
/usr/share/icons/oxygen/22x22/status/audio-volume-high.png
/usr/share/icons/oxygen/22x22/status/battery-080.png
/usr/share/icons/oxygen/22x22/categories
/usr/share/icons/oxygen/22x22/categories/preferences-other.png
/usr/share/icons/oxygen/22x22/categories/applications-multimedia.png
/usr/share/icons/oxygen/22x22/categories/applications-graphics.png
/usr/share/icons/oxygen/22x22/categories/applications-education-mathematics.png
/usr/share/icons/oxygen/22x22/categories/applications-internet.png
/usr/share/icons/oxygen/22x22/categories/applications-toys.png
/usr/share/icons/oxygen/22x22/categories/applications-development-web.png
/usr/share/icons/oxygen/22x22/categories/preferences-system.png
/usr/share/icons/oxygen/22x22/categories/preferences-desktop.png
/usr/share/icons/oxygen/22x22/categories/applications-education.png
/usr/share/icons/oxygen/22x22/categories/system-help.png
/usr/share/icons/oxygen/22x22/categories/applications-games.png
/usr/share/icons/oxygen/22x22/categories/applications-development-translation.png
/usr/share/icons/oxygen/22x22/categories/applications-engineering.png
/usr/share/icons/oxygen/22x22/categories/preferences-desktop-peripherals.png
/usr/share/icons/oxygen/22x22/categories/applications-education-language.png
/usr/share/icons/oxygen/22x22/categories/preferences-system-network.png
/usr/share/icons/oxygen/22x22/categories/applications-accessories.png
/usr/share/icons/oxygen/22x22/categories/applications-office.png
/usr/share/icons/oxygen/22x22/categories/preferences-desktop-personal.png
/usr/share/icons/oxygen/22x22/categories/applications-utilities.png
/usr/share/icons/oxygen/22x22/categories/applications-system.png
/usr/share/icons/oxygen/22x22/categories/applications-other.png
/usr/share/icons/oxygen/22x22/categories/applications-science.png
/usr/share/icons/oxygen/22x22/categories/applications-development.png
/usr/share/icons/oxygen/22x22/emotes
/usr/share/icons/oxygen/22x22/emotes/face-ninja.png
/usr/share/icons/oxygen/22x22/emotes/gift.png
/usr/share/icons/oxygen/22x22/emotes/face-smile.png
/usr/share/icons/oxygen/22x22/emotes/rose.png
/usr/share/icons/oxygen/22x22/emotes/face-laughing.png
/usr/share/icons/oxygen/22x22/emotes/food.png
/usr/share/icons/oxygen/22x22/emotes/face-plain.png
/usr/share/icons/oxygen/22x22/emotes/rose-wilted.png
/usr/share/icons/oxygen/22x22/emotes/face-yawn.png
/usr/share/icons/oxygen/22x22/emotes/face-wink.png
/usr/share/icons/oxygen/22x22/emotes/face-crying.png
/usr/share/icons/oxygen/22x22/emotes/face-uncertain.png
/usr/share/icons/oxygen/22x22/emotes/heart.png
/usr/share/icons/oxygen/22x22/emotes/face-raspberry.png
/usr/share/icons/oxygen/22x22/emotes/face-sad.png
/usr/share/icons/oxygen/22x22/emotes/opinion-disagree.png
/usr/share/icons/oxygen/22x22/emotes/face-worried.png
/usr/share/icons/oxygen/22x22/emotes/food-pizza.png
/usr/share/icons/oxygen/22x22/emotes/face-smile-gearhead-female.png
/usr/share/icons/oxygen/22x22/emotes/face-angel.png
/usr/share/icons/oxygen/22x22/emotes/face-devilish.png
/usr/share/icons/oxygen/22x22/emotes/face-smirk.png
/usr/share/icons/oxygen/22x22/emotes/face-pirate.png
/usr/share/icons/oxygen/22x22/emotes/face-star.png
/usr/share/icons/oxygen/22x22/emotes/face-quiet.png
/usr/share/icons/oxygen/22x22/emotes/face-angry.png
/usr/share/icons/oxygen/22x22/emotes/face-smile-big.png
/usr/share/icons/oxygen/22x22/emotes/face-clown.png
/usr/share/icons/oxygen/22x22/emotes/face-confused.png
/usr/share/icons/oxygen/22x22/emotes/opinion-okay.png
/usr/share/icons/oxygen/22x22/emotes/drink-beer.png
/usr/share/icons/oxygen/22x22/emotes/food-cake.png
/usr/share/icons/oxygen/22x22/emotes/face-smile-gearhead-male.png
/usr/share/icons/oxygen/22x22/emotes/heart-broken.png
/usr/share/icons/oxygen/22x22/emotes/opinion-no.png
/usr/share/icons/oxygen/22x22/emotes/opinion-agree.png
/usr/share/icons/oxygen/22x22/emotes/face-smile-grin.png
/usr/share/icons/oxygen/22x22/emotes/face-sleep.png
/usr/share/icons/oxygen/22x22/emotes/face-cool.png
/usr/share/icons/oxygen/22x22/emotes/face-laugh.png
/usr/share/icons/oxygen/22x22/emotes/face-embarrassed.png
/usr/share/icons/oxygen/22x22/emotes/face-in-love.png
/usr/share/icons/oxygen/22x22/emotes/face-sleeping.png
/usr/share/icons/oxygen/22x22/emotes/drink-martini.png
/usr/share/icons/oxygen/22x22/emotes/face-hug-right.png
/usr/share/icons/oxygen/22x22/emotes/face-sick.png
/usr/share/icons/oxygen/22x22/emotes/face-surprise.png
/usr/share/icons/oxygen/22x22/emotes/face-hug-left.png
/usr/share/icons/oxygen/22x22/emotes/face-foot-in-mouth.png
/usr/share/icons/oxygen/22x22/emotes/face-glasses.png
/usr/share/icons/oxygen/22x22/emotes/face-kiss.png
/usr/share/icons/oxygen/22x22/mimetypes
/usr/share/icons/oxygen/22x22/mimetypes/application-x-wmf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-applix-spreadsheet.png
/usr/share/icons/oxygen/22x22/mimetypes/text-csv.png
/usr/share/icons/oxygen/22x22/mimetypes/application-sxw.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.sun.xml.draw.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-python-bytecode.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-plasma.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-speex+ogg.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-shellscript.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-zoo.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-troff-man.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-cue.png
/usr/share/icons/oxygen/22x22/mimetypes/text-rdf.png
/usr/share/icons/oxygen/22x22/mimetypes/text-html.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-cpio.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.ms-excel.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.formula.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-generic.png
/usr/share/icons/oxygen/22x22/mimetypes/uri-rtspt.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-mplayer2.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.text.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-egon.png
/usr/share/icons/oxygen/22x22/mimetypes/text-vnd.abc.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-ac3.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-generic.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-hex.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-ldif.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-c++hdr.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-arj.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.database.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-csrc.png
/usr/share/icons/oxygen/22x22/mimetypes/image-svg+xml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-lzma-compressed-tar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-executable-script.png
/usr/share/icons/oxygen/22x22/mimetypes/application-relaxng.png
/usr/share/icons/oxygen/22x22/mimetypes/message.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.stardivision.draw.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-tcl.png
/usr/share/icons/oxygen/22x22/mimetypes/x-office-document.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-krita.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-gzpostscript.png
/usr/share/icons/oxygen/22x22/mimetypes/application-pkcs7-signature.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-type1.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.rn-realmedia.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-object.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-nfo.png
/usr/share/icons/oxygen/22x22/mimetypes/image-svg+xml-compressed.png
/usr/share/icons/oxygen/22x22/mimetypes/application-pgp-encrypted.png
/usr/share/icons/oxygen/22x22/mimetypes/application-octet-stream.png
/usr/share/icons/oxygen/22x22/mimetypes/video-x-mng.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-pem-key.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-ttf.png
/usr/share/icons/oxygen/22x22/mimetypes/uri-pnm.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-java.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-tzo.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.graphics.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-adpcm.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-tar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-kgetlist.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.spreadsheet-template.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-desktop.png
/usr/share/icons/oxygen/22x22/mimetypes/image-x-generic.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-cda.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-otf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-rss+xml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-perl.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-java-archive.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-haskell.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-archive.png
/usr/share/icons/oxygen/22x22/mimetypes/application-javascript.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-bibtex.png
/usr/share/icons/oxygen/22x22/mimetypes/application-xsd.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-php.png
/usr/share/icons/oxygen/22x22/mimetypes/uri-mmst.png
/usr/share/icons/oxygen/22x22/mimetypes/text-css.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-katefilelist.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-script.png
/usr/share/icons/oxygen/22x22/mimetypes/application-pgp-keys.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-snf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.ms-powerpoint.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.chart.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-siag.png
/usr/share/icons/oxygen/22x22/mimetypes/application-rtf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-pdf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-designer.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-7z-compressed.png
/usr/share/icons/oxygen/22x22/mimetypes/application-illustrator.png
/usr/share/icons/oxygen/22x22/mimetypes/x-media-podcast.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-tex.png
/usr/share/icons/oxygen/22x22/mimetypes/odf.png
/usr/share/icons/oxygen/22x22/mimetypes/text-vcalendar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-java-applet.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.image.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-nzb.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-patch.png
/usr/share/icons/oxygen/22x22/mimetypes/application-postscript.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-chdr.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-csharp.png
/usr/share/icons/oxygen/22x22/mimetypes/application-pkcs7-mime.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-mswinurl.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.sun.xml.calc.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-objcsrc.png
/usr/share/icons/oxygen/22x22/mimetypes/x-office-spreadsheet.png
/usr/share/icons/oxygen/22x22/mimetypes/inode-directory.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-bdf.png
/usr/share/icons/oxygen/22x22/mimetypes/x-office-calendar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-pcf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-rpm.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-marble.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-kvtml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-smb-server.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-deb.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-awk.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-trash.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-bzip-compressed-tar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-java.png
/usr/share/icons/oxygen/22x22/mimetypes/text-xmcd.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-dtd.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-po.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-tgif.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-kontour.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-wav.png
/usr/share/icons/oxygen/22x22/mimetypes/uri-rtspu.png
/usr/share/icons/oxygen/22x22/mimetypes/text-mathml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-zip.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-monkey.png
/usr/share/icons/oxygen/22x22/mimetypes/uri-mmsu.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-tarz.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-qet-project.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-compressed-tar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.scribus.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-changelog.png
/usr/share/icons/oxygen/22x22/mimetypes/x-kde-nsplugin-generated.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-sql.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-gzdvi.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-log.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-gnumeric.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-prs.sid.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-javascript.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-bzip.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-copying.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-lyx.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-vcard.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-cd-image.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-xslfo.png
/usr/share/icons/oxygen/22x22/mimetypes/image-x-xfig.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-rar.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-ace.png
/usr/share/icons/oxygen/22x22/mimetypes/kopete_emoticons.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-smb-workgroup.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-kcsrc.png
/usr/share/icons/oxygen/22x22/mimetypes/application-xhtml+xml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-pgp-signature.png
/usr/share/icons/oxygen/22x22/mimetypes/video-x-generic.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-shockwave-flash.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-mimearchive.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-sharedlib.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-authors.png
/usr/share/icons/oxygen/22x22/mimetypes/text-troff.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-zerosize.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-flac+ogg.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-ar.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-install.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-ms-dos-executable.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-font-afm.png
/usr/share/icons/oxygen/22x22/mimetypes/message-news.png
/usr/share/icons/oxygen/22x22/mimetypes/text-rtf.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-m4.png
/usr/share/icons/oxygen/22x22/mimetypes/text-calendar.png
/usr/share/icons/oxygen/22x22/mimetypes/encrypted.png
/usr/share/icons/oxygen/22x22/mimetypes/text-sgml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-lzop.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.sun.xml.draw.template.png
/usr/share/icons/oxygen/22x22/mimetypes/image-x-eps.png
/usr/share/icons/oxygen/22x22/mimetypes/uri-mms.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-compress.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-vnd.rn-realvideo.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.spreadsheet.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-makefile.png
/usr/share/icons/oxygen/22x22/mimetypes/fonts-package.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.sun.xml.calc.template.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-gzip.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-ruby.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-quattropro.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-python.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-arc.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-texinfo.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-bzdvi.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-cmake.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-aiff.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.stardivision.calc.png
/usr/share/icons/oxygen/22x22/mimetypes/x-office-contact.png
/usr/share/icons/oxygen/22x22/mimetypes/package-x-generic.png
/usr/share/icons/oxygen/22x22/mimetypes/text-vnd.wap.wml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-msword.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-qet-element.png
/usr/share/icons/oxygen/22x22/mimetypes/image-x-vnd.trolltech.qpicture.png
/usr/share/icons/oxygen/22x22/mimetypes/text-xml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.presentation-template.png
/usr/share/icons/oxygen/22x22/mimetypes/unknown.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-lha.png
/usr/share/icons/oxygen/22x22/mimetypes/application-xml.png
/usr/share/icons/oxygen/22x22/mimetypes/application-xslt+xml.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-adasrc.png
/usr/share/icons/oxygen/22x22/mimetypes/text-enriched.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-subrip.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-x-flac.png
/usr/share/icons/oxygen/22x22/mimetypes/x-office-address-book.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-pak.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-mswrite.png
/usr/share/icons/oxygen/22x22/mimetypes/message-rfc822.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.oasis.opendocument.presentation.png
/usr/share/icons/oxygen/22x22/mimetypes/application-vnd.ms-access.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-bittorrent.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-c++src.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-objchdr.png
/usr/share/icons/oxygen/22x22/mimetypes/audio-midi.png
/usr/share/icons/oxygen/22x22/mimetypes/text-plain.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-pascal.png
/usr/share/icons/oxygen/22x22/mimetypes/text-x-readme.png
/usr/share/icons/oxygen/22x22/mimetypes/application-x-executable.png
/usr/share/icons/oxygen/22x22/mimetypes/text-directory.png
/usr/share/icons/oxygen/22x22/apps
/usr/share/icons/oxygen/22x22/apps/konqueror.png
/usr/share/icons/oxygen/22x22/apps/kaffeine.png
/usr/share/icons/oxygen/22x22/apps/kopete_all_away.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-cryptography.png
/usr/share/icons/oxygen/22x22/apps/krita.png
/usr/share/icons/oxygen/22x22/apps/knotes.png
/usr/share/icons/oxygen/22x22/apps/rawconverter.png
/usr/share/icons/oxygen/22x22/apps/graphics-viewer-document.png
/usr/share/icons/oxygen/22x22/apps/office-calendar.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-windows.png
/usr/share/icons/oxygen/22x22/apps/step.png
/usr/share/icons/oxygen/22x22/apps/kchart.png
/usr/share/icons/oxygen/22x22/apps/preferences-web-browser-shortcuts.png
/usr/share/icons/oxygen/22x22/apps/kolf.png
/usr/share/icons/oxygen/22x22/apps/oxygen.png
/usr/share/icons/oxygen/22x22/apps/lokalize.png
/usr/share/icons/oxygen/22x22/apps/krdc.png
/usr/share/icons/oxygen/22x22/apps/kwalletmanager.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-wallpaper.png
/usr/share/icons/oxygen/22x22/apps/konversation.png
/usr/share/icons/oxygen/22x22/apps/preferences-plugin.png
/usr/share/icons/oxygen/22x22/apps/klipper.png
/usr/share/icons/oxygen/22x22/apps/utilities-file-archiver.png
/usr/share/icons/oxygen/22x22/apps/tagua.png
/usr/share/icons/oxygen/22x22/apps/digikam.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-notification-bell.png
/usr/share/icons/oxygen/22x22/apps/ksniffer.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-default-applications.png
/usr/share/icons/oxygen/22x22/apps/strigi.png
/usr/share/icons/oxygen/22x22/apps/kdf.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-power-management.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-windows-move.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-network-sharing.png
/usr/share/icons/oxygen/22x22/apps/preferences-web-browser-cache.png
/usr/share/icons/oxygen/22x22/apps/utilities-log-viewer.png
/usr/share/icons/oxygen/22x22/apps/system-file-manager.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-font.png
/usr/share/icons/oxygen/22x22/apps/kblogger.png
/usr/share/icons/oxygen/22x22/apps/xorg.png
/usr/share/icons/oxygen/22x22/apps/plasma.png
/usr/share/icons/oxygen/22x22/apps/kplato.png
/usr/share/icons/oxygen/22x22/apps/accessories-dictionary.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-text-to-speech.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-bluetooth.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-keyboard.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-session-services.png
/usr/share/icons/oxygen/22x22/apps/inkscape.png
/usr/share/icons/oxygen/22x22/apps/kontact.png
/usr/share/icons/oxygen/22x22/apps/fontforge.png
/usr/share/icons/oxygen/22x22/apps/korgac.png
/usr/share/icons/oxygen/22x22/apps/qelectrotech.png
/usr/share/icons/oxygen/22x22/apps/plasmagik.png
/usr/share/icons/oxygen/22x22/apps/wine.png
/usr/share/icons/oxygen/22x22/apps/internet-telephony.png
/usr/share/icons/oxygen/22x22/apps/preferences-web-browser-cookies.png
/usr/share/icons/oxygen/22x22/apps/kbugbuster.png
/usr/share/icons/oxygen/22x22/apps/akregator.png
/usr/share/icons/oxygen/22x22/apps/kjournal.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-theme.png
/usr/share/icons/oxygen/22x22/apps/ktorrent.png
/usr/share/icons/oxygen/22x22/apps/scribus.png
/usr/share/icons/oxygen/22x22/apps/preferences-web-browser-adblock.png
/usr/share/icons/oxygen/22x22/apps/krfb.png
/usr/share/icons/oxygen/22x22/apps/accessories-calculator.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-user.png
/usr/share/icons/oxygen/22x22/apps/ksudoku.png
/usr/share/icons/oxygen/22x22/apps/ktip.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-icons.png
/usr/share/icons/oxygen/22x22/apps/yakuake.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-color.png
/usr/share/icons/oxygen/22x22/apps/akonadi.png
/usr/share/icons/oxygen/22x22/apps/utilities-desktop-extra.png
/usr/share/icons/oxygen/22x22/apps/java.png
/usr/share/icons/oxygen/22x22/apps/preferences-kcalc-constants.png
/usr/share/icons/oxygen/22x22/apps/kuickshow.png
/usr/share/icons/oxygen/22x22/apps/showfoto.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-user-password.png
/usr/share/icons/oxygen/22x22/apps/kword.png
/usr/share/icons/oxygen/22x22/apps/kspread.png
/usr/share/icons/oxygen/22x22/apps/semn.png
/usr/share/icons/oxygen/22x22/apps/internet-mail.png
/usr/share/icons/oxygen/22x22/apps/kcmkwm.png
/usr/share/icons/oxygen/22x22/apps/kverbos.png
/usr/share/icons/oxygen/22x22/apps/preferences-contact-list.png
/usr/share/icons/oxygen/22x22/apps/kthesaurus.png
/usr/share/icons/oxygen/22x22/apps/system-software-update.png
/usr/share/icons/oxygen/22x22/apps/kopete_offline.png
/usr/share/icons/oxygen/22x22/apps/esd.png
/usr/share/icons/oxygen/22x22/apps/office-address-book.png
/usr/share/icons/oxygen/22x22/apps/kig.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-notification.png
/usr/share/icons/oxygen/22x22/apps/okteta.png
/usr/share/icons/oxygen/22x22/apps/preferences-web-browser-stylesheets.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-screensaver.png
/usr/share/icons/oxygen/22x22/apps/acroread.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-launch-feedback.png
/usr/share/icons/oxygen/22x22/apps/nepomuk.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-filetype-association.png
/usr/share/icons/oxygen/22x22/apps/quassel.png
/usr/share/icons/oxygen/22x22/apps/utilities-system-monitor.png
/usr/share/icons/oxygen/22x22/apps/networkmanager.png
/usr/share/icons/oxygen/22x22/apps/kexi.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-accessibility.png
/usr/share/icons/oxygen/22x22/apps/utilities-terminal.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-locale.png
/usr/share/icons/oxygen/22x22/apps/kde.png
/usr/share/icons/oxygen/22x22/apps/multimedia-volume-control.png
/usr/share/icons/oxygen/22x22/apps/hwinfo.png
/usr/share/icons/oxygen/22x22/apps/kopete_some_away.png
/usr/share/icons/oxygen/22x22/apps/help-browser.png
/usr/share/icons/oxygen/22x22/apps/kfontview.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-printer.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-sound.png
/usr/share/icons/oxygen/22x22/apps/kde-windows.png
/usr/share/icons/oxygen/22x22/apps/kbruch.png
/usr/share/icons/oxygen/22x22/apps/kopete_some_online.png
/usr/share/icons/oxygen/22x22/apps/k3b.png
/usr/share/icons/oxygen/22x22/apps/ksnapshot.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-display-color.png
/usr/share/icons/oxygen/22x22/apps/accessories-text-editor.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-gaming.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-login.png
/usr/share/icons/oxygen/22x22/apps/kcolorchooser.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-windows-actions.png
/usr/share/icons/oxygen/22x22/apps/internet-web-browser.png
/usr/share/icons/oxygen/22x22/apps/email.png
/usr/share/icons/oxygen/22x22/apps/kollision.png
/usr/share/icons/oxygen/22x22/apps/accessories-character-map.png
/usr/share/icons/oxygen/22x22/apps/kpgp.png
/usr/share/icons/oxygen/22x22/apps/preferences-plugin-script.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-time.png
/usr/share/icons/oxygen/22x22/apps/kmplayer.png
/usr/share/icons/oxygen/22x22/apps/kformula.png
/usr/share/icons/oxygen/22x22/apps/partitionmanager.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-display.png
/usr/share/icons/oxygen/22x22/apps/system-users.png
/usr/share/icons/oxygen/22x22/apps/bovo.png
/usr/share/icons/oxygen/22x22/apps/kcmdf.png
/usr/share/icons/oxygen/22x22/apps/preferences-desktop-mouse.png
/usr/share/icons/oxygen/22x22/apps/kpresenter.png
/usr/share/icons/oxygen/22x22/apps/preferences-system-performance.png
/usr/share/icons/oxygen/22x22/apps/basket.png
/usr/share/icons/oxygen/22x22/intl
}

set list {
open_location /usr/share/icons/oxygen/22x22/actions/document-open-remote.png
close_tab /usr/share/icons/oxygen/22x22/actions/tab-close.png
close_window /usr/share/icons/oxygen/22x22/actions/window-close.png
about /usr/share/icons/oxygen/22x22/actions/help-about.png
back /usr/share/icons/oxygen/22x22/actions/arrow-left.png
back /usr/share/icons/oxygen/22x22/actions/media-skip-backward.png
back /usr/share/icons/oxygen/22x22/actions/draw-arrow-back.png
bookmark /usr/share/icons/oxygen/22x22/actions/bookmark-new-list.png
bookmark /usr/share/icons/oxygen/22x22/actions/bookmarks.png
copy /usr/share/icons/oxygen/22x22/actions/edit-copy.png
cut /usr/share/icons/oxygen/22x22/actions/edit-cut.png
delete /usr/share/icons/oxygen/22x22/actions/edit-delete.png
download /usr/share/icons/oxygen/22x22/actions/download.png
find /usr/share/icons/oxygen/22x22/actions/edit-find.png
forward /usr/share/icons/oxygen/22x22/actions/arrow-right.png
forward /usr/share/icons/oxygen/22x22/actions/media-skip-forward.png
forward /usr/share/icons/oxygen/22x22/actions/draw-arrow-forward.png
help /usr/share/icons/oxygen/22x22/actions/help-contents.png
help /usr/share/icons/oxygen/22x22/actions/help-contextual.png
help /usr/share/icons/oxygen/22x22/actions/help-hint.png
help /usr/share/icons/oxygen/22x22/apps/help-browser.png
help /usr/share/icons/oxygen/22x22/categories/system-help.png
home /usr/share/icons/oxygen/22x22/actions/go-home.png
home /usr/share/icons/oxygen/22x22/places/user-home.png
new_tab /usr/share/icons/oxygen/22x22/actions/tab-new.png
new_window /usr/share/icons/oxygen/22x22/actions/window-new.png
open_file /usr/share/icons/oxygen/22x22/actions/document-open.png
paste /usr/share/icons/oxygen/22x22/actions/edit-paste.png
preferences /usr/share/icons/oxygen/22x22/categories/preferences-desktop.png
preferences /usr/share/icons/oxygen/22x22/categories/preferences-other.png
preferences /usr/share/icons/oxygen/22x22/categories/preferences-system.png
print /usr/share/icons/oxygen/22x22/actions/document-print.png
print_preview /usr/share/icons/oxygen/22x22/actions/document-preview.png
quit /usr/share/icons/oxygen/22x22/actions/application-exit.png
redo /usr/share/icons/oxygen/22x22/actions/edit-redo.png
reload /usr/share/icons/oxygen/22x22/actions/view-refresh.png
save_as /usr/share/icons/oxygen/22x22/actions/document-save-as.png
select_all /usr/share/icons/oxygen/22x22/actions/edit-select-all.png
stop /usr/share/icons/oxygen/22x22/actions/media-playback-stop.png
stop /usr/share/icons/oxygen/22x22/actions/process-stop.png
undo /usr/share/icons/oxygen/22x22/actions/edit-undo.png
}

set oldlist {
about /usr/share/gtk-doc/html/pygtk/icons/stock_about_24.png
back /usr/share/gtk-doc/html/pygtk/icons/stock_left_arrow_24.png
bookmark /usr/share/icons/Gant.Xfce/24x24/stock/stock_bookmark.png
copy /usr/share/gtk-doc/html/pygtk/icons/stock_copy_24.png
cut /usr/share/gtk-doc/html/pygtk/icons/stock_cut_24.png
delete /usr/share/gtk-doc/html/pygtk/icons/stock_trash_24.png
download /usr/share/icons/gnome/24x24/emblems/emblem-downloads.png
find /usr/share/gtk-doc/html/pygtk/icons/stock_search_24.png
forward /usr/share/gtk-doc/html/pygtk/icons/stock_right_arrow_24.png
help /usr/share/gtk-doc/html/pygtk/icons/stock_help_24.png
home /usr/share/gtk-doc/html/pygtk/icons/stock_home_24.png
new_tab /usr/share/gtk-doc/html/pygtk/icons/stock_new_24.png
new_window /usr/share/gtk-doc/html/pygtk/icons/stock_network_24.png
open_file /usr/share/gtk-doc/html/pygtk/icons/stock_open_24.png
paste /usr/share/gtk-doc/html/pygtk/icons/stock_paste_24.png
preferences /usr/share/gtk-doc/html/pygtk/icons/stock_preferences_24.png
print /usr/share/gtk-doc/html/pygtk/icons/stock_print_24.png
print_preview /usr/share/gtk-doc/html/pygtk/icons/stock_print_preview_24.png
quit /usr/share/gtk-doc/html/pygtk/icons/stock_exit_24.png
redo /usr/share/gtk-doc/html/pygtk/icons/stock_redo_24.png
reload /usr/share/gtk-doc/html/pygtk/icons/stock_refresh_24.png
save_as /usr/share/gtk-doc/html/pygtk/icons/stock_save_as_24.png
select_all /usr/share/gtk-doc/html/pygtk/icons/stock_broken_image_24.png
stop /usr/share/gtk-doc/html/pygtk/icons/stock_stop_24.png
undo /usr/share/gtk-doc/html/pygtk/icons/stock_undo_24.png
}
foreach {key file} $list {
    set icon($key) [image create picture -file $file]
}

if { [file exists ../library] } {
    set blt_library ../library
}

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

#set image [image create picture -file ~/images.jpeg]
set bg [blt::background create linear -highcolor  grey70 -lowcolor grey95 \
	-jitter 10 -colorscale log -relativeto self]

set image ""

blt::tk::frame .mbar -bg $bg

set t "Hello, World"
blt::combobutton .mbar.file \
    -text "File" \
    -underline 0 \
    -image $image \
    -relief flat \
    -activerelief raised \
    -arrowon off \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -menu .mbar.file.m



blt::combomenu .mbar.file.m \
    -width { 0 400 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.file.m add -text "New Window" -accelerator "Ctrl+N" -underline 0 \
    -icon $icon(new_window)
.mbar.file.m add -text "New Tab" -accelerator "Ctrl+T" -underline 4 \
    -icon $icon(new_tab)
.mbar.file.m add -text "Open Location..." -accelerator "Ctrl+L" -underline 5 \
    -icon $icon(open_location)
.mbar.file.m add -text "Open File..." -accelerator "Ctrl+O" -underline 0 \
    -icon $icon(open_file)
.mbar.file.m add -text "Close Window" -accelerator "Ctrl+Shift+W" -underline 9 \
    -icon $icon(close_window)
.mbar.file.m add -text "Close Tab" -accelerator "Ctrl+W" -underline 0 -icon $icon(close_tab)
.mbar.file.m add -type separator
.mbar.file.m add -text "Save Page As..." -accelerator "Ctrl+O" -underline 10 \
    -icon $icon(save_as)
.mbar.file.m add -text "Save Page As PDF..." -accelerator "Ctrl+Shift+W" -underline 15
.mbar.file.m add -text "Send Link..." -accelerator "Ctrl+W" -underline 1
.mbar.file.m add -type separator
.mbar.file.m add -text "Page Setup..." -underline 8
.mbar.file.m add -text "Print Preview" -accelerator "Ctrl+Shift+W" -underline 9 \
    -icon $icon(print_preview)
.mbar.file.m add -text "Print..." -accelerator "Ctrl+P" -underline 0 \
    -icon $icon(print)
.mbar.file.m add -type separator
.mbar.file.m add -text "Import..." -underline 0
.mbar.file.m add -type separator
.mbar.file.m add -text "Work Offline" -underline 0
.mbar.file.m add -text "Quit" -accelerator "Ctrl+Q" -underline 0 \
    -icon $icon(quit) 

blt::combobutton .mbar.edit \
    -text "Edit" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menu .mbar.edit.m

blt::combomenu .mbar.edit.m \
    -width { 0 400 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.edit.m add -text "Undo" -accelerator "Ctrl+Z"  \
    -icon $icon(undo)
.mbar.edit.m add -text "Redo" -accelerator "Ctrl+Shift+Z"  \
    -icon $icon(redo)
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Cut" -accelerator "Ctrl+X" \
    -icon $icon(cut)
.mbar.edit.m add -text "Copy" -accelerator "Ctrl+C" \
    -icon $icon(copy)
.mbar.edit.m add -text "Paste" -accelerator "Ctrl+V" \
    -icon $icon(paste)
.mbar.edit.m add -text "Delete" -accelerator "Del" \
    -icon $icon(delete)
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Select All" -accelerator "Ctrl+X" \
    -icon $icon(select_all)
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Find" -accelerator "Ctrl+F"  \
    -icon $icon(find)
.mbar.edit.m add -text "Find Again" -accelerator "Ctrl+G"
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Preferences" \
    -icon $icon(preferences)

blt::combomenu .mbar.edit.m.m
.mbar.edit.m.m add -type command -text "five" -accelerator "^A" -command "set t five"
.mbar.edit.m.m add -type command -text "six" -accelerator "^B" -command "set t six"
.mbar.edit.m.m add -type command -text "seven" -accelerator "^C" -command "set t seven"
.mbar.edit.m.m add -type command -text "eight" -accelerator "^D" -command "set t eight"
.mbar.edit.m.m add -type cascade -text "cascade" -accelerator "^E" 


blt::combobutton .mbar.view \
    -text "View" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menu .mbar.view.m

blt::combomenu .mbar.view.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.view.m add -type cascade -text "Toolbars" -underline 0 
.mbar.view.m add -type checkbutton -text "Status Bar" \
    -underline 4 -variable statusbar
.mbar.view.m add -type checkbutton -text "Sidebar" \
    -underline 5 -variable sidebar 
.mbar.view.m add -type checkbutton -text "Adblock Plus: Blockable items" \
    -accelerator "Ctrl+Shift+V" -underline 0 -variable adblock 
.mbar.view.m add -type separator
.mbar.view.m add -text "Stop" -accelerator "Esc" -underline 9 \
    -icon $icon(stop)
.mbar.view.m add -text "Reload" -accelerator "Ctrl+R" -underline 0 \
    -icon $icon(reload)
.mbar.view.m add -type separator
.mbar.view.m add -type cascade -text "Zoom" -accelerator "Ctrl+O" -underline 10 
.mbar.view.m add -type cascade -text "Page Style" -accelerator "Ctrl+Shift+W" \
    -underline 15
.mbar.view.m add -type cascade -text "Character Encoding" -accelerator "Ctrl+W" \
    -underline 1
.mbar.view.m add -type separator
.mbar.view.m add -text "Page Source" -underline 8 -accelerator "Ctrl+U"
.mbar.view.m add -text "Full Screen" -accelerator "F11" -underline 9 


blt::combobutton .mbar.history \
    -text "History" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menu .mbar.history.m

blt::combomenu .mbar.history.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.history.m add -text "Back" -accelerator "Alt+Left Arrow" \
    -underline 0 -icon $icon(back)
.mbar.history.m add -text "Forward" -accelerator "Alt+Right Arrow" \
    -underline 4 -icon $icon(forward)
.mbar.history.m add -text "Home" -accelerator "Alt+Home" \
    -underline 5 -icon $icon(home)
.mbar.history.m add -text "Show All History" -accelerator "Ctrl+Shift+H" \
    -underline 0 
.mbar.history.m add -type separator
.mbar.history.m add -type cascade -text "Recently Closed Tabs" \
    -accelerator "Ctrl+O" -underline 10 

blt::combobutton .mbar.bmarks \
    -text "Bookmarks" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menu .mbar.bmarks.m

blt::combomenu .mbar.bmarks.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.bmarks.m add -text "Bookmark This Page" -accelerator "Ctrl+D" \
    -underline 0 -icon $icon(bookmark)
.mbar.bmarks.m add -text "Subscribe to This Page..." \
    -underline 4 -icon $icon(forward)
.mbar.bmarks.m add -text "Bookmark All Tabs"  \
    -underline 5 -icon $icon(home)
.mbar.bmarks.m add -text "Organize Bookmarks" \
    -underline 0 
.mbar.bmarks.m add -type separator
.mbar.bmarks.m add -type cascade -text "Bookmarks Toolbar" \
    -underline 10 
.mbar.bmarks.m add -type separator
.mbar.bmarks.m add -type cascade -text "Recently Bookmarked" \
    -underline 10 
.mbar.bmarks.m add -type cascade -text "Recent Tags" \
    -underline 10 
.mbar.bmarks.m add -type separator
.mbar.bmarks.m add -text "Page 1" \
    -underline 10 
.mbar.bmarks.m add -text "Page 2" \
    -underline 10 

blt::combobutton .mbar.tools \
    -text "Tools" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menu .mbar.tools.m

blt::combomenu .mbar.tools.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.tools.m add -text "Web Search" -accelerator "Ctrl+K" \
    -underline 0 
.mbar.tools.m add -type separator
.mbar.tools.m add -text "Downloads" -accelerator "Ctrl+Y" \
    -underline 4 -icon $icon(download)
.mbar.tools.m add -text "Add-ons" -underline 0 
.mbar.tools.m add -type separator
.mbar.tools.m add -text "PDF Download - Options" -underline 0 
.mbar.tools.m add -text "Save Images From Tabs" -underline 10 
.mbar.tools.m add -text "Error Console" \
    -accelerator "Ctrl+Shift+J" -underline 10 
.mbar.tools.m add -text "Adblock Plus Preferences..." \
    -accelerator "Ctrl+Shift+E" -underline 10 
.mbar.tools.m add -text "Page Info" \
    -accelerator "Ctrl+I" -underline 10 
.mbar.tools.m add -type separator
.mbar.tools.m add -text "Clear Private Data" \
    -accelerator "Ctrl+Shift+Del" -underline 10 
.mbar.tools.m add -text "Batch Download Settings" \
    -underline 10 

blt::combobutton .mbar.help \
    -text "Help" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menu .mbar.help.m

blt::combomenu .mbar.help.m \
    -width { 0 600 } -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.help.m add -text "Help Contents" \
    -underline 0 -icon $icon(help)
.mbar.help.m add -text "Release Notes" -underline 0
.mbar.help.m add -text "Report Broken Website..." -underline 5
.mbar.help.m add -text "Report Web Forgery..." -underline 0 
.mbar.help.m add -type separator
.mbar.help.m add -text "Check For Updates..." -underline 0 
.mbar.help.m add -text "About..." -underline 0  -icon $icon(about)

canvas .c
blt::table .mbar \
    1,0 .mbar.file -fill both \
    1,1 .mbar.edit -fill both \
    1,2 .mbar.view -fill both \
    1,3 .mbar.history -fill both \
    1,4 .mbar.bmarks -fill both \
    1,5 .mbar.tools -fill both \
    1,6 .mbar.help -fill both \

blt::table configure .mbar c* -padx 2 -resize none
blt::table configure .mbar c7 -resize expand

blt::table . \
    0,0 .mbar -fill x \
    1,0 .c -fill both 

blt::table configure . r0 -resize none
blt::table configure . r1 -resize expand
